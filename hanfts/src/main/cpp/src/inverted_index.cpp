#include "inverted_index.h"
#include "tokenizer.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace fts {

// Magic number written at the start of every index file to detect corruption
// or version mismatches on load.
static constexpr uint32_t FILE_MAGIC = 0x4C465431;

// Title tokens are indexed with this multiplier applied to their frequency so
// that title matches rank above body matches for the same term.
static constexpr int TITLE_WEIGHT = 3;

/**
 * Computes the TF-IDF score contribution of a single term in a document.
 *
 * TF  = term_freq / total_terms   (normalized term frequency)
 * IDF = log((N+1) / (df+1)) + 1  (smoothed inverse document frequency)
 *
 * The +1 smoothing prevents zero IDF for terms that appear in every document
 * and avoids log(0) when the corpus is empty.
 */
float InvertedIndex::termScore(int term_freq, int total_terms, int df) const {
    if (total_terms == 0 || df <= 0) return 0.0f;
    float tf  = static_cast<float>(term_freq) / static_cast<float>(total_terms);
    int   N   = static_cast<int>(docs_.size());
    float idf = std::log(static_cast<float>(N + 1) / static_cast<float>(df + 1)) + 1.0f;
    return tf * idf;
}

/**
 * Accumulates @p tokens into @p term_counts, multiplying each hit by @p weight.
 * Called twice per document: once for the title (weight = TITLE_WEIGHT) and
 * once for the body (weight = 1).
 */
void InvertedIndex::indexTokens(int doc_id,
                                 const std::vector<std::string>& tokens,
                                 int weight,
                                 std::unordered_map<std::string, int>& term_counts) {
    for (const auto& token : tokens) {
        term_counts[token] += weight;
    }
}

/**
 * Indexes a document, replacing any existing document with the same @p doc_id.
 *
 * The document is first removed (if it exists) to ensure posting lists are
 * clean before re-indexing. If no tokens are produced (e.g. the text consists
 * entirely of punctuation) the document is not stored.
 */
void InvertedIndex::addDocument(int doc_id,
                                 const std::string& title,
                                 const std::string& body) {
    removeDocument(doc_id);

    auto title_tokens = Tokenizer::tokenize(title);
    auto body_tokens  = Tokenizer::tokenize(body);

    std::unordered_map<std::string, int> term_counts;
    term_counts.reserve(title_tokens.size() + body_tokens.size());
    indexTokens(doc_id, title_tokens, TITLE_WEIGHT, term_counts);
    indexTokens(doc_id, body_tokens,  1,            term_counts);

    if (term_counts.empty()) return;

    // Store total weighted term count for TF normalization.
    int total = 0;
    for (auto& [term, cnt] : term_counts) {
        total += cnt;
    }
    docs_[doc_id] = DocInfo{total};

    // Add this document to each term's posting list and build the reverse map.
    std::vector<std::string> terms;
    terms.reserve(term_counts.size());
    for (auto& [term, cnt] : term_counts) {
        auto& postings = index_[term];
        postings.push_back(Posting{doc_id, cnt});
        doc_freq_[term]++;
        terms.push_back(term);
    }
    doc_terms_[doc_id] = std::move(terms);
}

/**
 * Removes a document and cleans up all associated posting list entries.
 *
 * Uses doc_terms_ (the reverse map) to locate only the terms that actually
 * appear in this document, avoiding a full index scan.
 * Empty posting lists are removed entirely to keep the index compact.
 */
void InvertedIndex::removeDocument(int doc_id) {
    auto doc_it = docs_.find(doc_id);
    if (doc_it == docs_.end()) return;
    docs_.erase(doc_it);

    auto terms_it = doc_terms_.find(doc_id);
    if (terms_it != doc_terms_.end()) {
        for (const auto& term : terms_it->second) {
            auto idx_it = index_.find(term);
            if (idx_it == index_.end()) continue;

            // Erase this document's entry from the posting list.
            auto& postings = idx_it->second;
            postings.erase(
                std::remove_if(postings.begin(), postings.end(),
                               [doc_id](const Posting& p) { return p.doc_id == doc_id; }),
                postings.end());

            // Decrement document frequency.
            auto df_it = doc_freq_.find(term);
            if (df_it != doc_freq_.end()) {
                df_it->second = std::max(0, df_it->second - 1);
            }

            // Remove the term entirely if no documents reference it anymore.
            if (idx_it->second.empty()) {
                index_.erase(idx_it);
                doc_freq_.erase(term);
            }
        }
        doc_terms_.erase(terms_it);
    }
}

/**
 * Searches the index and returns scored results sorted in descending order.
 *
 * Each query token is expanded to all index terms that share the same prefix
 * (capped at MAX_PREFIX_TERMS per token). This prefix expansion is what makes
 * typing "안녕" match documents containing "안녕하세요": the bigram "안녕" is a
 * prefix of itself in the sorted index, so it resolves immediately.
 *
 * Using std::map::lower_bound() on the sorted index makes prefix iteration
 * O(log N + K) where K is the number of matching terms.
 */
std::vector<SearchResult> InvertedIndex::search(const std::string& query,
                                                  int limit) const {
    auto query_tokens = Tokenizer::tokenize(query);
    if (query_tokens.empty()) return {};

    std::unordered_map<int, float> scores;
    scores.reserve(docs_.size());

    // Hard cap to prevent runaway iteration on very common prefixes.
    static constexpr int MAX_PREFIX_TERMS = 64;

    for (const auto& token : query_tokens) {
        int prefix_count = 0;
        auto it = index_.lower_bound(token);
        while (it != index_.end() && prefix_count < MAX_PREFIX_TERMS) {
            // Stop when the current term no longer starts with the query token.
            if (it->first.size() < token.size() ||
                it->first.compare(0, token.size(), token) != 0) {
                break;
            }

            auto df_it = doc_freq_.find(it->first);
            int  df    = (df_it != doc_freq_.end()) ? df_it->second : 1;

            for (const auto& posting : it->second) {
                auto doc_it = docs_.find(posting.doc_id);
                if (doc_it == docs_.end()) continue;
                scores[posting.doc_id] += termScore(posting.term_freq,
                                                     doc_it->second.total_terms,
                                                     df);
            }
            ++it;
            ++prefix_count;
        }
    }

    // Collect scores and sort descending.
    std::vector<SearchResult> results;
    results.reserve(scores.size());
    for (auto& [id, score] : scores) {
        results.push_back(SearchResult{id, score});
    }
    std::sort(results.begin(), results.end(),
              [](const SearchResult& a, const SearchResult& b) {
                  return a.score > b.score;
              });

    if (limit > 0 && static_cast<int>(results.size()) > limit) {
        results.resize(static_cast<size_t>(limit));
    }
    return results;
}

void InvertedIndex::clear() {
    index_.clear();
    docs_.clear();
    doc_freq_.clear();
    doc_terms_.clear();
}

/**
 * Rebuilds doc_terms_ (the doc→terms reverse map) by iterating over index_.
 * Called after load() because doc_terms_ is not persisted to disk.
 */
void InvertedIndex::buildDocTerms() {
    doc_terms_.clear();
    for (auto& [term, postings] : index_) {
        for (auto& p : postings) {
            doc_terms_[p.doc_id].push_back(term);
        }
    }
}

/**
 * Binary file format (little-endian, all integers are uint32 unless noted):
 *
 *   [magic: uint32]
 *   [num_docs: uint32]
 *   for each doc:
 *     [doc_id: uint32] [total_terms: uint32]
 *   [num_terms: uint32]
 *   for each term:
 *     [term_len: uint16] [term_bytes: uint8 × term_len]
 *     [num_postings: uint32]
 *     for each posting:
 *       [doc_id: uint32] [term_freq: uint32]
 */
bool InvertedIndex::save(const std::string& path) const {
    FILE* f = fopen(path.c_str(), "wb");
    if (!f) return false;

    bool ok = true;
    auto write32 = [&](uint32_t v) { if (fwrite(&v, 4, 1, f) != 1) ok = false; };
    auto write16 = [&](uint16_t v) { if (fwrite(&v, 2, 1, f) != 1) ok = false; };

    write32(FILE_MAGIC);

    write32(static_cast<uint32_t>(docs_.size()));
    for (auto& [id, info] : docs_) {
        write32(static_cast<uint32_t>(id));
        write32(static_cast<uint32_t>(info.total_terms));
    }

    write32(static_cast<uint32_t>(index_.size()));
    for (auto& [term, postings] : index_) {
        write16(static_cast<uint16_t>(term.size()));
        if (fwrite(term.data(), 1, term.size(), f) != term.size()) ok = false;
        write32(static_cast<uint32_t>(postings.size()));
        for (auto& p : postings) {
            write32(static_cast<uint32_t>(p.doc_id));
            write32(static_cast<uint32_t>(p.term_freq));
        }
    }

    fclose(f);
    return ok;
}

/** Loads the index from the binary format written by save(). */
bool InvertedIndex::load(const std::string& path) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false; // File does not exist yet; start with an empty index.

    clear();

    auto read32 = [&](uint32_t& v) -> bool { return fread(&v, 4, 1, f) == 1; };
    auto read16 = [&](uint16_t& v) -> bool { return fread(&v, 2, 1, f) == 1; };

    uint32_t magic = 0;
    if (!read32(magic) || magic != FILE_MAGIC) { fclose(f); return false; }

    uint32_t num_docs = 0;
    if (!read32(num_docs)) { fclose(f); return false; }
    docs_.reserve(num_docs);
    for (uint32_t i = 0; i < num_docs; ++i) {
        uint32_t id = 0, total = 0;
        if (!read32(id) || !read32(total)) { fclose(f); return false; }
        docs_[static_cast<int>(id)] = DocInfo{static_cast<int>(total)};
    }

    uint32_t num_terms = 0;
    if (!read32(num_terms)) { fclose(f); return false; }
    doc_freq_.reserve(num_terms);
    for (uint32_t i = 0; i < num_terms; ++i) {
        uint16_t term_len = 0;
        if (!read16(term_len)) { fclose(f); return false; }
        std::string term(term_len, '\0');
        if (fread(term.data(), 1, term_len, f) != term_len) { fclose(f); return false; }

        uint32_t num_postings = 0;
        if (!read32(num_postings)) { fclose(f); return false; }
        std::vector<Posting> postings;
        postings.reserve(num_postings);
        for (uint32_t j = 0; j < num_postings; ++j) {
            uint32_t doc_id = 0, freq = 0;
            if (!read32(doc_id) || !read32(freq)) { fclose(f); return false; }
            postings.push_back(Posting{static_cast<int>(doc_id), static_cast<int>(freq)});
        }
        doc_freq_[term] = static_cast<int>(postings.size());
        index_[term]    = std::move(postings);
    }

    fclose(f);
    buildDocTerms(); // Reconstruct the reverse map (not stored on disk).
    return true;
}

}

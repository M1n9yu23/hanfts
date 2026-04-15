#pragma once

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <cstdint>

namespace fts {

/** A ranked search result returned by InvertedIndex::search(). */
struct SearchResult {
    int   doc_id; ///< The document identifier supplied at index time.
    float score;  ///< TF-IDF relevance score; higher is more relevant.
};

/**
 * In-memory inverted index with TF-IDF ranking and prefix search.
 *
 * Documents are identified by an integer @c doc_id chosen by the caller.
 * Each document has a title (weighted 3×) and a body. Both are tokenized
 * by Tokenizer before indexing.
 *
 * The index can be persisted to a binary file and reloaded with save()/load().
 *
 * This class is NOT thread-safe. Concurrent access must be synchronized
 * externally (see SearchEngine, which wraps this class with a shared_mutex).
 */
class InvertedIndex {
public:
    /**
     * Indexes a document, replacing any existing document with the same @p doc_id.
     * Title tokens are weighted 3× relative to body tokens.
     */
    void addDocument(int doc_id, const std::string &title, const std::string &body);

    /** Removes the document with the given @p doc_id. No-op if not found. */
    void removeDocument(int doc_id);

    /**
     * Searches the index for documents matching @p query.
     *
     * Each query token is expanded to all index terms that share the same prefix
     * (up to MAX_PREFIX_TERMS = 64 per token) to support partial-word search.
     * Results are scored with a TF-IDF variant and returned sorted descending.
     *
     * @param limit  Maximum number of results; 0 or negative means unlimited.
     */
    std::vector<SearchResult> search(const std::string &query, int limit = 20) const;

    /**
     * Serializes the index to a binary file at @p path.
     * @return true on success, false if the file cannot be written.
     */
    bool save(const std::string &path) const;

    /**
     * Deserializes the index from a binary file at @p path.
     * Clears the current index before loading.
     * @return true on success, false if the file is missing or corrupt.
     */
    bool load(const std::string &path);

    /** Removes all documents and terms from the index. */
    void clear();

    /** Returns the number of documents currently in the index. */
    int docCount() const { return static_cast<int>(docs_.size()); }

private:
    /** One entry in a term's posting list. */
    struct Posting {
        int doc_id;
        int term_freq; ///< Weighted frequency (title hits count 3×).
    };

    /** Per-document metadata used for TF normalization. */
    struct DocInfo {
        int total_terms; ///< Sum of all weighted term frequencies for this doc.
    };

    // Sorted map so that lower_bound() can efficiently find prefix matches.
    std::map<std::string, std::vector<Posting>> index_;

    std::unordered_map<int, DocInfo>              docs_;
    std::unordered_map<std::string, int>          doc_freq_;   ///< Number of docs containing each term.
    std::unordered_map<int, std::vector<std::string>> doc_terms_; ///< Reverse map: doc → terms (for removal).

    /** Computes the TF-IDF score contribution of a single term occurrence. */
    float termScore(int term_freq, int total_terms, int df) const;

    /** Accumulates weighted token frequencies into @p term_counts. */
    void indexTokens(int doc_id,
                     const std::vector<std::string> &tokens,
                     int weight,
                     std::unordered_map<std::string, int> &term_counts);

    /** Rebuilds doc_terms_ from the current index_ (used after load()). */
    void buildDocTerms();
};

}

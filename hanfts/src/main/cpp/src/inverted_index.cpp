/*
 * Copyright 2026 Gyugle
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "inverted_index.h"
#include "tokenizer.h"

#include <algorithm>
#include <cmath>

namespace fts {

static constexpr int TITLE_WEIGHT = 3;

/**
 * TF  = term_freq / total_terms   (normalized term frequency)
 * IDF = log((N+1) / (df+1)) + 1  (smoothed to avoid log(0))
 */
float InvertedIndex::termScore(int term_freq, int total_terms, int df) const {
    if (total_terms == 0 || df <= 0) return 0.0f;
    float tf  = static_cast<float>(term_freq) / static_cast<float>(total_terms);
    int   N   = static_cast<int>(docs_.size());
    float idf = std::log(static_cast<float>(N + 1) / static_cast<float>(df + 1)) + 1.0f;
    return tf * idf;
}

void InvertedIndex::indexTokens(int doc_id,
                                 const std::vector<std::string>& tokens,
                                 int weight,
                                 std::unordered_map<std::string, int>& term_counts) {
    for (const auto& token : tokens) {
        term_counts[token] += weight;
    }
}

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

    int total = 0;
    for (auto& [term, cnt] : term_counts) total += cnt;
    docs_[doc_id] = DocInfo{total};

    std::vector<std::string> terms;
    terms.reserve(term_counts.size());
    for (auto& [term, cnt] : term_counts) {
        index_[term].push_back(Posting{doc_id, cnt});
        doc_freq_[term]++;
        terms.push_back(term);
    }
    doc_terms_[doc_id] = std::move(terms);
}

void InvertedIndex::removeDocument(int doc_id) {
    auto doc_it = docs_.find(doc_id);
    if (doc_it == docs_.end()) return;
    docs_.erase(doc_it);

    auto terms_it = doc_terms_.find(doc_id);
    if (terms_it != doc_terms_.end()) {
        for (const auto& term : terms_it->second) {
            auto idx_it = index_.find(term);
            if (idx_it == index_.end()) continue;

            auto& postings = idx_it->second;
            postings.erase(
                std::remove_if(postings.begin(), postings.end(),
                               [doc_id](const Posting& p) { return p.doc_id == doc_id; }),
                postings.end());

            auto df_it = doc_freq_.find(term);
            if (df_it != doc_freq_.end()) {
                df_it->second = std::max(0, df_it->second - 1);
            }

            if (idx_it->second.empty()) {
                index_.erase(idx_it);
                doc_freq_.erase(term);
            }
        }
        doc_terms_.erase(terms_it);
    }
}

std::vector<SearchResult> InvertedIndex::search(const std::string& query, int limit) const {
    auto query_tokens = Tokenizer::tokenize(query);
    if (query_tokens.empty()) return {};

    std::unordered_map<int, float> scores;
    scores.reserve(docs_.size());

    static constexpr int MAX_PREFIX_TERMS = 64;

    for (const auto& token : query_tokens) {
        int prefix_count = 0;
        auto it = index_.lower_bound(token);
        while (it != index_.end() && prefix_count < MAX_PREFIX_TERMS) {
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

}

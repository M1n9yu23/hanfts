#pragma once

#include "inverted_index.h"
#include <string>
#include <vector>
#include <tuple>
#include <shared_mutex>

namespace fts {

/**
 * Thread-safe in-memory full-text search engine backed by InvertedIndex.
 *
 * Wraps InvertedIndex with a std::shared_mutex to allow multiple concurrent
 * readers (search) while serializing all writers (indexDocument, removeDocument,
 * rebuildIndex). The index lives entirely in memory and is not persisted to disk.
 *
 * Instances are created via the JNI bridge (fts_jni.cpp) using an opaque handle
 * (pointer cast to jlong) and must be explicitly destroyed to avoid memory leaks.
 */
class SearchEngine {
public:
    SearchEngine() = default;

    /** Returns the number of documents currently in the index. */
    int documentCount() const;

    /** Indexes or replaces a document. Acquires an exclusive write lock. */
    void indexDocument(int doc_id, const std::string& title, const std::string& body);

    /** Removes a document by ID. No-op if not found. Acquires an exclusive write lock. */
    void removeDocument(int doc_id);

    /** Removes all documents from the index. Acquires an exclusive write lock. */
    void clear();

    /**
     * Searches the index and returns scored results sorted by relevance.
     * Acquires a shared read lock; multiple concurrent searches are allowed.
     */
    std::vector<SearchResult> search(const std::string& query, int limit = 20) const;

    /**
     * Clears the index and rebuilds it from @p documents.
     * Each element is a tuple of (doc_id, title, body).
     * Acquires an exclusive write lock.
     */
    void rebuildIndex(const std::vector<std::tuple<int, std::string, std::string>>& documents);

private:
    InvertedIndex index_;
    mutable std::shared_mutex mutex_;
};

}

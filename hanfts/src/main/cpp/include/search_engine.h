#pragma once

#include "inverted_index.h"
#include <string>
#include <vector>
#include <tuple>
#include <shared_mutex>

namespace fts {

/**
 * Thread-safe full-text search engine backed by InvertedIndex.
 *
 * Wraps InvertedIndex with a std::shared_mutex to allow multiple concurrent
 * readers (search) while serializing all writers (indexDocument, removeDocument,
 * rebuildIndex). Every mutating operation also persists the index to disk via
 * persist() so the state survives process restarts.
 *
 * Instances are created via the JNI bridge (fts_jni.cpp) using a handle
 * (opaque pointer cast to jlong) and must be explicitly destroyed to avoid
 * memory leaks.
 */
class SearchEngine {
public:
    /**
     * Constructs a SearchEngine that stores its index at @p index_path.
     * If a previously persisted index exists at that path it is loaded
     * immediately; otherwise the engine starts with an empty index.
     */
    explicit SearchEngine(const std::string& index_path);

    /** Indexes or replaces a document. Acquires an exclusive write lock. */
    void indexDocument(int doc_id, const std::string& title, const std::string& body);

    /** Removes a document by ID. Acquires an exclusive write lock. */
    void removeDocument(int doc_id);

    /**
     * Searches the index and returns matching document IDs sorted by score.
     * Acquires a shared read lock; multiple concurrent searches are allowed.
     */
    std::vector<int> search(const std::string& query, int limit = 20);

    /**
     * Clears the index and rebuilds it from @p documents.
     * Each element is a tuple of (doc_id, title, body).
     * Acquires an exclusive write lock.
     */
    void rebuildIndex(const std::vector<std::tuple<int, std::string, std::string>>& documents);

private:
    std::string    index_path_;
    InvertedIndex  index_;
    mutable std::shared_mutex mutex_;

    /** Writes the current index to disk. Must be called while holding a write lock. */
    void persist();
};

}

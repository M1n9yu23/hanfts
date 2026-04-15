#include "search_engine.h"

namespace fts {

/**
 * Constructs the engine and loads any previously persisted index from disk.
 * If the file does not exist yet, InvertedIndex::load() returns false and the
 * engine silently starts with an empty index — this is expected on first use.
 */
SearchEngine::SearchEngine(const std::string& index_path)
    : index_path_(index_path) {
    index_.load(index_path_);
}

/**
 * Indexes or replaces a document. Takes an exclusive (write) lock so that
 * no concurrent reads can see a partially updated index.
 */
void SearchEngine::indexDocument(int doc_id,
                                  const std::string& title,
                                  const std::string& body) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    index_.addDocument(doc_id, title, body);
    persist();
}

/** Removes a document. Takes an exclusive (write) lock. */
void SearchEngine::removeDocument(int doc_id) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    index_.removeDocument(doc_id);
    persist();
}

/**
 * Searches the index. Takes a shared (read) lock so multiple threads can
 * search concurrently without blocking each other.
 */
std::vector<int> SearchEngine::search(const std::string& query, int limit) {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto results = index_.search(query, limit);
    std::vector<int> ids;
    ids.reserve(results.size());
    for (auto& r : results) {
        ids.push_back(r.doc_id);
    }
    return ids;
}

/**
 * Clears the entire index and rebuilds it from @p documents.
 * Takes an exclusive (write) lock for the duration of the rebuild.
 */
void SearchEngine::rebuildIndex(
    const std::vector<std::tuple<int, std::string, std::string>>& documents) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    index_.clear();
    for (auto& [id, title, body] : documents) {
        index_.addDocument(id, title, body);
    }
    persist();
}

/**
 * Writes the current index to disk.
 * Must be called while the caller holds a write lock on mutex_.
 */
void SearchEngine::persist() {
    index_.save(index_path_);
}

}

#include "search_engine.h"

namespace fts {

int SearchEngine::documentCount() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return index_.docCount();
}

void SearchEngine::indexDocument(int doc_id,
                                  const std::string& title,
                                  const std::string& body) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    index_.addDocument(doc_id, title, body);
}

void SearchEngine::removeDocument(int doc_id) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    index_.removeDocument(doc_id);
}

void SearchEngine::clear() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    index_.clear();
}

std::vector<SearchResult> SearchEngine::search(const std::string& query, int limit) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return index_.search(query, limit);
}

void SearchEngine::rebuildIndex(
    const std::vector<std::tuple<int, std::string, std::string>>& documents) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    index_.clear();
    for (auto& [id, title, body] : documents) {
        index_.addDocument(id, title, body);
    }
}

}

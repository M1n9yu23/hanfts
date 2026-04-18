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
#include "search_engine.h"

namespace fts {

int SearchEngine::documentCount() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return index_.docCount();
}

void SearchEngine::indexDocument(int64_t doc_id,
                                  const std::string& title,
                                  const std::string& body) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    index_.addDocument(doc_id, title, body);
}

void SearchEngine::removeDocument(int64_t doc_id) {
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
    const std::vector<std::tuple<int64_t, std::string, std::string>>& documents) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    index_.clear();
    for (auto& [id, title, body] : documents) {
        index_.addDocument(id, title, body);
    }
}

}

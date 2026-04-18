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
package com.gyugle.hanfts

import java.io.Closeable

/**
 * A full-text search engine that indexes and queries documents by integer ID.
 *
 * Supports both Korean (bigram tokenization) and ASCII (word-level) text.
 * Title text is weighted 3× higher than body text in relevance scoring.
 *
 * Obtain an instance via the factory function:
 * ```kotlin
 * SearchEngine().use { engine ->
 *     engine.rebuildIndex(documents)
 *     val results = engine.search("검색어")
 * }
 * ```
 *
 * **Threading:** all mutating operations ([indexDocument], [removeDocument],
 * [rebuildIndex], [clear]) are blocking calls that acquire an exclusive lock
 * internally. Run them on a background dispatcher (e.g. `Dispatchers.Default`)
 * to avoid blocking the main thread.
 * Concurrent [search] calls are always safe and do not block each other.
 */
interface SearchEngine : Closeable {
  /** Number of documents currently in the index. */
  val documentCount: Int

  /**
   * Adds or replaces a document in the index.
   * If a document with the same [id] already exists it is removed first.
   *
   * @throws IllegalStateException if this engine has been closed.
   */
  fun indexDocument(id: Long, title: String, body: String)

  /**
   * Adds or replaces [document] in the index.
   * If a document with the same [Document.id] already exists it is removed first.
   *
   * @throws IllegalStateException if this engine has been closed.
   */
  fun indexDocument(
    document: Document,
  ) = indexDocument(document.id, document.title, document.body)

  /**
   * Removes the document with the given [id] from the index.
   * No-op if no document with that ID exists.
   *
   * @throws IllegalStateException if this engine has been closed.
   */
  fun removeDocument(id: Long)

  /**
   * Removes all documents from the index.
   *
   * @throws IllegalStateException if this engine has been closed.
   */
  fun clear()

  /**
   * Searches indexed documents for [query] and returns results ranked by relevance.
   *
   * @param query Search text (Korean or ASCII). A blank query returns an empty list.
   * @param limit Maximum number of results. Must be positive. Defaults to 20.
   * @return [SearchResult] list ordered by [SearchResult.score] descending.
   * @throws IllegalArgumentException if [limit] is not positive.
   * @throws IllegalStateException if this engine has been closed.
   */
  fun search(query: String, limit: Int = 20): List<SearchResult>

  /**
   * Clears the entire index and rebuilds it from [documents].
   *
   * For large document sets this is a blocking operation; call it on a
   * background dispatcher.
   *
   * @param documents Documents to index.
   * @throws IllegalStateException if this engine has been closed.
   */
  fun rebuildIndex(documents: List<Document>)

  companion object {
    /**
     * Creates a new [SearchEngine] backed by a native C++17 FTS engine.
     *
     * @throws IllegalStateException if the native engine fails to initialise.
     */
    operator fun invoke(): SearchEngine = NativeSearchEngine()
  }
}

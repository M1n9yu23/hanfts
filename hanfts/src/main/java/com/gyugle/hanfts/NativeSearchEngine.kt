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

import java.util.concurrent.atomic.AtomicLong

internal class NativeSearchEngine : SearchEngine {
  private val handle = AtomicLong(nativeCreate())

  init {
    check(handle.get() != 0L) { "Failed to initialise native FTS engine" }
  }

  override val documentCount: Int
    get() = withHandle { nativeDocumentCount(it) }

  override fun indexDocument(id: Long, title: String, body: String) =
    withHandle { nativeIndexDocument(it, id, title, body) }

  override fun removeDocument(id: Long) =
    withHandle { nativeRemoveDocument(it, id) }

  override fun clear() =
    withHandle { nativeClear(it) }

  override fun search(query: String, limit: Int): List<SearchResult> {
    require(limit > 0) { "limit must be positive, was $limit" }
    if (query.isBlank()) return emptyList()
    return withHandle { h ->
      val packed = nativeSearch(h, query, limit)
      List(packed.size / 2) { i ->
        SearchResult(
          id = packed[i * 2],
          score = Float.fromBits(packed[i * 2 + 1].toInt()),
        )
      }
    }
  }

  override fun rebuildIndex(documents: List<Document>) =
    withHandle { h ->
      nativeRebuildIndex(
        h,
        LongArray(documents.size) { documents[it].id },
        Array(documents.size) { documents[it].title },
        Array(documents.size) { documents[it].body },
      )
    }

  override fun close() {
    val h = handle.getAndSet(0L)
    if (h != 0L) nativeDestroy(h)
  }

  private inline fun <T> withHandle(block: (Long) -> T): T {
    val h = handle.get()
    check(h != 0L) { "SearchEngine is closed" }
    return block(h)
  }

  private external fun nativeCreate(): Long

  private external fun nativeDestroy(handle: Long)

  private external fun nativeDocumentCount(handle: Long): Int

  private external fun nativeIndexDocument(handle: Long, id: Long, title: String, body: String)

  private external fun nativeRemoveDocument(handle: Long, id: Long)

  private external fun nativeClear(handle: Long)

  private external fun nativeSearch(handle: Long, query: String, limit: Int): LongArray

  private external fun nativeRebuildIndex(
    handle: Long,
    ids: LongArray,
    titles: Array<String>,
    bodies: Array<String>,
  )

  companion object {
    init {
      System.loadLibrary("hanfts")
    }
  }
}

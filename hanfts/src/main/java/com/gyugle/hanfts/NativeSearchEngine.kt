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
import java.util.concurrent.locks.ReentrantReadWriteLock
import kotlin.concurrent.read
import kotlin.concurrent.write

internal class NativeSearchEngine : SearchEngine {
  private val handle = AtomicLong(nativeCreate())
  private val lock = ReentrantReadWriteLock()

  // String (public) ↔ Long (native) bidirectional mapping. Both maps and the counter
  // are guarded by [lock]: read for queries, write for mutations.
  private val stringToInternalId = HashMap<String, Long>()
  private val internalIdToString = HashMap<Long, String>()
  private val idCounter = AtomicLong(1L)

  init {
    check(handle.get() != 0L) { "Failed to initialise native FTS engine" }
  }

  override val documentCount: Int
    get() = withRead { nativeDocumentCount(it) }

  override fun indexDocument(id: String, title: String, body: String) =
    withWrite { h ->
      val existing = stringToInternalId[id]
      val internalId = existing ?: idCounter.getAndIncrement()
      nativeIndexDocument(h, internalId, title, body)
      if (existing == null) {
        stringToInternalId[id] = internalId
        internalIdToString[internalId] = id
      }
    }

  override fun removeDocument(id: String) =
    withWrite { h ->
      val internalId = stringToInternalId[id] ?: return@withWrite
      nativeRemoveDocument(h, internalId)
      stringToInternalId.remove(id)
      internalIdToString.remove(internalId)
    }

  override fun clear() =
    withWrite { h ->
      nativeClear(h)
      stringToInternalId.clear()
      internalIdToString.clear()
    }

  override fun search(query: String, limit: Int): List<SearchResult> {
    require(limit > 0) { "limit must be positive, was $limit" }
    if (query.isBlank()) return emptyList()
    return withRead { h ->
      val packed = nativeSearch(h, query, limit)
      val out = ArrayList<SearchResult>(packed.size / 2)
      var i = 0
      while (i < packed.size) {
        val stringId = internalIdToString[packed[i]]
        if (stringId != null) {
          out.add(SearchResult(stringId, Float.fromBits(packed[i + 1].toInt())))
        }
        i += 2
      }
      out
    }
  }

  override fun rebuildIndex(documents: List<Document>) =
    withWrite { h ->
      val newForward = HashMap<String, Long>(documents.size)
      val newReverse = HashMap<Long, String>(documents.size)
      val ids =
        LongArray(documents.size) { i ->
          val docId = documents[i].id
          newForward.getOrPut(docId) {
            val newId = idCounter.getAndIncrement()
            newReverse[newId] = docId
            newId
          }
        }
      nativeRebuildIndex(
        h,
        ids,
        Array(documents.size) { documents[it].title },
        Array(documents.size) { documents[it].body },
      )
      stringToInternalId.clear()
      stringToInternalId.putAll(newForward)
      internalIdToString.clear()
      internalIdToString.putAll(newReverse)
    }

  override fun close() {
    lock.write {
      val h = handle.getAndSet(0L)
      if (h != 0L) {
        nativeDestroy(h)
        stringToInternalId.clear()
        internalIdToString.clear()
      }
    }
  }

  private inline fun <T> withRead(block: (Long) -> T): T =
    lock.read {
      val h = handle.get()
      check(h != 0L) { "SearchEngine is closed" }
      block(h)
    }

  private inline fun <T> withWrite(block: (Long) -> T): T =
    lock.write {
      val h = handle.get()
      check(h != 0L) { "SearchEngine is closed" }
      block(h)
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

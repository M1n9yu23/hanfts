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

/**
 * A document to be indexed by [SearchEngine].
 *
 * @param id    Unique string identifier (e.g. UUID, slug, primary key). Indexing a document
 *              with an existing [id] replaces it. IDs are compared by [String.equals]/[String.hashCode],
 *              so Unicode-distinct strings (e.g. precomposed `"é"` vs decomposed `"é"`) are
 *              treated as different IDs — normalize on the caller side if needed.
 * @param title Short descriptive text, weighted 3× higher than [body] in relevance scoring.
 * @param body  Main content of the document.
 */
data class Document(
  val id: String,
  val title: String,
  val body: String,
)

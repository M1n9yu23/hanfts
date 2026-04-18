<div align="center">

# hanfts

**Native Full-Text Search for Android — Korean & English, zero dependencies, pure in-memory.**

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![Maven Central](https://img.shields.io/maven-central/v/io.github.m1n9yu23/hanfts.svg?label=Maven%20Central)](https://search.maven.org/artifact/io.github.m1n9yu23/hanfts)
[![API](https://img.shields.io/badge/API-21%2B-brightgreen.svg)](https://android-arsenal.com/api?level=21)
[![Kotlin](https://img.shields.io/badge/Kotlin-2.x-7F52FF.svg)](https://kotlinlang.org)
[![C++17](https://img.shields.io/badge/C++-17-00599C.svg)](https://isocpp.org)

**[한국어](README-ko.md)**

<img src="assets/sample_demo.gif" width="300" alt="hanfts demo">

</div>

## Overview

hanfts is an Android full-text search library that supports both Korean and English. It uses a bigram tokenizer implemented in C++17 to enable partial Korean search.

## Background

With Room's default setup, searching `"산책"` does not match a document containing `"오늘산책했다"`.

<div align="center">
  <img src="assets/img1.png" width="260">
  <img src="assets/img2.png" width="260">
  <img src="assets/img3.png" width="260">
</div>

hanfts provides an alternative that works out of the box, tokenizing Korean text into overlapping bigrams at the Unicode codepoint level without a morphological analyzer.

## Features

- **Korean bigram tokenization** — overlapping 2-grams at the Unicode codepoint level; no morphological analyzer required
- **English word tokenization** — case-insensitive, whitespace-delimited tokens
- **Mixed-language support** — Korean and English handled independently in the same document
- **Prefix search** — typing `"산"` matches `"산책"`, `"산보"` and more, in real time
- **TF-IDF ranking** — relevance scoring; title weighted 3× over body
- **Thread-safe** — concurrent reads via `std::shared_mutex`; writes are exclusive
- **Pure in-memory** — no file I/O, no Android `Context`, no permissions
- **Zero native dependencies** — pure C++17 STL only
- **Closeable** — deterministic native memory release via `close()` or `use { }`

## Installation

hanfts is available on Maven Central. No extra repository configuration needed.

```kotlin
dependencies {
    implementation("io.github.m1n9yu23:hanfts:<version>")
}
```

> Replace `<version>` with the [latest release](https://github.com/M1n9yu23/hanfts/releases).

## Quick Start

### Create and close

```kotlin
val engine = SearchEngine()

engine.close()

SearchEngine().use { engine ->
    // ...
}
```

### Build the index

```kotlin
engine.rebuildIndex(
    listOf(
        Document(id = 1L, title = "오늘의 산책", body = "날씨가 맑아서 공원을 걸었다."),
        Document(id = 2L, title = "Morning Walk", body = "The park was quiet and peaceful."),
    )
)

engine.indexDocument(id = 3L, title = "제목", body = "본문 내용")
engine.indexDocument(Document(id = 4L, title = "Title", body = "Body text"))
```

### Search

```kotlin
val results = engine.search("산책")
// → [SearchResult(id=1, score=1.82f)]

val results = engine.search("산책", limit = 5)
```

### Update and remove

```kotlin
engine.indexDocument(id = 1L, title = "updated title", body = "updated body")

engine.removeDocument(id = 1L)

engine.clear()
```

### Example with ViewModel

The engine can be integrated into any architecture. Here is an example using `ViewModel`.

```kotlin
class SearchViewModel : ViewModel() {

    private val engine = SearchEngine()

    init {
        viewModelScope.launch(Dispatchers.Default) {
            engine.rebuildIndex(/* your documents */)
        }
    }

    fun search(query: String): List<SearchResult> =
        engine.search(query)

    override fun onCleared() {
        super.onCleared()
        engine.close()
    }
}
```

## API Reference

### `SearchEngine`

Obtain an instance via the factory function:

```kotlin
val engine = SearchEngine()
```

| Method / Property | Description |
|---|---|
| `documentCount: Int` | Number of documents currently in the index. |
| `indexDocument(id, title, body)` | Adds or replaces a document. Replaces if `id` already exists. |
| `indexDocument(document: Document)` | Convenience overload accepting a `Document`. |
| `removeDocument(id: Long)` | Removes a document by ID. No-op if not found. |
| `clear()` | Removes all documents from the index. |
| `search(query, limit = 20)` | Returns results ranked by TF-IDF score, descending. Blank query returns empty list. |
| `rebuildIndex(documents: List<Document>)` | Clears and rebuilds the entire index atomically. |
| `close()` | Releases native memory. Do not use the engine after closing. |

> **Threading:** all mutating operations (`indexDocument`, `removeDocument`, `rebuildIndex`, `clear`) acquire an internal lock and are blocking. Run them on `Dispatchers.Default`. Concurrent `search()` calls are always safe.

### `Document`

```kotlin
data class Document(
    val id: Long,
    val title: String,
    val body: String,
)
```

### `SearchResult`

```kotlin
data class SearchResult(
    val id: Long,
    val score: Float,
)
```

## How It Works

### Tokenizer

| Input | Language | Tokens |
|---|---|---|
| `"오늘산책"` | Korean | `["오늘", "늘산", "산책", "오늘산책"]` |
| `"오늘산책했다"` | Korean (5+ chars) | `["오늘", "늘산", "산책", "책했", "했다"]` |
| `"Good morning"` | English | `["good", "morning"]` |
| `"meeting 미팅"` | Mixed | `["meeting", "미팅"]` |

Korean text is split into overlapping bigrams at the Unicode codepoint level. Words of **4 codepoints or fewer** also emit the full word as an additional token to improve exact-match recall.

Korean Unicode ranges covered:

```
U+AC00–U+D7A3  Hangul Syllables  (가–힣)
U+1100–U+11FF  Hangul Jamo
U+3130–U+318F  Hangul Compatibility Jamo
U+A960–U+A97F  Hangul Jamo Extended-A
U+D7B0–U+D7FF  Hangul Jamo Extended-B
```

### Scoring

```
TF    = term frequency in document / total weighted terms in document
IDF   = log((N + 1) / (df + 1)) + 1    (smoothed — always ≥ 1)
Score = TF × IDF
```

`N` = total document count, `df` = number of documents containing the term.  
Title tokens count **3×** relative to body tokens.

### Prefix Search

`std::map::lower_bound(token)` locates the first matching term; the iterator advances while the term starts with the query token. A cap of **64 matching terms per token** prevents single-character queries from expanding excessively.

### Architecture

```
SearchEngine            ← Kotlin public interface (factory: SearchEngine())
    │
    │  JNI — libhanfts.so
    ▼
NativeSearchEngine      ← internal Kotlin class, AtomicLong handle
    │
    ▼
fts::SearchEngine       ← C++ API + std::shared_mutex concurrency
    ├── fts::InvertedIndex  — TF-IDF posting lists, prefix scan via std::map
    └── fts::Tokenizer      — UTF-8 → codepoints → bigrams / words
```

`NativeSearchEngine` is `internal` — users interact only with the `SearchEngine` interface. The C++ object is managed as an opaque `jlong` handle via `AtomicLong`; `close()` uses `getAndSet(0)` to prevent double-free in concurrent scenarios.

## Notes

- **Index is in-memory only.** The index is not persisted to disk. Call `rebuildIndex()` each time the app starts, typically in a ViewModel `init` block on `Dispatchers.Default`.
- **Always call `close()`.** The engine holds native memory. Tie its lifecycle to a `ViewModel` (`onCleared`) or use the `use { }` block to ensure it is released.
- **Bigram tokenization has trade-offs.** Single-character queries may match a large number of documents. The prefix scan is capped at 64 terms per token to keep results manageable.
- **Mutating operations are blocking.** `rebuildIndex`, `indexDocument`, `removeDocument`, and `clear` acquire an exclusive write lock. Do not call them on the main thread.
- **`search()` is safe to call concurrently.** Multiple threads can search at the same time without additional synchronization.

## Requirements

- Android **minSdk 21** (Android 5.0 Lollipop)
- NDK **r25+**
- CMake **3.22.1+**
- C++17

ABI targets: `arm64-v8a`, `armeabi-v7a`, `x86_64`

## References

- **[Introduction to Information Retrieval](https://nlp.stanford.edu/IR-book/)** — Theoretical basis for the inverted index, TF-IDF scoring, and the smoothed IDF variant (`log((N+1)/(df+1)) + 1`).
- **[RFC 3629 — UTF-8, a transformation format of ISO 10646](https://www.rfc-editor.org/rfc/rfc3629)** — Reference specification for the manual 1–4 byte UTF-8 decoder.
- **[Unicode Standard — Hangul Blocks](https://www.unicode.org/charts/)** — Definition of Korean Unicode block ranges (Syllables U+AC00–U+D7A3, Jamo, Compatibility Jamo, Extended blocks).
- **[Android NDK — JNI Tips](https://developer.android.com/training/articles/perf-jni)** — Pointer-handle pattern, `DeleteLocalRef` management, and `JNI_ABORT` flag.
- **[cppreference — std::shared_mutex](https://en.cppreference.com/w/cpp/thread/shared_mutex)** — Readers-writer lock implementation (C++17).

## Contributing

All contributions are welcome — bug fixes, performance improvements, new language support, or documentation.

See [CONTRIBUTING.md](CONTRIBUTING.md) for details.

## License

```
Copyright 2026 Gyugle

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
```

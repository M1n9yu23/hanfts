package com.gyugle.hanfts

/**
 * A single result returned by [SearchEngine.search].
 *
 * @param id    The document identifier supplied at index time.
 * @param score TF-IDF relevance score. Higher values indicate a closer match.
 */
data class SearchResult(
    val id: Int,
    val score: Float,
)

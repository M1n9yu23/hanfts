package com.gyugle.hanfts

/**
 * A full-text search engine that indexes and queries documents by integer ID.
 *
 * Documents consist of a numeric [id], a [title] (boosted 3×), and a [body].
 * Supports both Korean (bigram tokenization) and ASCII (word-level) text.
 */
interface SearchEngine {

    /**
     * Adds or replaces a document in the index.
     * If a document with the same [id] already exists it is removed first.
     */
    fun indexDocument(id: Int, title: String, body: String)

    /** Removes the document with the given [id] from the index. */
    fun removeDocument(id: Int)

    /**
     * Searches indexed documents matching [query].
     *
     * @param query Search text (Korean or ASCII).
     * @param limit Maximum number of results to return. Defaults to 20.
     * @return Document IDs ordered by relevance score, best match first.
     */
    fun search(query: String, limit: Int = 20): List<Int>

    /**
     * Clears the entire index and rebuilds it from [documents].
     *
     * @param documents List of (id, title, body) triples to index.
     */
    fun rebuildIndex(documents: List<Triple<Int, String, String>>)
}

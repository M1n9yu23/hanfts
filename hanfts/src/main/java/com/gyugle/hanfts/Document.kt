package com.gyugle.hanfts

/**
 * A document to be indexed by [SearchEngine].
 *
 * @param id    Unique integer identifier. Indexing a document with an existing [id] replaces it.
 * @param title Short descriptive text, weighted 3× higher than [body] in relevance scoring.
 * @param body  Main content of the document.
 */
data class Document(
    val id: Int,
    val title: String,
    val body: String,
)

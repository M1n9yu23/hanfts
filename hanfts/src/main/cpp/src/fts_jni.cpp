#include "search_engine.h"

#include <jni.h>
#include <string>
#include <vector>

/**
 * Converts a JNI jstring to a std::string using the modified UTF-8 encoding
 * that the JVM uses internally. Returns an empty string if @p js is null.
 */
static std::string jstringToStdString(JNIEnv *env, jstring js) {
    if (!js) return "";
    const char *chars = env->GetStringUTFChars(js, nullptr);
    std::string result(chars);
    env->ReleaseStringUTFChars(js, chars);
    return result;
}

/**
 * Casts the opaque jlong handle back to a SearchEngine pointer.
 * The handle is created in nativeCreate and owned by the Kotlin side until
 * nativeDestroy is called.
 */
static fts::SearchEngine *toEngine(jlong handle) {
    return reinterpret_cast<fts::SearchEngine *>(handle);
}

extern "C" {

/**
 * Creates a new SearchEngine at the given index path and returns an opaque
 * handle (pointer cast to jlong) to the Kotlin caller.
 * Returns 0 on failure so the Kotlin side can throw IllegalStateException.
 */
JNIEXPORT jlong JNICALL
Java_com_gyugle_hanfts_NativeSearchEngine_nativeCreate(
        JNIEnv *env, jobject, jstring index_path) {
    try {
        auto path    = jstringToStdString(env, index_path);
        auto *engine = new fts::SearchEngine(path);
        return reinterpret_cast<jlong>(engine);
    } catch (...) {
        return 0L;
    }
}

/**
 * Destroys the SearchEngine and frees the native heap allocation.
 * Must be called exactly once per handle obtained from nativeCreate.
 */
JNIEXPORT void JNICALL
Java_com_gyugle_hanfts_NativeSearchEngine_nativeDestroy(
        JNIEnv *, jobject, jlong handle) {
    delete toEngine(handle);
}

/** Indexes or replaces a document identified by @p doc_id. */
JNIEXPORT void JNICALL
Java_com_gyugle_hanfts_NativeSearchEngine_nativeIndexDocument(
        JNIEnv *env, jobject, jlong handle,
        jint doc_id, jstring title, jstring body) {
    auto *engine = toEngine(handle);
    if (!engine) return;
    try {
        engine->indexDocument(
                static_cast<int>(doc_id),
                jstringToStdString(env, title),
                jstringToStdString(env, body));
    } catch (...) {}
}

/** Removes the document with the given @p doc_id from the index. */
JNIEXPORT void JNICALL
Java_com_gyugle_hanfts_NativeSearchEngine_nativeRemoveDocument(
        JNIEnv *, jobject, jlong handle, jint doc_id) {
    auto *engine = toEngine(handle);
    if (!engine) return;
    try {
        engine->removeDocument(static_cast<int>(doc_id));
    } catch (...) {}
}

/**
 * Searches the index and returns matching document IDs as a jintArray.
 * Returns an empty array (never null) so the Kotlin side can safely call
 * .toList() without a null check.
 */
JNIEXPORT jintArray JNICALL
Java_com_gyugle_hanfts_NativeSearchEngine_nativeSearch(
        JNIEnv *env, jobject, jlong handle,
        jstring query, jint limit) {
    auto *engine = toEngine(handle);
    if (!engine) return env->NewIntArray(0);
    try {
        auto ids = engine->search(
                jstringToStdString(env, query),
                static_cast<int>(limit));

        jintArray result = env->NewIntArray(static_cast<jsize>(ids.size()));
        if (!result) return env->NewIntArray(0);
        env->SetIntArrayRegion(result, 0,
                               static_cast<jsize>(ids.size()),
                               reinterpret_cast<const jint *>(ids.data()));
        return result;
    } catch (...) {
        return env->NewIntArray(0);
    }
}

/**
 * Clears the index and rebuilds it from parallel arrays of IDs, titles, and bodies.
 *
 * JNI_ABORT is passed to ReleaseIntArrayElements because we only read the
 * array and do not write back any changes.
 * Local references for each title/body string are released inside the loop to
 * avoid exhausting the JNI local reference table on large document sets.
 */
JNIEXPORT void JNICALL
Java_com_gyugle_hanfts_NativeSearchEngine_nativeRebuildIndex(
        JNIEnv *env, jobject, jlong handle,
        jintArray ids, jobjectArray titles, jobjectArray bodies) {
    auto *engine = toEngine(handle);
    if (!engine) return;
    try {
        jsize  len    = env->GetArrayLength(ids);
        jint  *id_arr = env->GetIntArrayElements(ids, nullptr);
        if (!id_arr) return;

        std::vector<std::tuple<int, std::string, std::string>> docs;
        docs.reserve(static_cast<size_t>(len));

        for (jsize i = 0; i < len; ++i) {
            auto title_js = static_cast<jstring>(env->GetObjectArrayElement(titles, i));
            auto body_js  = static_cast<jstring>(env->GetObjectArrayElement(bodies, i));
            docs.emplace_back(
                    static_cast<int>(id_arr[i]),
                    jstringToStdString(env, title_js),
                    jstringToStdString(env, body_js));
            env->DeleteLocalRef(title_js);
            env->DeleteLocalRef(body_js);
        }

        env->ReleaseIntArrayElements(ids, id_arr, JNI_ABORT);
        engine->rebuildIndex(docs);
    } catch (...) {}
}

}

#include "search_engine.h"

#include <jni.h>
#include <string>
#include <vector>
#include <cstring>
#include <stdexcept>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::string jstringToStdString(JNIEnv* env, jstring js) {
    if (!js) return "";
    const char* chars = env->GetStringUTFChars(js, nullptr);
    if (!chars) return "";
    std::string result(chars);
    env->ReleaseStringUTFChars(js, chars);
    return result;
}

static fts::SearchEngine* toEngine(jlong handle) {
    return reinterpret_cast<fts::SearchEngine*>(handle);
}

static void throwException(JNIEnv* env, const char* className, const char* msg) {
    jclass cls = env->FindClass(className);
    if (cls) env->ThrowNew(cls, msg);
}

static void throwIllegalState(JNIEnv* env, const char* msg) {
    throwException(env, "java/lang/IllegalStateException", msg);
}

static void throwRuntimeException(JNIEnv* env, const char* msg) {
    throwException(env, "java/lang/RuntimeException", msg);
}

// ---------------------------------------------------------------------------
// JNI functions
// ---------------------------------------------------------------------------

extern "C" {

JNIEXPORT jlong JNICALL
Java_com_gyugle_hanfts_NativeSearchEngine_nativeCreate(
        JNIEnv* env, jobject) {
    try {
        return reinterpret_cast<jlong>(new fts::SearchEngine());
    } catch (const std::exception& e) {
        throwRuntimeException(env, e.what());
        return 0L;
    } catch (...) {
        throwRuntimeException(env, "Unknown error during engine creation");
        return 0L;
    }
}

JNIEXPORT void JNICALL
Java_com_gyugle_hanfts_NativeSearchEngine_nativeDestroy(
        JNIEnv*, jobject, jlong handle) {
    delete toEngine(handle);
}

JNIEXPORT jint JNICALL
Java_com_gyugle_hanfts_NativeSearchEngine_nativeDocumentCount(
        JNIEnv* env, jobject, jlong handle) {
    auto* engine = toEngine(handle);
    if (!engine) { throwIllegalState(env, "SearchEngine is closed"); return 0; }
    try {
        return static_cast<jint>(engine->documentCount());
    } catch (const std::exception& e) {
        throwRuntimeException(env, e.what());
        return 0;
    }
}

JNIEXPORT void JNICALL
Java_com_gyugle_hanfts_NativeSearchEngine_nativeIndexDocument(
        JNIEnv* env, jobject, jlong handle,
        jint doc_id, jstring title, jstring body) {
    auto* engine = toEngine(handle);
    if (!engine) { throwIllegalState(env, "SearchEngine is closed"); return; }
    try {
        engine->indexDocument(
                static_cast<int>(doc_id),
                jstringToStdString(env, title),
                jstringToStdString(env, body));
    } catch (const std::exception& e) {
        throwRuntimeException(env, e.what());
    }
}

JNIEXPORT void JNICALL
Java_com_gyugle_hanfts_NativeSearchEngine_nativeClear(
        JNIEnv* env, jobject, jlong handle) {
    auto* engine = toEngine(handle);
    if (!engine) { throwIllegalState(env, "SearchEngine is closed"); return; }
    try {
        engine->clear();
    } catch (const std::exception& e) {
        throwRuntimeException(env, e.what());
    }
}

JNIEXPORT void JNICALL
Java_com_gyugle_hanfts_NativeSearchEngine_nativeRemoveDocument(
        JNIEnv* env, jobject, jlong handle, jint doc_id) {
    auto* engine = toEngine(handle);
    if (!engine) { throwIllegalState(env, "SearchEngine is closed"); return; }
    try {
        engine->removeDocument(static_cast<int>(doc_id));
    } catch (const std::exception& e) {
        throwRuntimeException(env, e.what());
    }
}

/**
 * Returns a packed IntArray of size `2 * resultCount` where each pair is:
 *   [doc_id, Float.floatToRawIntBits(score)]
 */
JNIEXPORT jintArray JNICALL
Java_com_gyugle_hanfts_NativeSearchEngine_nativeSearch(
        JNIEnv* env, jobject, jlong handle,
        jstring query, jint limit) {
    auto* engine = toEngine(handle);
    if (!engine) { throwIllegalState(env, "SearchEngine is closed"); return env->NewIntArray(0); }
    try {
        auto results = engine->search(
                jstringToStdString(env, query),
                static_cast<int>(limit));

        jsize packed_len = static_cast<jsize>(results.size() * 2);
        jintArray packed = env->NewIntArray(packed_len);
        if (!packed) return env->NewIntArray(0);

        std::vector<jint> data;
        data.reserve(static_cast<size_t>(packed_len));
        for (const auto& r : results) {
            data.push_back(static_cast<jint>(r.doc_id));
            jint score_bits = 0;
            static_assert(sizeof(float) == sizeof(jint), "float/jint size mismatch");
            std::memcpy(&score_bits, &r.score, sizeof(float));
            data.push_back(score_bits);
        }
        env->SetIntArrayRegion(packed, 0, packed_len, data.data());
        return packed;
    } catch (const std::exception& e) {
        throwRuntimeException(env, e.what());
        return env->NewIntArray(0);
    }
}

JNIEXPORT void JNICALL
Java_com_gyugle_hanfts_NativeSearchEngine_nativeRebuildIndex(
        JNIEnv* env, jobject, jlong handle,
        jintArray ids, jobjectArray titles, jobjectArray bodies) {
    auto* engine = toEngine(handle);
    if (!engine) { throwIllegalState(env, "SearchEngine is closed"); return; }
    try {
        jsize len        = env->GetArrayLength(ids);
        jsize titles_len = env->GetArrayLength(titles);
        jsize bodies_len = env->GetArrayLength(bodies);

        if (titles_len != len || bodies_len != len) {
            throwException(env, "java/lang/IllegalArgumentException",
                           "ids, titles, and bodies arrays must have the same length");
            return;
        }

        jint* id_arr = env->GetIntArrayElements(ids, nullptr);
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
    } catch (const std::exception& e) {
        throwRuntimeException(env, e.what());
    }
}

} // extern "C"

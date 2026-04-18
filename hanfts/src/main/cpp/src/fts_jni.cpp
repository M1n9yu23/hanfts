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
        jlong doc_id, jstring title, jstring body) {
    auto* engine = toEngine(handle);
    if (!engine) { throwIllegalState(env, "SearchEngine is closed"); return; }
    try {
        engine->indexDocument(
                static_cast<int64_t>(doc_id),
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
        JNIEnv* env, jobject, jlong handle, jlong doc_id) {
    auto* engine = toEngine(handle);
    if (!engine) { throwIllegalState(env, "SearchEngine is closed"); return; }
    try {
        engine->removeDocument(static_cast<int64_t>(doc_id));
    } catch (const std::exception& e) {
        throwRuntimeException(env, e.what());
    }
}

/**
 * Returns a packed LongArray of size `2 * resultCount` where each pair is:
 *   [doc_id, (jlong) Float.floatToRawIntBits(score)]
 */
JNIEXPORT jlongArray JNICALL
Java_com_gyugle_hanfts_NativeSearchEngine_nativeSearch(
        JNIEnv* env, jobject, jlong handle,
        jstring query, jint limit) {
    auto* engine = toEngine(handle);
    if (!engine) { throwIllegalState(env, "SearchEngine is closed"); return env->NewLongArray(0); }
    try {
        auto results = engine->search(
                jstringToStdString(env, query),
                static_cast<int>(limit));

        jsize packed_len = static_cast<jsize>(results.size() * 2);
        jlongArray packed = env->NewLongArray(packed_len);
        if (!packed) return env->NewLongArray(0);

        std::vector<jlong> data;
        data.reserve(static_cast<size_t>(packed_len));
        for (const auto& r : results) {
            data.push_back(static_cast<jlong>(r.doc_id));
            uint32_t score_bits = 0;
            static_assert(sizeof(float) == sizeof(uint32_t), "float/uint32_t size mismatch");
            std::memcpy(&score_bits, &r.score, sizeof(float));
            data.push_back(static_cast<jlong>(score_bits));
        }
        env->SetLongArrayRegion(packed, 0, packed_len, data.data());
        return packed;
    } catch (const std::exception& e) {
        throwRuntimeException(env, e.what());
        return env->NewLongArray(0);
    }
}

JNIEXPORT void JNICALL
Java_com_gyugle_hanfts_NativeSearchEngine_nativeRebuildIndex(
        JNIEnv* env, jobject, jlong handle,
        jlongArray ids, jobjectArray titles, jobjectArray bodies) {
    auto* engine = toEngine(handle);
    if (!engine) { throwIllegalState(env, "SearchEngine is closed"); return; }

    jsize len        = env->GetArrayLength(ids);
    jsize titles_len = env->GetArrayLength(titles);
    jsize bodies_len = env->GetArrayLength(bodies);

    if (titles_len != len || bodies_len != len) {
        throwException(env, "java/lang/IllegalArgumentException",
                       "ids, titles, and bodies arrays must have the same length");
        return;
    }

    jlong* id_arr = env->GetLongArrayElements(ids, nullptr);
    if (!id_arr) return;

    struct Guard {
        JNIEnv* env; jlongArray ids; jlong* ptr;
        ~Guard() { env->ReleaseLongArrayElements(ids, ptr, JNI_ABORT); }
    } guard{env, ids, id_arr};

    try {
        std::vector<std::tuple<int64_t, std::string, std::string>> docs;
        docs.reserve(static_cast<size_t>(len));

        for (jsize i = 0; i < len; ++i) {
            auto title_js = static_cast<jstring>(env->GetObjectArrayElement(titles, i));
            auto body_js  = static_cast<jstring>(env->GetObjectArrayElement(bodies, i));
            docs.emplace_back(
                    static_cast<int64_t>(id_arr[i]),
                    jstringToStdString(env, title_js),
                    jstringToStdString(env, body_js));
            env->DeleteLocalRef(title_js);
            env->DeleteLocalRef(body_js);
        }

        engine->rebuildIndex(docs);
    } catch (const std::exception& e) {
        throwRuntimeException(env, e.what());
    }
}

} // extern "C"

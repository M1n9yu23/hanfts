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
#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace fts {

/**
 * Tokenizer for Korean and ASCII text.
 *
 * Korean words are split into overlapping bigrams so that partial-word queries
 * still match. For example, "안녕하세요" produces the tokens {"안녕", "녕하", "하세", "세요"}.
 * Short words (≤ 4 codepoints) also emit the full word as an extra token to
 * improve exact-match precision.
 *
 * ASCII words are lowercased and emitted as a single token.
 *
 * All input is expected to be valid UTF-8. Ill-formed byte sequences are
 * skipped silently.
 */
class Tokenizer {
public:
    /**
     * Tokenizes @p utf8_text and returns a deduplicated list of tokens.
     * The order of tokens reflects the order of first occurrence in the text.
     */
    static std::vector<std::string> tokenize(const std::string &utf8_text);

private:
    /** Decodes a UTF-8 string into a sequence of Unicode codepoints. */
    static std::vector<uint32_t> toCodepoints(const std::string &utf8);

    /** Encodes a single Unicode codepoint back to a UTF-8 string. */
    static std::string codepointToUtf8(uint32_t cp);

    /** Encodes a slice of codepoints [start, start+len) to UTF-8. */
    static std::string
    codepointsToUtf8(const std::vector<uint32_t> &cps, size_t start, size_t len);

    /** Returns true if @p cp belongs to any Korean Unicode block. */
    static bool isKorean(uint32_t cp);

    static bool isAsciiAlpha(uint32_t cp);
    static bool isAsciiDigit(uint32_t cp);

    /** Returns true if @p cp is a character that can form a word token. */
    static bool isWordChar(uint32_t cp);

    /** Lowercases ASCII letters; leaves all other codepoints unchanged. */
    static uint32_t toLowerAscii(uint32_t cp);

    /**
     * Appends bigram tokens (and optionally the full word) from @p word into @p out.
     * Called for Korean and non-ASCII word runs.
     */
    static void addBigrams(const std::vector<uint32_t> &word, std::vector<std::string> &out);
};

}

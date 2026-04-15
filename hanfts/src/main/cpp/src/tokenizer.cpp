#include "tokenizer.h"
#include <algorithm>
#include <unordered_set>

namespace fts {

/**
 * Decodes a UTF-8 string into a vector of Unicode codepoints.
 * Ill-formed sequences (unexpected continuation bytes, truncated multi-byte
 * sequences, etc.) are skipped by advancing one byte at a time.
 */
std::vector<uint32_t> Tokenizer::toCodepoints(const std::string &utf8) {
    std::vector<uint32_t> cps;
    cps.reserve(utf8.size());
    size_t i = 0;
    while (i < utf8.size()) {
        auto c = static_cast<uint8_t>(utf8[i]);
        uint32_t cp;
        if (c < 0x80) {                                          // 1-byte (U+0000–U+007F)
            cp = c;
            i += 1;
        } else if ((c & 0xE0) == 0xC0 && i + 1 < utf8.size()) { // 2-byte (U+0080–U+07FF)
            cp = static_cast<uint32_t>(c & 0x1F) << 6 |
                 static_cast<uint32_t>(static_cast<uint8_t>(utf8[i + 1]) & 0x3F);
            i += 2;
        } else if ((c & 0xF0) == 0xE0 && i + 2 < utf8.size()) { // 3-byte (U+0800–U+FFFF)
            cp = static_cast<uint32_t>(c & 0x0F) << 12 |
                 static_cast<uint32_t>(static_cast<uint8_t>(utf8[i + 1]) & 0x3F) << 6 |
                 static_cast<uint32_t>(static_cast<uint8_t>(utf8[i + 2]) & 0x3F);
            i += 3;
        } else if ((c & 0xF8) == 0xF0 && i + 3 < utf8.size()) { // 4-byte (U+10000–U+10FFFF)
            cp = static_cast<uint32_t>(c & 0x07) << 18 |
                 static_cast<uint32_t>(static_cast<uint8_t>(utf8[i + 1]) & 0x3F) << 12 |
                 static_cast<uint32_t>(static_cast<uint8_t>(utf8[i + 2]) & 0x3F) << 6 |
                 static_cast<uint32_t>(static_cast<uint8_t>(utf8[i + 3]) & 0x3F);
            i += 4;
        } else {
            i += 1; // Skip ill-formed byte.
            continue;
        }
        cps.push_back(cp);
    }
    return cps;
}

/** Encodes a single Unicode codepoint to its UTF-8 byte sequence. */
std::string Tokenizer::codepointToUtf8(uint32_t cp) {
    std::string s;
    if (cp < 0x80) {
        s += static_cast<char>(cp);
    } else if (cp < 0x800) {
        s += static_cast<char>(0xC0 | (cp >> 6));
        s += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        s += static_cast<char>(0xE0 | (cp >> 12));
        s += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        s += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        s += static_cast<char>(0xF0 | (cp >> 18));
        s += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        s += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        s += static_cast<char>(0x80 | (cp & 0x3F));
    }
    return s;
}

/** Encodes the codepoints in [start, start+len) to a UTF-8 string. */
std::string Tokenizer::codepointsToUtf8(const std::vector<uint32_t> &cps,
                                        size_t start, size_t len) {
    std::string result;
    for (size_t i = start; i < start + len && i < cps.size(); ++i) {
        result += codepointToUtf8(cps[i]);
    }
    return result;
}

/**
 * Returns true if @p cp belongs to a Korean Unicode block:
 *   AC00–D7A3  Hangul Syllables (precomposed)
 *   1100–11FF  Hangul Jamo
 *   3130–318F  Hangul Compatibility Jamo
 *   A960–A97F  Hangul Jamo Extended-A
 *   D7B0–D7FF  Hangul Jamo Extended-B
 */
bool Tokenizer::isKorean(uint32_t cp) {
    return (cp >= 0xAC00 && cp <= 0xD7A3) ||
           (cp >= 0x1100 && cp <= 0x11FF) ||
           (cp >= 0x3130 && cp <= 0x318F) ||
           (cp >= 0xA960 && cp <= 0xA97F) ||
           (cp >= 0xD7B0 && cp <= 0xD7FF);
}

bool Tokenizer::isAsciiAlpha(uint32_t cp) {
    return (cp >= 'a' && cp <= 'z') || (cp >= 'A' && cp <= 'Z');
}

bool Tokenizer::isAsciiDigit(uint32_t cp) {
    return cp >= '0' && cp <= '9';
}

bool Tokenizer::isWordChar(uint32_t cp) {
    return isKorean(cp) || isAsciiAlpha(cp) || isAsciiDigit(cp);
}

/** Maps ASCII uppercase letters to lowercase; all other codepoints pass through. */
uint32_t Tokenizer::toLowerAscii(uint32_t cp) {
    if (cp >= 'A' && cp <= 'Z') return cp + 32;
    return cp;
}

/**
 * Generates bigram tokens from @p word and appends them to @p out.
 *
 * Strategy:
 *  - A single-codepoint word emits itself as-is.
 *  - A longer word emits every consecutive pair of codepoints as a bigram.
 *  - Short words (≤ 4 codepoints) additionally emit the whole word to boost
 *    exact-match recall for short queries.
 */
void Tokenizer::addBigrams(const std::vector<uint32_t> &word,
                           std::vector<std::string> &out) {
    if (word.empty()) return;

    if (word.size() == 1) {
        out.push_back(codepointsToUtf8(word, 0, 1));
        return;
    }

    for (size_t i = 0; i + 1 < word.size(); ++i) {
        out.push_back(codepointsToUtf8(word, i, 2));
    }
    if (word.size() <= 4) {
        // Also index the full word so short exact queries rank higher.
        out.push_back(codepointsToUtf8(word, 0, word.size()));
    }
}

/**
 * Tokenizes @p utf8_text into a deduplicated list of index tokens.
 *
 * Processing steps:
 *  1. Decode the input to Unicode codepoints.
 *  2. Split on non-word characters to extract word runs.
 *  3. Lowercase ASCII characters within each run.
 *  4. Korean runs → bigrams (via addBigrams).
 *     ASCII/digit runs → single lowercased token.
 *  5. Deduplicate while preserving first-occurrence order.
 */
std::vector<std::string> Tokenizer::tokenize(const std::string &utf8_text) {
    const auto cps = toCodepoints(utf8_text);

    std::vector<std::string> tokens;
    tokens.reserve(cps.size());

    size_t i = 0;
    while (i < cps.size()) {
        if (!isWordChar(cps[i])) {
            ++i;
            continue;
        }

        // Collect a contiguous word run.
        size_t start = i;
        while (i < cps.size() && isWordChar(cps[i])) {
            ++i;
        }

        std::vector<uint32_t> word(cps.begin() + static_cast<ptrdiff_t>(start),
                                   cps.begin() + static_cast<ptrdiff_t>(i));

        // Normalize ASCII case within the run.
        for (auto &cp: word) {
            cp = toLowerAscii(cp);
        }

        bool hasKorean = false;
        bool hasAscii  = false;
        for (auto cp: word) {
            if (isKorean(cp))                    hasKorean = true;
            if (isAsciiAlpha(cp) || isAsciiDigit(cp)) hasAscii  = true;
        }

        if (hasKorean) {
            // Korean (possibly mixed) → bigram tokenization.
            addBigrams(word, tokens);
        } else if (hasAscii) {
            // Pure ASCII/digit run → single lowercased token.
            tokens.push_back(codepointsToUtf8(word, 0, word.size()));
        } else {
            // Other scripts (CJK, etc.) → bigram tokenization.
            addBigrams(word, tokens);
        }
    }

    // Deduplicate tokens while preserving their first-occurrence order.
    std::unordered_set<std::string> seen;
    std::vector<std::string> result;
    result.reserve(tokens.size());
    for (auto &t: tokens) {
        if (!t.empty() && seen.insert(t).second) {
            result.push_back(std::move(t));
        }
    }
    return result;
}

}

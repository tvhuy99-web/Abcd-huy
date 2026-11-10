package com.cleverraven.cataclysmdda;

import java.util.regex.Pattern;

/**
 * Final speech-only cleanup for game color markup. Keep actual color words,
 * item names, numbers, punctuation and user data: their meaning cannot be
 * inferred from the color code. Source-aware game adapters supply semantic
 * descriptions of visual state before this final cleanup.
 */
final class ReaderSpokenText {
    private static final Pattern COLOR_TAG = Pattern.compile(
            "(?i)</?color(?:_[a-z0-9_]+)?>");

    private ReaderSpokenText() { }

    static String clean(String text) {
        return text == null ? "" : COLOR_TAG.matcher(text).replaceAll("");
    }
}

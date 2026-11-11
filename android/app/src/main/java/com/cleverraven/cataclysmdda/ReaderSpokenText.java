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
    // Separately joined status sentences can create "5/5..". Remove
    // that stray second full stop, but preserve intentional ellipses
    // (...), decimal points, names, numbers and all substantive text.
    private static final Pattern DUPLICATE_STOP = Pattern.compile(
            "(?<!\\.)\\.{2}(?!\\.)");

    private ReaderSpokenText() { }

    static String clean(String text) {
        if (text == null) { return ""; }
        return DUPLICATE_STOP.matcher(COLOR_TAG.matcher(text).replaceAll(""))
                .replaceAll(".");
    }
}

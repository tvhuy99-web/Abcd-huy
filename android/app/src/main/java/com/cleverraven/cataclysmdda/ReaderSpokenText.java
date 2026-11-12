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

    // The engine renders armor coverage with ASCII silhouettes, often
    // dozens of rows consisting entirely of "###  ####". This is a
    // visual illustration, not item data. Keep every real stat, narrative
    // and number, including names or text containing a meaningful '#'.
    private static final Pattern ASCII_ARMOR_DIAGRAM_LINE = Pattern.compile(
            "(?m)^[ \t]*(?:#[ \t]*){2,}\r?$");
    private static final Pattern EXCESS_DIAGRAM_GAPS = Pattern.compile("\n{3,}");

    private ReaderSpokenText() { }

    static String clean(String text) {
        if (text == null) { return ""; }
        String cleaned = COLOR_TAG.matcher(text).replaceAll("");
        if (cleaned.indexOf('#') >= 0) {
            String withoutArt = ASCII_ARMOR_DIAGRAM_LINE.matcher(cleaned).replaceAll("");
            if (!withoutArt.equals(cleaned)) {
                // Preserve section breaks without a long silence for each
                // removed row of the visual silhouette.
                cleaned = EXCESS_DIAGRAM_GAPS.matcher(withoutArt).replaceAll("\n\n");
            }
        }
        return DUPLICATE_STOP.matcher(cleaned).replaceAll(".");
    }
}

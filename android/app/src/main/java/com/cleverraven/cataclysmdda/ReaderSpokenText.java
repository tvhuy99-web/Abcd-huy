package com.cleverraven.cataclysmdda;

import java.util.Locale;
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
            "(?m)^[ \\t]*(?:#[ \\t]*){2,}\\r?$");
    private static final Pattern EXCESS_DIAGRAM_GAPS = Pattern.compile("\n{3,}");

    // Speech-only expansion of compact measurement units directly after numbers
    // so Vietnamese TTS engines pronounce them naturally without altering UI width.
    private static final Pattern UNIT_DEG_C = Pattern.compile("(\\d)[ \\t]*°C(?![\\p{L}\\p{N}_/])");
    private static final Pattern UNIT_DEG_F = Pattern.compile("(\\d)[ \\t]*°F(?![\\p{L}\\p{N}_/])");
    private static final Pattern UNIT_KMH = Pattern.compile("(\\d)[ \\t]*km/h(?![\\p{L}\\p{N}_/])");
    private static final Pattern UNIT_MSVH = Pattern.compile("(\\d)[ \\t]*mSv/h(?![\\p{L}\\p{N}_/])");
    private static final Pattern UNIT_KJ = Pattern.compile("(\\d)[ \\t]*kJ(?![\\p{L}\\p{N}_/])");
    private static final Pattern UNIT_KCAL = Pattern.compile("(\\d)[ \\t]*kcal(?![\\p{L}\\p{N}_/])");
    private static final Pattern UNIT_ML = Pattern.compile("(\\d)[ \\t]*mL(?![\\p{L}\\p{N}_/])");

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

    /** Expand units only for the selected Vietnamese voice, without changing UI text. */
    static String expandVietnameseUnits(String text, Locale locale) {
        if (text == null || locale == null || !"vi".equals(locale.getLanguage())) {
            return text;
        }
        String cleaned = text;
        cleaned = UNIT_DEG_C.matcher(cleaned).replaceAll("$1 độ C");
        cleaned = UNIT_DEG_F.matcher(cleaned).replaceAll("$1 độ F");
        cleaned = UNIT_KMH.matcher(cleaned).replaceAll("$1 ki lô mét trên giờ");
        cleaned = UNIT_MSVH.matcher(cleaned).replaceAll("$1 mi li si vớt trên giờ");
        cleaned = UNIT_KJ.matcher(cleaned).replaceAll("$1 ki lô jun");
        cleaned = UNIT_KCAL.matcher(cleaned).replaceAll("$1 ki lô ca lo");
        cleaned = UNIT_ML.matcher(cleaned).replaceAll("$1 mi li lít");
        return cleaned;
    }
}

package com.cleverraven.cataclysmdda;

import java.util.regex.Pattern;







final class ReaderSpokenText {
    private static final Pattern COLOR_TAG = Pattern.compile(
            "(?i)</?color(?:_[a-z0-9_]+)?>");



    private static final Pattern DUPLICATE_STOP = Pattern.compile(
            "(?<!\\.)\\.{2}(?!\\.)");





    private static final Pattern ASCII_ARMOR_DIAGRAM_LINE = Pattern.compile(
            "(?m)^[ \\t]*(?:#[ \\t]*){2,}\\r?$");
    private static final Pattern EXCESS_DIAGRAM_GAPS = Pattern.compile("\n{3,}");

    private ReaderSpokenText() { }

    static String clean(String text) {
        if (text == null) { return ""; }
        String cleaned = COLOR_TAG.matcher(text).replaceAll("");
        if (cleaned.indexOf('#') >= 0) {
            String withoutArt = ASCII_ARMOR_DIAGRAM_LINE.matcher(cleaned).replaceAll("");
            if (!withoutArt.equals(cleaned)) {


                cleaned = EXCESS_DIAGRAM_GAPS.matcher(withoutArt).replaceAll("\n\n");
            }
        }
        return DUPLICATE_STOP.matcher(cleaned).replaceAll(".");
    }
}

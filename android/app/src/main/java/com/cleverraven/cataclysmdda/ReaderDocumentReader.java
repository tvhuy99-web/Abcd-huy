package com.cleverraven.cataclysmdda;

import android.app.Activity;
import android.icu.text.BreakIterator;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;














public final class ReaderDocumentReader {
    private static final Locale VIETNAMESE = new Locale("vi", "VN");
    private static final int MAX_PARAGRAPH_CHARS = 700;

    private final Activity activity;
    private String sessionKey = "";
    private String sourceText = "";
    private List<String> paragraphs = new ArrayList<>();
    private int index;
    private boolean open;
    private boolean interrupted;
    private String lastSentence = "";
    private final Map<String, Integer> positions = new HashMap<>();

    public ReaderDocumentReader(Activity activity) {
        this.activity = activity;
    }

    public boolean isOpen() { return open; }


    public String speechKey() { return "document:" + sessionKey; }

    private String string(int id) { return activity.getString(id); }


    public String open(String contextKey, String nodeId, String text) {
        String key = contextKey + ":" + nodeId;
        String safeText = text == null ? "" : text;








        if (!key.equals(sessionKey) || paragraphs.isEmpty() || !safeText.equals(sourceText)) {
            savePosition();
            boolean sameKey = key.equals(sessionKey);
            sessionKey = key;
            sourceText = safeText;
            paragraphs = splitParagraphs(safeText);
            Integer saved = positions.get(key);
            int restored = saved == null ? (sameKey ? index : 0) : saved;
            index = paragraphs.isEmpty() ? 0
                    : Math.max(0, Math.min(restored, paragraphs.size() - 1));
        }
        open = true;
        interrupted = false;
        return paragraphSpeech();
    }


    public void clear() {
        savePosition();
        open = false;
        interrupted = false;
        paragraphs = new ArrayList<>();
    }

    private void savePosition() {
        if (open && !sessionKey.isEmpty()) {
            positions.put(sessionKey, index);
        }
    }

    public void markFinished() {
        interrupted = false;
    }

    public void markInterrupted() {
        if (open) {
            interrupted = true;
        }
    }

    private String paragraphSpeech() {
        if (paragraphs.isEmpty()) {
            return string(R.string.a11yDocEnd);
        }
        String paragraph = paragraphs.get(index);
        lastSentence = lastSentenceOf(paragraph);
        return activity.getString(R.string.a11yDocParagraphPosition, index + 1, paragraphs.size())
                + ". " + paragraph;
    }

    public String nextParagraph() {
        if (!open || paragraphs.isEmpty() || index + 1 >= paragraphs.size()) {
            return string(R.string.a11yDocEnd);
        }
        index++;
        interrupted = false;
        savePosition();
        return paragraphSpeech();
    }

    public String previousParagraph() {
        if (!open || paragraphs.isEmpty() || index <= 0) {
            return string(R.string.a11yDocStart);
        }
        index--;
        interrupted = false;
        savePosition();
        return paragraphSpeech();
    }

    private static String lastSentenceOf(String paragraph) {
        BreakIterator sentences = BreakIterator.getSentenceInstance(VIETNAMESE);
        sentences.setText(paragraph);
        int start = sentences.first();
        int lastStart = 0;
        int lastEnd = paragraph.length();
        for (int end = sentences.next(); end != BreakIterator.DONE;
             start = end, end = sentences.next()) {
            lastStart = start;
            lastEnd = end;
        }
        return paragraph.substring(lastStart, lastEnd).trim();
    }

    private static List<String> splitParagraphs(String text) {
        ArrayList<String> result = new ArrayList<>();
        for (String block : text.split("\n\\s*\n")) {
            String trimmed = block.trim();
            if (trimmed.isEmpty()) {
                continue;
            }
            if (trimmed.length() <= MAX_PARAGRAPH_CHARS) {
                result.add(trimmed);
                continue;
            }
            BreakIterator sentences = BreakIterator.getSentenceInstance(VIETNAMESE);
            sentences.setText(trimmed);
            StringBuilder current = new StringBuilder();
            int start = sentences.first();
            for (int end = sentences.next(); end != BreakIterator.DONE;
                 start = end, end = sentences.next()) {
                String sentence = trimmed.substring(start, end).trim();
                if (sentence.isEmpty()) {
                    continue;
                }
                if (current.length() > 0
                        && current.length() + sentence.length() + 1 > MAX_PARAGRAPH_CHARS) {
                    result.add(current.toString());
                    current.setLength(0);
                }
                if (current.length() > 0) {
                    current.append(' ');
                }
                current.append(sentence);
            }
            if (current.length() > 0) {
                result.add(current.toString());
            }
        }
        return result;
    }
}

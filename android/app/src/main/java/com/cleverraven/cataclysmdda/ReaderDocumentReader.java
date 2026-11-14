package com.cleverraven.cataclysmdda;

import android.app.Activity;
import android.icu.text.BreakIterator;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;

/**
 * Session reader for long document nodes (R04.4).
 *
 * A document node carries its whole text in one snapshot field; the game
 * side stays unchanged. This class owns the reading position on the
 * Java side: the text is split into paragraphs once per session
 * (blank-line separated, over-long paragraphs split by sentence), and
 * the position is the paragraph most recently started. Interruption by
 * higher-priority speech never moves the position and never restarts
 * the document. Reading positions live only for the session (in memory,
 * keyed by screen context + node id) and are dropped when the screen
 * context changes.
 */
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

    /** Speech key used for every paragraph of the current document. */
    public String speechKey() { return "document:" + sessionKey; }

    private String string(int id) { return activity.getString(id); }

    /** Opens (or re-opens) a document and returns the paragraph to speak. */
    public String open(String contextKey, String nodeId, String text) {
        String key = contextKey + ":" + nodeId;
        String safeText = text == null ? "" : text;
        // Re-split not only for a new document but also when this
        // document's text changed under the same key, or when the live
        // paragraphs were dropped by clear() at a lifecycle boundary
        // (R21.1): before this, re-opening the same document after a
        // pause/resume found an empty paragraph list and reported the
        // end of the document, and a filter change in the message log
        // kept reading the pre-filter text. The saved position is kept
        // and clamped into the new text, never restarted from zero.
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

    /** Drops the live session (screen context changed, pause, close). */
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

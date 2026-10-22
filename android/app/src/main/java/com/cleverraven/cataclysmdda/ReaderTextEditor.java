package com.cleverraven.cataclysmdda;

import android.app.Activity;
import android.icu.text.BreakIterator;
import android.text.InputType;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;

import java.text.Normalizer;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

/** Native IME with TalkBack, plus a one-finger Vietnamese keyboard without IME. */
public final class ReaderTextEditor {
    public interface Listener {
        void speak(String text);
        void submit(String text);
        void cancel();
    }

    private static final Locale VIETNAMESE = new Locale("vi", "VN");
    private final Activity activity;
    private final Listener listener;
    private final boolean numeric;
    private final LinearLayout root;
    private final LinearLayout choices;
    private final EditText field;
    private final List<Choice> items = new ArrayList<>();
    private int selected;
    private String group = "controls";
    private boolean uppercase;
    private boolean clearing;
    private long revision;

    private static final class Choice {
        final String label;
        final Runnable action;
        Choice(String label, Runnable action) { this.label = label; this.action = action; }
    }

    public ReaderTextEditor(Activity activity, String initial, boolean numeric, Listener listener) {
        this.activity = activity;
        this.listener = listener;
        this.numeric = numeric;
        root = new LinearLayout(activity);
        root.setOrientation(LinearLayout.VERTICAL);
        TextView instructions = new TextView(activity);
        instructions.setText(R.string.a11yEditorInstructions);
        instructions.setTextColor(0xffeeeeee);
        root.addView(instructions);
        field = new EditText(activity);
        field.setSaveEnabled(false);
        field.setSingleLine(true);
        field.setInputType(numeric ? InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_SIGNED
                | InputType.TYPE_NUMBER_FLAG_DECIMAL : InputType.TYPE_CLASS_TEXT);
        field.setContentDescription(activity.getString(R.string.a11yEditorValue));
        field.setText(initial);
        field.setSelection(field.length());
        root.addView(field);
        choices = new LinearLayout(activity);
        choices.setOrientation(LinearLayout.VERTICAL);
        root.addView(choices);
        build();
    }

    public LinearLayout getView() { return root; }
    public String getText() { return field.getText().toString(); }
    public String contextKey() { return group + ":" + revision; }

    private String string(int id) { return activity.getString(id); }
    private void add(int label, Runnable action) { items.add(new Choice(string(label), action)); }

    private void build() {
        revision++;
        items.clear();
        if ("controls".equals(group)) {
            add(R.string.a11yEditorReadValue, () -> listener.speak(valueDescription()));
            if (!numeric) {
                add(R.string.a11yEditorLetters, () -> open("letters"));
                add(R.string.a11yEditorTone, () -> open("tones"));
                add(R.string.a11yEditorCase, () -> {
                    uppercase = !uppercase;
                    listener.speak(string(uppercase ? R.string.a11yEditorUppercase : R.string.a11yEditorLowercase));
                });
            }
            add(R.string.a11yEditorDigits, () -> open("digits"));
            add(R.string.a11yEditorPunctuation, () -> open("punctuation"));
            if (!numeric) { add(R.string.a11yEditorSpace, () -> insert(" ")); }
            add(R.string.a11yEditorLeft, () -> move(-1));
            add(R.string.a11yEditorRight, () -> move(1));
            add(R.string.a11yEditorDelete, this::backspace);
            add(clearing ? R.string.a11yEditorConfirmClear : R.string.a11yEditorClear, () -> {
                if (clearing) {
                    field.setText("");
                    clearing = false;
                    build();
                    listener.speak(string(R.string.a11yEditorEmpty));
                } else {
                    clearing = true;
                    build();
                    readCurrent();
                }
            });
            add(R.string.a11yEditorApply, () -> listener.submit(getText()));
            add(R.string.a11yEditorCancel, listener::cancel);
        } else if ("tones".equals(group)) {
            add(R.string.a11yEditorKeyboardReturn, () -> open("controls"));
            int[] labels = { R.string.a11yToneNone, R.string.a11yToneGrave, R.string.a11yToneAcute,
                    R.string.a11yToneHook, R.string.a11yToneTilde, R.string.a11yToneDot };
            String[] marks = { "", "\u0300", "\u0301", "\u0309", "\u0303", "\u0323" };
            for (int i = 0; i < labels.length; i++) {
                final String mark = marks[i];
                add(labels[i], () -> tone(mark));
            }
        } else {
            add(R.string.a11yEditorKeyboardReturn, () -> open("controls"));
            String alphabet = "letters".equals(group) ? "aăâbcdđeêghiklmnoôơpqrstuưvxyfzjw"
                    : "digits".equals(group) ? "0123456789" : numeric ? "-+." : ".,!?-_:;/\\@#%&*+=()[]{}\"'<>|~`$^";
            if (uppercase) { alphabet = alphabet.toUpperCase(VIETNAMESE); }
            for (int offset = 0; offset < alphabet.length();) {
                int cp = alphabet.codePointAt(offset);
                final String character = new String(Character.toChars(cp));
                items.add(new Choice(character, () -> insert(character)));
                offset += Character.charCount(cp);
            }
        }
        selected = Math.max(0, Math.min(selected, items.size() - 1));
        render();
    }

    private void open(String next) {
        clearing = false;
        group = next;
        selected = 0;
        build();
        readCurrent();
    }

    private void render() {
        choices.removeAllViews();
        int start = selected / 16 * 16;
        for (int i = start; i < Math.min(start + 16, items.size()); i++) {
            final int index = i;
            Button button = new Button(activity);
            button.setText(items.get(i).label);
            button.setSelected(i == selected);
            button.setOnClickListener(view -> { selected = index; activate(); });
            choices.addView(button);
        }
        if (items.size() > 16) {
            Button previous = new Button(activity);
            previous.setText(R.string.a11yPreviousPage);
            previous.setOnClickListener(view -> navigate(-16));
            choices.addView(previous);
            Button next = new Button(activity);
            next.setText(R.string.a11yNextPage);
            next.setOnClickListener(view -> navigate(16));
            choices.addView(next);
        }
    }

    public void navigate(int delta) {
        clearing = false;
        selected = Math.floorMod(selected + delta, items.size());
        render();
        readCurrent();
    }

    public void activate() { items.get(selected).action.run(); }

    public void readCurrent() {
        listener.speak(items.get(selected).label + ". "
                + activity.getString(R.string.a11yPosition, selected + 1, items.size()));
    }

    private BreakIterator boundaries(String text) {
        BreakIterator iterator = BreakIterator.getCharacterInstance(VIETNAMESE);
        iterator.setText(text);
        return iterator;
    }

    private int caret(String text) {
        int position = Math.max(0, Math.min(text.length(), field.getSelectionEnd()));
        BreakIterator iterator = boundaries(text);
        return iterator.isBoundary(position) ? position : Math.max(0, iterator.preceding(position));
    }

    private void insert(String value) {
        clearing = false;
        String text = getText();
        int position = caret(text);
        field.setText(text.substring(0, position) + value + text.substring(position));
        field.setSelection(position + value.length());
        listener.speak(" ".equals(value) ? string(R.string.a11yEditorSpace) : value);
    }

    private void move(int delta) {
        String text = getText();
        int position = caret(text);
        BreakIterator iterator = boundaries(text);
        int target = delta < 0 ? iterator.preceding(position) : iterator.following(position);
        if (target == BreakIterator.DONE) { target = delta < 0 ? 0 : text.length(); }
        field.setSelection(target);
        int next = iterator.following(target);
        listener.speak(next == BreakIterator.DONE ? string(R.string.a11yEditorEnd)
                : text.substring(target, next));
    }

    private void backspace() {
        String text = getText();
        int position = caret(text);
        if (position == 0) { listener.speak(string(R.string.a11yEditorStart)); return; }
        int previous = boundaries(text).preceding(position);
        String removed = text.substring(previous, position);
        field.setText(text.substring(0, previous) + text.substring(position));
        field.setSelection(previous);
        listener.speak(activity.getString(R.string.a11yEditorDeleted, removed));
    }

    private void tone(String mark) {
        String text = getText();
        int position = caret(text);
        if (position == 0) { listener.speak(string(R.string.a11yEditorNoVowel)); return; }
        int previous = boundaries(text).preceding(position);
        String decomposed = Normalizer.normalize(text.substring(previous, position), Normalizer.Form.NFD);
        if (decomposed.isEmpty() || "aeiouyAEIOUY".indexOf(decomposed.charAt(0)) < 0) {
            listener.speak(string(R.string.a11yEditorNoVowel));
            return;
        }
        StringBuilder base = new StringBuilder();
        for (int offset = 0; offset < decomposed.length();) {
            int cp = decomposed.codePointAt(offset);
            if (cp != 0x0300 && cp != 0x0301 && cp != 0x0303 && cp != 0x0309 && cp != 0x0323) {
                base.appendCodePoint(cp);
            }
            offset += Character.charCount(cp);
        }
        String replacement = Normalizer.normalize(base + mark, Normalizer.Form.NFC);
        field.setText(text.substring(0, previous) + replacement + text.substring(position));
        field.setSelection(previous + replacement.length());
        listener.speak(replacement);
    }

    public String valueDescription() {
        return getText().isEmpty() ? string(R.string.a11yEditorEmpty) : getText();
    }

    public boolean back() {
        if (!"controls".equals(group)) { open("controls"); return true; }
        listener.cancel();
        return true;
    }
}

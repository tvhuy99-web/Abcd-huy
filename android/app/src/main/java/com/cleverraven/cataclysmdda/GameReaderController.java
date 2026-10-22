package com.cleverraven.cataclysmdda;

import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.os.Looper;
import android.view.View;
import android.view.ViewGroup;
import android.view.accessibility.AccessibilityManager;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.atomic.AtomicLong;

/** Main-looper presentation/input owner. Native callbacks only enqueue intents. */
public final class GameReaderController implements AutoCloseable, ReaderGestureSurface.Listener {
    private static final AtomicLong REQUEST_SEQUENCE = new AtomicLong();
    private static final int PAGE_SIZE = 30;
    private final CataclysmDDA activity;
    private final ReaderPreferences preferences;
    private final ReaderSpeechQueue speech;
    private final AccessibilityManager accessibility;
    private final ReaderGestureSurface root;
    private final TextView title;
    private final TextView status;
    private final LinearLayout rows;
    private final Handler handler = new Handler(Looper.getMainLooper());
    private final List<Button> rowViews = new ArrayList<>();
    private final AccessibilityManager.TouchExplorationStateChangeListener explorationListener;
    private ReaderSnapshot snapshot;
    private ReaderTextEditor editor;
    private String editorContext = "";
    private long editorRequest;
    private boolean readScreenSummary;
    private String search = "";
    private boolean onlyCommands;
    private String focus = "";
    private boolean foreground;
    private boolean windowFocused;
    private boolean closed;
    private boolean controls;
    private int controlIndex;
    private boolean nativeConfigured;
    private boolean lastEnabled;
    private boolean lastSuspended = true;
    private volatile long presentationEpoch;

    public GameReaderController(CataclysmDDA activity, ViewGroup parent) {
        this.activity = activity;
        preferences = new ReaderPreferences(activity);
        accessibility = (AccessibilityManager) activity.getSystemService(Context.ACCESSIBILITY_SERVICE);
        speech = new ReaderSpeechQueue(activity, this::onSpeechHealth);
        root = new ReaderGestureSurface(activity, this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setClickable(true);
        root.setBackgroundColor(0xff161616);
        int padding = Math.round(12 * activity.getResources().getDisplayMetrics().density);
        root.setPadding(padding, padding, padding, padding);
        title = text(root, "");
        title.setTextSize(22);
        status = text(root, activity.getString(R.string.a11yReaderWaiting));
        ScrollView scroll = new ScrollView(activity);
        rows = new LinearLayout(activity);
        rows.setOrientation(LinearLayout.VERTICAL);
        scroll.addView(rows);
        root.addView(scroll, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1));
        LinearLayout pages = new LinearLayout(activity);
        button(pages, R.string.a11yPreviousPage, () -> navigate(-PAGE_SIZE));
        button(pages, R.string.a11yNextPage, () -> navigate(PAGE_SIZE));
        root.addView(pages);
        LinearLayout tools = new LinearLayout(activity);
        button(tools, R.string.a11yReaderControls, this::showControls);
        button(tools, R.string.a11yStopSpeech, this::stopSpeech);
        button(tools, R.string.a11yReaderSetup, this::openSettings);
        button(tools, R.string.a11yReaderSearch, this::openSearch);
        root.addView(tools);
        parent.addView(root, new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT));
        root.setVisibility(View.GONE);
        explorationListener = enabled -> {
            root.cancelGesture();
            speech.stop();
            presentationEpoch++;
            updateNativeState(true);
            render();
        };
        if (accessibility != null) {
            accessibility.addTouchExplorationStateChangeListener(explorationListener);
        }
    }

    private void onSpeechHealth(AndroidSpeech.State state) {
        if (closed) { return; }
        updateNativeState(false);
        if (state == AndroidSpeech.State.READY && !usesTouchExploration()) {
            readCurrent();
        } else if (state == AndroidSpeech.State.FAILED || state == AndroidSpeech.State.MISSING_LANGUAGE
                || state == AndroidSpeech.State.NETWORK_REQUIRED) {
            status.setText(R.string.a11yReaderVoiceRecovery);
            status.announceForAccessibility(activity.getString(R.string.a11yReaderVoiceRecovery));
        }
    }

    private TextView text(LinearLayout parent, String value) {
        TextView text = new TextView(activity);
        text.setTextColor(0xffeeeeee);
        text.setText(value);
        parent.addView(text);
        return text;
    }

    private Button button(LinearLayout parent, int label, Runnable action) {
        Button button = new Button(activity);
        button.setText(label);
        button.setOnClickListener(view -> action.run());
        parent.addView(button);
        return button;
    }

    public void onResume() {
        foreground = true;
        windowFocused = activity.hasWindowFocus();
        presentationEpoch++;
        root.setVisibility(preferences.isEnabled() ? View.VISIBLE : View.GONE);
        if (preferences.isEnabled() && !speech.isReady()) {
            speech.initialize(preferences);
        }
        updateNativeState(true);
    }

    public void onPause() {
        foreground = false;
        presentationEpoch++;
        root.cancelGesture();
        speech.stop();
        snapshot = null;
        editor = null;
        editorRequest = 0;
        rows.removeAllViews();
        updateNativeState(true);
    }

    public void onWindowFocusChanged(boolean focused) {
        windowFocused = focused;
        if (!focused) {
            root.cancelGesture();
            speech.stop();
        }
        updateNativeState(false);
    }

    private void updateNativeState(boolean force) {
        boolean enabled = !closed && preferences.isEnabled();
        boolean suspended = !foreground || !windowFocused || (!usesTouchExploration() && !speech.isReady());
        if (force || !nativeConfigured || lastEnabled != enabled || lastSuspended != suspended) {
            try {
                CataclysmDDA.nativeConfigureReader(enabled, suspended);
                nativeConfigured = true;
                lastEnabled = enabled;
                lastSuspended = suspended;
            } catch (UnsatisfiedLinkError unavailable) {
                nativeConfigured = false;
                status.setText(R.string.a11yReaderNativeUnavailable);
            }
        }
    }

    public void receive(String json) {
        final long owner = presentationEpoch;
        handler.post(() -> {
            if (closed || !foreground || !preferences.isEnabled() || owner != presentationEpoch) {
                return;
            }
            try {
                JSONObject object = new JSONObject(json);
                if (object.has("status")) {
                    String code = object.getString("status");
                    if ("text_applied".equals(code)) {
                        if (editor != null && receiptMatches(object)) {
                            editor = null;
                            editorRequest = 0;
                            render();
                            feedback(activity.getString(R.string.a11yEditorApplied));
                        }
                        return;
                    }
                    if ("invalid_value".equals(code)) {
                        if (editor != null && receiptMatches(object)) {
                            editorRequest = 0;
                            feedback(activity.getString(R.string.a11yEditorInvalid));
                        }
                        return;
                    }
                    int label = "disabled".equals(code) ? R.string.a11yReaderDisabledEntry
                            : "unsupported".equals(code) ? R.string.a11yReaderUnsupported
                            : R.string.a11yReaderStale;
                    if (editor != null && receiptMatches(object)) { editorRequest = 0; }
                    feedback(activity.getString(label));
                    return;
                }
                if (object.optBoolean("inactive")) {
                    snapshot = null;
                    editor = null;
                    editorRequest = 0;
                    root.cancelGesture();
                    speech.setContext("");
                    rows.removeAllViews();
                    status.setText(R.string.a11yReaderWaiting);
                    return;
                }
                ReaderSnapshot next = new ReaderSnapshot(object);
                if (snapshot != null && next.session == snapshot.session && next.frame == snapshot.frame
                        && (next.generation < snapshot.generation ||
                        (next.generation == snapshot.generation && next.revision < snapshot.revision))) {
                    return;
                }
                boolean changedContext = snapshot == null || !snapshot.contextKey().equals(next.contextKey());
                String previous = focus;
                snapshot = next;
                speech.setContext(next.contextKey());
                if (changedContext || next.find(focus) == null) {
                    focus = next.find(next.focus) != null ? next.focus
                            : next.nodes.isEmpty() ? "" : next.nodes.get(0).id;
                }
                if (changedContext) {
                    controls = false;
                    editor = null;
                    editorRequest = 0;
                    readScreenSummary = true;
                    search = "";
                    onlyCommands = false;
                }
                render();
                if (!usesTouchExploration() && (changedContext || !previous.equals(focus))) {
                    readCurrent();
                }
            } catch (JSONException malformed) {
                snapshot = null;
                rows.removeAllViews();
                status.setText(R.string.a11yReaderMalformed);
                feedback(activity.getString(R.string.a11yReaderMalformed));
            }
        });
    }

    private boolean receiptMatches(JSONObject object) {
        return Long.toString(editorRequest).equals(object.optString("request"))
                && editorContext.equals(object.optString("session") + ":" + object.optString("frame")
                + ":" + object.optString("generation"));
    }

    private String nodeText(ReaderSnapshot.Node node, boolean includeDetail) {
        StringBuilder text = new StringBuilder(node.label);
        if (!node.value.isEmpty()) { text.append(". ").append(node.value); }
        if (node.checked) { text.append(". ").append(activity.getString(R.string.a11yChecked)); }
        else if ("checkbox".equals(node.role)) { text.append(". ").append(activity.getString(R.string.a11yUnchecked)); }
        if (node.selected) { text.append(". ").append(activity.getString(R.string.a11ySelected)); }
        if (!search.isEmpty() || onlyCommands) {
            List<ReaderSnapshot.Node> filtered = visibleNodes();
            int position = filtered.indexOf(node);
            if (position >= 0) {
                text.append(". ").append(activity.getString(R.string.a11yPosition, position + 1, filtered.size()));
            }
        } else if (node.position > 0 && node.total > 0) {
            text.append(". ").append(activity.getString(R.string.a11yPosition, node.position, node.total));
        }
        if (!node.enabled) {
            text.append(". ").append(node.disabledReason.isEmpty()
                    ? activity.getString(R.string.a11yReaderDisabledEntry) : node.disabledReason);
        }
        if (includeDetail && !node.detail.isEmpty()) { text.append(". ").append(node.detail); }
        return text.toString();
    }

    private int index() {
        if (snapshot != null) {
            List<ReaderSnapshot.Node> filtered = visibleNodes();
            for (int i = 0; i < filtered.size(); i++) {
                if (filtered.get(i).id.equals(focus)) { return i; }
            }
        }
        return 0;
    }

    private List<ReaderSnapshot.Node> visibleNodes() {
        ArrayList<ReaderSnapshot.Node> result = new ArrayList<>();
        if (snapshot == null) { return result; }
        String query = search.toLowerCase(new Locale("vi", "VN"));
        for (ReaderSnapshot.Node node : snapshot.nodes) {
            if ((!onlyCommands || "command".equals(node.role)) && (query.isEmpty()
                    || (node.label + " " + node.value).toLowerCase(new Locale("vi", "VN")).contains(query))) {
                result.add(node);
            }
        }
        return result;
    }

    private void render() {
        if (closed) { return; }
        title.setText(controls ? R.string.a11yReaderControls : R.string.a11yReaderWaiting);
        rows.removeAllViews();
        rowViews.clear();
        if (editor != null) {
            title.setText(R.string.a11yEditorTitle);
            rows.addView(editor.getView());
            return;
        }
        if (controls) {
            int[] labels = controlLabels();
            for (int i = 0; i < labels.length; i++) {
                final int selected = i;
                Button button = button(rows, labels[i], () -> performControl(selected));
                button.setSelected(i == controlIndex);
                rowViews.add(button);
            }
            return;
        }
        if (snapshot == null) { return; }
        title.setText(snapshot.title);
        status.setText(snapshot.summary);
        List<ReaderSnapshot.Node> filtered = visibleNodes();
        if (!search.isEmpty() || onlyCommands) {
            status.setText(activity.getString(R.string.a11yReaderSearchCount, filtered.size()));
        }
        int start = (index() / PAGE_SIZE) * PAGE_SIZE;
        final ReaderSnapshot owner = snapshot;
        for (int i = start; i < Math.min(start + PAGE_SIZE, filtered.size()); i++) {
            ReaderSnapshot.Node node = filtered.get(i);
            Button button = new Button(activity);
            button.setText(nodeText(node, false));
            // Disabled entries remain discoverable and can explain their reason.
            button.setSelected(node.id.equals(focus));
            button.setOnClickListener(view -> {
                if (snapshot != owner) { feedback(activity.getString(R.string.a11yReaderStale)); return; }
                focus = node.id;
                activate();
            });
            button.setOnFocusChangeListener((view, focused) -> {
                if (focused && snapshot == owner && !usesTouchExploration()) {
                    focus = node.id;
                    readCurrent();
                }
            });
            rows.addView(button);
            rowViews.add(button);
        }
    }

    @Override
    public boolean usesTouchExploration() {
        return accessibility != null && accessibility.isEnabled() && accessibility.isTouchExplorationEnabled();
    }

    @Override
    public String gestureContext() {
        return presentationEpoch + ":" + (controls ? "controls" : snapshot == null ? "waiting"
                : snapshot.contextKey() + ":" + snapshot.revision)
                + (editor == null ? "" : ":editor:" + editor.contextKey());
    }

    @Override
    public void navigate(int delta) {
        if (editor != null) { editor.navigate(delta); return; }
        if (controls) {
            controlIndex = Math.floorMod(controlIndex + (delta > 0 ? 1 : -1), controlLabels().length);
            render();
            feedback(activity.getString(controlLabels()[controlIndex]));
        } else if (snapshot != null && !visibleNodes().isEmpty()) {
            List<ReaderSnapshot.Node> filtered = visibleNodes();
            int selected = Math.max(0, Math.min(filtered.size() - 1, index() + delta));
            focus = filtered.get(selected).id;
            render();
            send("focus", "");
            readCurrent();
        }
    }

    @Override
    public void activate() {
        if (editor != null) { editor.activate(); return; }
        if (controls) { performControl(controlIndex); return; }
        ReaderSnapshot.Node node = snapshot == null ? null : snapshot.find(focus);
        if (node == null) { return; }
        if (!node.enabled) { feedback(nodeText(node, true)); return; }
        if (node.actions.contains("set_text")) {
            openEditor(node);
            return;
        }
        if ("document".equals(node.role)) {
            if (usesTouchExploration()) { feedback(node.detail); }
            else { speech.say(node.id + ":document", node.detail, ReaderSpeechQueue.LONG_TEXT); }
            return;
        }
        send("activate", "");
    }

    private void openEditor(ReaderSnapshot.Node node) {
        editorContext = snapshot.contextKey();
        editorRequest = 0;
        editor = new ReaderTextEditor(activity, node.value, "number".equals(node.role),
                new ReaderTextEditor.Listener() {
                    @Override
                    public void speak(String text) { feedback(text); }
                    @Override
                    public void submit(String text) {
                        if (editorRequest == 0) { editorRequest = send("set_text", text); }
                    }
                    @Override
                    public void cancel() {
                        editor = null;
                        editorRequest = 0;
                        root.cancelGesture();
                        render();
                        readCurrent();
                    }
                });
        root.cancelGesture();
        render();
        feedback(activity.getString(R.string.a11yEditorInstructions));
    }

    private void openSearch() {
        if (editor != null || snapshot == null) { return; }
        controls = false;
        editorRequest = 0;
        editor = new ReaderTextEditor(activity, search, false, new ReaderTextEditor.Listener() {
            @Override
            public void speak(String text) { feedback(text); }
            @Override
            public void submit(String text) {
                search = text;
                onlyCommands = false;
                editor = null;
                List<ReaderSnapshot.Node> filtered = visibleNodes();
                focus = filtered.isEmpty() ? "" : filtered.get(0).id;
                root.cancelGesture();
                render();
                feedback(activity.getString(R.string.a11yReaderSearchCount, filtered.size()));
            }
            @Override
            public void cancel() { editor = null; render(); readCurrent(); }
        });
        root.cancelGesture();
        render();
        feedback(activity.getString(R.string.a11yReaderSearch));
    }

    private long send(String action, String value) {
        ReaderSnapshot owner = snapshot;
        if (owner == null || !foreground || !windowFocused || lastSuspended) { return 0; }
        final long request = REQUEST_SEQUENCE.incrementAndGet();
        try {
            if (!CataclysmDDA.nativeReaderIntent(owner.session, owner.frame, owner.generation,
                    owner.revision, request, focus, action, value)) {
                feedback(activity.getString(R.string.a11yReaderStale));
            } else { return request; }
        } catch (UnsatisfiedLinkError unavailable) {
            feedback(activity.getString(R.string.a11yReaderNativeUnavailable));
        }
        return 0;
    }

    public void readCurrent() {
        if (editor != null) { editor.readCurrent(); return; }
        if (controls) { feedback(activity.getString(controlLabels()[controlIndex])); return; }
        ReaderSnapshot.Node node = snapshot == null ? null : snapshot.find(focus);
        if (node != null) {
            if (usesTouchExploration()) {
                status.announceForAccessibility(nodeText(node, false));
            } else {
                String summary = readScreenSummary ? snapshot.summary + ". " : "";
                readScreenSummary = false;
                speech.focus(node.id, snapshot.title + ". " + summary + nodeText(node, false));
            }
        }
    }

    @Override
    public void detail() {
        if (editor != null) { feedback(editor.valueDescription()); return; }
        ReaderSnapshot.Node node = snapshot == null ? null : snapshot.find(focus);
        if (node != null) { feedback(nodeText(node, true)); }
    }

    private void feedback(String text) {
        if (!foreground || closed) { return; }
        if (usesTouchExploration()) {
            status.setText(text);
            status.announceForAccessibility(text);
        } else {
            speech.say("feedback", text, ReaderSpeechQueue.FOCUS);
        }
    }

    @Override
    public void stopSpeech() { speech.stop(); }

    private int[] controlLabels() {
        return new int[] { R.string.a11yReaderReturn, R.string.a11yReaderWhere,
                R.string.a11yReaderDetail, R.string.a11yReaderRepeat,
                R.string.a11yStopSpeech, R.string.a11yReaderCancel,
                R.string.a11yReaderSetup, R.string.a11yReaderGestureHelp,
                R.string.a11yReaderSearch, R.string.a11yReaderAllCommands, R.string.a11yReaderAllItems };
    }

    public void showControls() {
        if (editor != null) { editor.back(); return; }
        root.cancelGesture();
        controls = true;
        controlIndex = 0;
        speech.stop();
        render();
        readCurrent();
    }

    public void onBackPressed() {
        if (editor != null) { editor.back(); return; }
        if (controls) {
            controls = false;
            render();
            readCurrent();
        } else { showControls(); }
    }

    private void performControl(int index) {
        switch (index) {
            case 0: controls = false; render(); readCurrent(); break;
            case 1:
                feedback(snapshot == null ? activity.getString(R.string.a11yReaderWaiting)
                        : snapshot.title + ". " + snapshot.summary);
                break;
            case 2: detail(); break;
            case 3: if (usesTouchExploration()) { readCurrent(); } else { speech.repeat(); } break;
            case 4: stopSpeech(); break;
            case 5: controls = false; render(); send("cancel", ""); break;
            case 6: openSettings(); break;
            case 7: feedback(activity.getString(R.string.a11yReaderGestureInstructions)); break;
            case 8: openSearch(); break;
            case 9:
            case 10:
                controls = false;
                search = "";
                onlyCommands = index == 9;
                List<ReaderSnapshot.Node> filtered = visibleNodes();
                focus = filtered.isEmpty() ? "" : filtered.get(0).id;
                render();
                readCurrent();
                break;
            default: break;
        }
    }

    private void openSettings() {
        root.cancelGesture();
        speech.stop();
        activity.startActivity(new Intent(activity, ReaderSetupActivity.class));
    }

    public boolean isEnabled() { return preferences.isEnabled(); }

    @Override
    public void close() {
        if (closed) { return; }
        closed = true;
        foreground = false;
        presentationEpoch++;
        root.cancelGesture();
        speech.close();
        if (accessibility != null) {
            accessibility.removeTouchExplorationStateChangeListener(explorationListener);
        }
        updateNativeState(true);
        handler.removeCallbacksAndMessages(null);
        if (root.getParent() instanceof ViewGroup) { ((ViewGroup) root.getParent()).removeView(root); }
    }
}

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
    private static final AtomicLong ANNOUNCEMENT_SEQUENCE = new AtomicLong();
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
    private boolean movementPractice;
    private int movementPracticeDirections;
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
        movementPractice = false;
        movementPracticeDirections = 0;
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
                String selectedItemFocus = "";
                boolean selectionChanged = false;
                boolean announcesSelection = "targeting".equals(next.category)
                        || "world-selection".equals(next.category)
                        || "world-generation".equals(next.category)
                        || "mod-selection".equals(next.category)
                        || "active-world-mods".equals(next.category)
                        || "world-options".equals(next.category)
                        || "DEFAULTMODE".equals(next.category)
                        || "overmap".equals(next.category)
                        || "OPTIONS".equals(next.category);
                if (!changedContext && announcesSelection && next.category.equals(snapshot.category)) {
                    for (ReaderSnapshot.Node node : next.nodes) {
                        if (node.selected) {
                            ReaderSnapshot.Node previousNode = snapshot.find(node.id);
                            if (previousNode == null || !previousNode.selected) {
                                selectedItemFocus = node.id;
                                selectionChanged = true;
                                break;
                            }
                        }
                    }
                    if (!selectionChanged && "targeting".equals(next.category)
                            && "target:cursor".equals(focus)) {
                        ReaderSnapshot.Node previousCursor = snapshot.find("target:cursor");
                        ReaderSnapshot.Node nextCursor = next.find("target:cursor");
                        if (previousCursor != null && nextCursor != null
                                && (!previousCursor.value.equals(nextCursor.value)
                                    || !previousCursor.detail.equals(nextCursor.detail))) {
                            selectedItemFocus = "target:cursor";
                            selectionChanged = true;
                        }
                    }
                    if (!selectionChanged && "overmap".equals(next.category)) {
                        ReaderSnapshot.Node previousCursor = snapshot.find("overmap:cursor");
                        ReaderSnapshot.Node nextCursor = next.find("overmap:cursor");
                        if (previousCursor != null && nextCursor != null
                                && (!previousCursor.value.equals(nextCursor.value)
                                    || !previousCursor.detail.equals(nextCursor.detail))) {
                            selectedItemFocus = "overmap:cursor";
                            selectionChanged = true;
                        }
                        if (!selectionChanged) {
                            ReaderSnapshot.Node previousRoute = snapshot.find("overmap:route");
                            ReaderSnapshot.Node nextRoute = next.find("overmap:route");
                            if (previousRoute != null && nextRoute != null
                                    && (!previousRoute.value.equals(nextRoute.value)
                                        || previousRoute.enabled != nextRoute.enabled)) {
                                selectedItemFocus = "overmap:route";
                                selectionChanged = true;
                            }
                        }
                    }
                    if (!selectionChanged && ("world-generation".equals(next.category)
                            || "mod-selection".equals(next.category)
                            || "active-world-mods".equals(next.category)
                            || "world-options".equals(next.category)
                            || "OPTIONS".equals(next.category))) {
                        ReaderSnapshot.Node previousFocus = snapshot.find(focus);
                        ReaderSnapshot.Node nextFocus = next.find(focus);
                        if (previousFocus != null && nextFocus != null
                                && (!previousFocus.value.equals(nextFocus.value)
                                    || !previousFocus.detail.equals(nextFocus.detail)
                                    || previousFocus.enabled != nextFocus.enabled
                                    || !previousFocus.disabledReason.equals(nextFocus.disabledReason))) {
                            selectedItemFocus = focus;
                            selectionChanged = true;
                        }
                    }
                }
                String previous = focus;
                snapshot = next;
                speech.setContext(next.contextKey());
                if (selectionChanged) {
                    focus = selectedItemFocus;
                } else if (changedContext || next.find(focus) == null) {
                    focus = next.find(next.focus) != null ? next.focus
                            : next.nodes.isEmpty() ? "" : next.nodes.get(0).id;
                }
                if (changedContext) {
                    controls = false;
                    movementPractice = false;
                    movementPracticeDirections = 0;
                    editor = null;
                    editorRequest = 0;
                    readScreenSummary = true;
                    search = "";
                    onlyCommands = false;
                }
                render();
                if (selectionChanged) {
                    readCurrent();
                } else if (!usesTouchExploration() && (changedContext || !previous.equals(focus))) {
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

    public void announceGameMessage(String text, int priority) {
        if (text == null || text.trim().isEmpty() || text.length() > 16000) { return; }
        final String message = text;
        final int level = priority >= ReaderSpeechQueue.URGENT
                ? ReaderSpeechQueue.URGENT : ReaderSpeechQueue.INFORMATION;
        handler.post(() -> {
            if (closed || !foreground || !windowFocused || !preferences.isEnabled() || snapshot == null) {
                return;
            }
            if (usesTouchExploration()) {
                status.setText(message);
                status.announceForAccessibility(message);
            } else {
                speech.say("game-message:" + ANNOUNCEMENT_SEQUENCE.incrementAndGet(), message, level);
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
    public boolean movementPracticeEnabled() {
        return movementPractice && !controls && editor == null
                && snapshot != null && snapshot.find("world:here") != null;
    }

    @Override
    public void practiceMovementDirection(int dx, int dy) {
        if (!movementPracticeEnabled()) { return; }
        ReaderSnapshot.Node node = snapshot.find(actionForDirection(dx, dy));
        if (node == null) { return; }
        movementPracticeDirections |= directionBit(dx, dy);
        int practiced = Integer.bitCount(movementPracticeDirections);
        if (practiced == 8) {
            preferences.setMovementPracticeComplete();
            movementPractice = false;
            controls = true;
            controlIndex = 11;
            root.cancelGesture();
            render();
            feedback(activity.getString(R.string.a11yReaderMovementPracticeComplete));
            return;
        }
        feedback(activity.getString(R.string.a11yReaderMovementPracticeProgress,
                                    node.label, practiced));
    }

    static int directionBit(int dx, int dy) {
        if (dy < 0) { return dx < 0 ? 1 << 7 : dx > 0 ? 1 << 1 : 1; }
        if (dy > 0) { return dx < 0 ? 1 << 5 : dx > 0 ? 1 << 3 : 1 << 4; }
        return dx < 0 ? 1 << 6 : 1 << 2;
    }

    private static String actionForDirection(int dx, int dy) {
        if (dy < 0) { return dx < 0 ? "LEFTUP" : dx > 0 ? "RIGHTUP" : "UP"; }
        if (dy > 0) { return dx < 0 ? "LEFTDOWN" : dx > 0 ? "RIGHTDOWN" : "DOWN"; }
        return dx < 0 ? "LEFT" : "RIGHT";
    }

    @Override
    public boolean worldMovementEnabled() {
        return !controls && !movementPractice && editor == null
                && preferences.isFastMovementEnabled()
                && snapshot != null && snapshot.find("world:here") != null;
    }

    @Override
    public void moveDirection(int dx, int dy) {
        if (!worldMovementEnabled()) { return; }
        String action = actionForDirection(dx, dy);
        ReaderSnapshot.Node node = snapshot.find(action);
        if (node == null || !node.enabled) {
            feedback(activity.getString(R.string.a11yFastMovementUnavailable));
            return;
        }
        focus = action;
        send("activate", "");
    }

    @Override
    public void activate() {
        if (movementPractice) { endMovementPractice(); return; }
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
        int[] labels = new int[] { R.string.a11yReaderReturn, R.string.a11yReaderWhere,
                R.string.a11yReaderDetail, R.string.a11yReaderRepeat,
                R.string.a11yStopSpeech, R.string.a11yReaderCancel,
                R.string.a11yReaderSetup, R.string.a11yReaderGestureHelp,
                R.string.a11yReaderSearch, R.string.a11yReaderAllCommands, R.string.a11yReaderAllItems,
                preferences.isFastMovementEnabled() ? R.string.a11yReaderFastMovementOn
                : preferences.hasCompletedMovementPractice() ? R.string.a11yReaderFastMovementOffReady
                : R.string.a11yReaderFastMovementOffPractice,
                R.string.a11yReaderMovementPractice };
        if (!preferences.isFastMovementEnabled()
                && !preferences.hasCompletedMovementPractice()) {
            int[] expanded = new int[labels.length + 1];
            System.arraycopy(labels, 0, expanded, 0, labels.length);
            expanded[labels.length] = R.string.a11yReaderSkipMovementPracticeEnable;
            return expanded;
        }
        return labels;
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
        if (movementPractice) { endMovementPractice(); return; }
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
            case 11: {
                if (!preferences.isFastMovementEnabled()
                        && !preferences.hasCompletedMovementPractice()) {
                    beginMovementPractice();
                    break;
                }
                boolean enabled = !preferences.isFastMovementEnabled();
                preferences.setFastMovementEnabled(enabled);
                root.cancelGesture();
                render();
                feedback(activity.getString(enabled ? R.string.a11yReaderFastMovementOn
                                       : R.string.a11yReaderFastMovementOffReady));
                break;
            }
            case 12: beginMovementPractice(); break;
            case 13:
                if (!preferences.isFastMovementEnabled()
                        && !preferences.hasCompletedMovementPractice()) {
                    preferences.setFastMovementEnabled(true);
                    root.cancelGesture();
                    render();
                    feedback(activity.getString(R.string.a11yReaderFastMovementOn));
                }
                break;
            default: break;
        }
    }

    private void beginMovementPractice() {
        if (usesTouchExploration()) {
            feedback(activity.getString(R.string.a11yReaderMovementPracticeTalkBack));
            return;
        }
        if (snapshot == null || snapshot.find("world:here") == null) {
            feedback(activity.getString(R.string.a11yReaderMovementPracticeWorldRequired));
            return;
        }
        movementPractice = true;
        movementPracticeDirections = 0;
        controls = false;
        root.cancelGesture();
        render();
        feedback(activity.getString(R.string.a11yReaderMovementPracticeInstructions));
    }

    private void endMovementPractice() {
        movementPractice = false;
        root.cancelGesture();
        controls = true;
        controlIndex = 11;
        render();
        feedback(activity.getString(R.string.a11yReaderMovementPracticeExited));
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

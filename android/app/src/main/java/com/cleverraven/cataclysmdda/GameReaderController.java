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
    private final ReaderCuePlayer cues;
    private final AccessibilityManager accessibility;
    private final ReaderGestureSurface root;
    private final TextView title;
    private final TextView status;
    private final LinearLayout rows;
    private final Handler handler = new Handler(Looper.getMainLooper());
    private final List<Button> rowViews = new ArrayList<>();
    private final AccessibilityManager.TouchExplorationStateChangeListener explorationListener;
    private ReaderSnapshot snapshot;
    private final ReaderDocumentReader documentReader;
    private ReaderTextEditor editor;
    private String editorContext = "";
    private long editorRequest;
    private String formContext = "";
    private long formRequest;
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
    private Runnable gestureMapTrialTimeout;

    public GameReaderController(CataclysmDDA activity, ViewGroup parent) {
        this.activity = activity;
        preferences = new ReaderPreferences(activity);
        accessibility = (AccessibilityManager) activity.getSystemService(Context.ACCESSIBILITY_SERVICE);
        speech = new ReaderSpeechQueue(activity, new ReaderSpeechQueue.Listener() {
            @Override
            public void onHealth(AndroidSpeech.State state) {
                onSpeechHealth(state);
            }

            @Override
            public void onVoiceRecovered() {
                if (closed) { return; }
                status.setText(R.string.a11yReaderVoiceRecoveredDefault);
                status.announceForAccessibility(
                        activity.getString(R.string.a11yReaderVoiceRecoveredDefault));
            }
        });
        // Optional cues (R21.2): fired by the engine at the same points
        // that produce spoken outcomes/warnings, plus locally for intent
        // rejection receipts. A cue failure is said out loud once.
        cues = new ReaderCuePlayer(activity, preferences, () -> {
            if (!closed) {
                feedback(activity.getString(R.string.a11yCueUnavailable));
            }
        });
        documentReader = new ReaderDocumentReader(activity);
        // Document position survives interruptions (R04.4): the queue
        // only reports whether the current paragraph finished or was cut
        // off; the document reader decides what Continue replays.
        speech.setCompletionListener(new ReaderSpeechQueue.CompletionListener() {
            @Override
            public void onFinished(String key) {
                if (key.equals(documentReader.speechKey())) {
                    documentReader.markFinished();
                }
            }

            @Override
            public void onInterrupted(String key) {
                if (key.equals(documentReader.speechKey())) {
                    documentReader.markInterrupted();
                }
            }
        });
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
            boolean revertedTrial = false;
            if (enabled && preferences.isGestureMapTrialPending()) {
                preferences.revertGestureMapTrial();
                revertedTrial = true;
                if (gestureMapTrialTimeout != null) {
                    handler.removeCallbacks(gestureMapTrialTimeout);
                    gestureMapTrialTimeout = null;
                }
            }
            presentationEpoch++;
            updateNativeState(true);
            render();
            if (revertedTrial) {
                feedback(activity.getString(R.string.a11yReaderGestureMapReverted));
            }
        };
        if (accessibility != null) {
            accessibility.addTouchExplorationStateChangeListener(explorationListener);
        }
    }

    private void onSpeechHealth(AndroidSpeech.State state) {
        if (closed) { return; }
        ReaderDiagnostics.get().event("speech_" + state.name().toLowerCase(Locale.ROOT));
        ReaderDiagnostics.get().setSpeechStats(speech.snapshotStats());
        updateNativeState(false);
        if (state == AndroidSpeech.State.READY && !usesTouchExploration()) {
            readCurrent();
        } else if (state == AndroidSpeech.State.FAILED || state == AndroidSpeech.State.MISSING_LANGUAGE
                || state == AndroidSpeech.State.MISSING_VOICE
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
        if (closed) { return; }
        ReaderDiagnostics.get().event("lifecycle_resume");
        foreground = true;
        windowFocused = activity.hasWindowFocus();
        presentationEpoch++;
        root.setVisibility(preferences.isEnabled() ? View.VISIBLE : View.GONE);
        if (preferences.isEnabled() && !speech.isReady()) {
            speech.initialize(preferences);
        }
        boolean trialWasPending = preferences.isGestureMapTrialPending();
        boolean trialActive = preferences.startGestureMapTrialTimer();
        if (trialActive && preferences.isEnabled() && !usesTouchExploration()) {
            controls = true;
            controlIndex = controlLabels().length - 2;
            scheduleGestureMapTrialTimeout();
            render();
            readCurrent();
        } else if (trialWasPending) {
            preferences.revertGestureMapTrial();
            controls = true;
            controlIndex = 0;
            render();
            feedback(activity.getString(R.string.a11yReaderGestureMapReverted));
        }
        updateNativeState(true);
    }

    public void onPause() {
        ReaderDiagnostics.get().event("lifecycle_pause");
        ReaderDiagnostics.get().setSpeechStats(speech.snapshotStats());
        foreground = false;
        presentationEpoch++;
        if (gestureMapTrialTimeout != null) {
            handler.removeCallbacks(gestureMapTrialTimeout);
            gestureMapTrialTimeout = null;
        }
        root.cancelGesture();
        movementPractice = false;
        movementPracticeDirections = 0;
        // Lifecycle boundary (R20.3): not a plain stop — the queue also
        // forgets its last utterance, its context and any pending TTS
        // retry, so nothing queued or repeatable from this foreground
        // session can be spoken after a later resume.
        speech.clearForLifecycle();
        documentReader.clear();
        snapshot = null;
        editor = null;
        editorRequest = 0;
        formRequest = 0;
        rows.removeAllViews();
        updateNativeState(true);
    }

    public void onWindowFocusChanged(boolean focused) {
        if (closed) { return; }
        windowFocused = focused;
        if (!focused) {
            root.cancelGesture();
            // Losing window focus (lock screen, call, system dialog) is
            // the same speech boundary as onPause (R20.3): pending and
            // repeatable speech from before it must not resume playing
            // when focus returns; the republished snapshot speaks fresh.
            speech.clearForLifecycle();
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
                    // Receipt outcomes feed the diagnostics counters
                    // (R21.4): codes only, never the intent's contents.
                    ReaderDiagnostics.get().count("receipt_" + code);
                    if ("invalid_value".equals(code) && formRequest != 0
                            && formReceiptMatches(object)) {
                        // A form draft value was rejected on the game thread
                        // (R02.4); the applied value was never touched.
                        formRequest = 0;
                        playRejectedCue();
                        feedback(activity.getString(R.string.a11yEditorInvalid));
                        return;
                    }
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
                            playRejectedCue();
                            feedback(activity.getString(R.string.a11yEditorInvalid));
                        }
                        return;
                    }
                    int label = "disabled".equals(code) ? R.string.a11yReaderDisabledEntry
                            : "unsupported".equals(code) ? R.string.a11yReaderUnsupported
                            : R.string.a11yReaderStale;
                    if (editor != null && receiptMatches(object)) { editorRequest = 0; }
                    // A rejected intent gets the rejected cue (R21.2)
                    // alongside its spoken receipt; both say the same
                    // thing — nothing was performed.
                    playRejectedCue();
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
                        || "inventory".equals(next.category)
                        || "dialogue".equals(next.category)
                        || "world-selection".equals(next.category)
                        || "world-generation".equals(next.category)
                        || "mod-selection".equals(next.category)
                        || "active-world-mods".equals(next.category)
                        || "world-options".equals(next.category)
                        || "DEFAULTMODE".equals(next.category)
                        || "activity".equals(next.category)
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
                    if (!selectionChanged && "inventory".equals(next.category)) {
                        ReaderSnapshot.Node previousFocus = snapshot.find(focus);
                        ReaderSnapshot.Node nextFocus = next.find(focus);
                        if (previousFocus != null && nextFocus != null
                                && (!previousFocus.value.equals(nextFocus.value)
                                    || previousFocus.checked != nextFocus.checked
                                    || previousFocus.enabled != nextFocus.enabled
                                    || !previousFocus.disabledReason.equals(nextFocus.disabledReason))) {
                            selectedItemFocus = focus;
                            selectionChanged = true;
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
                    if (!selectionChanged && "activity".equals(next.category)) {
                        ReaderSnapshot.Node previousActivity = snapshot.find("activity:current");
                        ReaderSnapshot.Node nextActivity = next.find("activity:current");
                        if (previousActivity != null && nextActivity != null
                                && !previousActivity.label.equals(nextActivity.label)) {
                            selectedItemFocus = "activity:current";
                            selectionChanged = true;
                        }
                        if (!selectionChanged) {
                            ReaderSnapshot.Node previousInterruptibility = snapshot.find("activity:interruptibility");
                            ReaderSnapshot.Node nextInterruptibility = next.find("activity:interruptibility");
                            if (previousInterruptibility != null && nextInterruptibility != null
                                    && !previousInterruptibility.value.equals(nextInterruptibility.value)) {
                                selectedItemFocus = "activity:interruptibility";
                                selectionChanged = true;
                            }
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
                ReaderSnapshot previousSnapshot = snapshot;
                snapshot = next;
                speech.setContext(next.contextKey());
                if (selectionChanged) {
                    focus = selectedItemFocus;
                } else if (changedContext) {
                    focus = next.find(next.focus) != null ? next.focus
                            : next.nodes.isEmpty() ? "" : next.nodes.get(0).id;
                } else if (next.find(focus) == null) {
                    // Focus policy (R02.1): a vanished node lands on its
                    // nearest surviving ancestor, then on the nearest
                    // surviving sibling, and only then on the default.
                    focus = resolveFocusFallback(previousSnapshot, previous, next);
                }
                if (changedContext) {
                    controls = false;
                    movementPractice = false;
                    movementPracticeDirections = 0;
                    // A mode change kills any gesture still in flight
                    // (R05.3): the surface owner string also changes with
                    // the context, and cancelling here makes the kill
                    // explicit instead of relying on that check alone.
                    root.cancelGesture();
                    editor = null;
                    editorRequest = 0;
                    formRequest = 0;
                    readScreenSummary = true;
                    search = "";
                    onlyCommands = false;
                    // Document sessions are per screen context (R04.4);
                    // the saved position is kept for a later visit.
                    documentReader.clear();
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
            ReaderDiagnostics.get().count(level == ReaderSpeechQueue.URGENT
                    ? "announce_urgent" : "announce_info");
            if (usesTouchExploration()) {
                status.setText(message);
                status.announceForAccessibility(message);
            } else {
                // Information messages live 8 s in the queue (R21.1):
                // long enough to be heard in a quiet moment, short
                // enough that a burst of turn messages cannot still be
                // describing an old turn half a minute later. Urgent
                // warnings keep the standard 15 s life.
                speech.say("game-message:" + ANNOUNCEMENT_SEQUENCE.incrementAndGet(), message,
                        level, level == ReaderSpeechQueue.URGENT ? 15000 : 8000);
                ReaderDiagnostics.get().setSpeechStats(speech.snapshotStats());
            }
        });
    }

    /** A rejected intent: counted, then cued (R21.2/R21.4). */
    private void playRejectedCue() {
        ReaderDiagnostics.get().count("cue_rejected");
        cues.play(ReaderCuePlayer.REJECTED);
    }

    /**
     * Cue entry point from the engine (R21.2), called through
     * CataclysmDDA.onAccessibilityCue. The name is one of the internal
     * ReaderCuePlayer literals; the cue duplicates a signal that is
     * also spoken, and the player itself checks the user's opt-in.
     */
    public void onGameCue(String name) {
        if (closed || !foreground || !preferences.isEnabled() || name == null) {
            return;
        }
        ReaderDiagnostics.get().count("cue_" + name);
        cues.play(name);
    }

    private boolean receiptMatches(JSONObject object) {
        return Long.toString(editorRequest).equals(object.optString("request"))
                && editorContext.equals(object.optString("session") + ":" + object.optString("frame")
                + ":" + object.optString("generation"));
    }

    private boolean formReceiptMatches(JSONObject object) {
        return Long.toString(formRequest).equals(object.optString("request"))
                && formContext.equals(object.optString("session") + ":" + object.optString("frame")
                + ":" + object.optString("generation"));
    }

    /**
     * Where focus goes when the focused node disappears from a new snapshot
     * of the same screen: nearest surviving ancestor in the tree, then the
     * nearest surviving sibling under the same parent, then the snapshot
     * default (R02.1).
     */
    private static String resolveFocusFallback(ReaderSnapshot oldSnapshot, String oldFocus,
                                               ReaderSnapshot next) {
        if (oldSnapshot != null) {
            ReaderSnapshot.Node old = oldSnapshot.find(oldFocus);
            if (old != null) {
                String ancestor = old.parentId;
                for (int guard = 0; guard < 32 && !ancestor.isEmpty(); guard++) {
                    if (next.find(ancestor) != null) {
                        return ancestor;
                    }
                    ReaderSnapshot.Node parent = oldSnapshot.find(ancestor);
                    ancestor = parent == null ? "" : parent.parentId;
                }
                ReaderSnapshot.Node best = null;
                for (ReaderSnapshot.Node candidate : next.nodes) {
                    if (candidate.parentId.equals(old.parentId) && candidate.depth == old.depth
                            && (best == null || Math.abs(candidate.position - old.position)
                            < Math.abs(best.position - old.position))) {
                        best = candidate;
                    }
                }
                if (best != null) {
                    return best.id;
                }
            }
        }
        return next.find(next.focus) != null ? next.focus
                : next.nodes.isEmpty() ? "" : next.nodes.get(0).id;
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
        // Tree state (R02.1): position/total already count the sibling set
        // for tree nodes, so only the level and the expansion state are new.
        if (node.depth >= 0 && node.hasChildren) {
            text.append(". ").append(activity.getString(node.expanded
                    ? R.string.a11yTreeExpanded : R.string.a11yTreeCollapsed));
        }
        if (includeDetail && node.depth >= 0) {
            text.append(". ").append(activity.getString(R.string.a11yTreeLevel, node.depth + 1));
        }
        // Table row (R02.2): read the cells under their column headers. A
        // cell that repeats the row label or is empty adds nothing.
        if (!node.tableId.isEmpty() && snapshot != null) {
            ReaderSnapshot.Table table = snapshot.table(node.tableId);
            for (ReaderSnapshot.Cell cell : node.cells) {
                if (cell.value.isEmpty() || cell.value.equals(node.label)) { continue; }
                text.append(". ");
                ReaderSnapshot.Column column = table == null ? null : table.column(cell.column);
                if (column != null && !column.header.isEmpty()) {
                    text.append(column.header).append(": ");
                }
                text.append(cell.value);
                String unit = !cell.unit.isEmpty() ? cell.unit
                        : column != null ? column.unit : "";
                if (!unit.isEmpty()) { text.append(' ').append(unit); }
            }
        }
        // Form state (R02.3): a pending draft and a field error are always
        // spoken; the choice list is detail-only.
        if (node.hasDraft) {
            text.append(". ").append(activity.getString(R.string.a11yFormDraftValue,
                    displayRawValue(node, node.draftValue)));
        }
        if (!node.fieldError.isEmpty()) {
            text.append(". ").append(activity.getString(R.string.a11yFormFieldError,
                    node.fieldError));
        }
        if (includeDetail && !node.choices.isEmpty()) {
            StringBuilder choices = new StringBuilder();
            for (ReaderSnapshot.Choice choice : node.choices) {
                if (choices.length() > 0) { choices.append(", "); }
                choices.append(choice.label.isEmpty() ? choice.value : choice.label);
            }
            text.append(". ").append(activity.getString(R.string.a11yFormChoices,
                    choices.toString()));
        }
        // Numeric domain (R04.2): the range and step are detail-level
        // facts of a field, spoken on request, not on every focus move.
        if (includeDetail && node.hasFieldRange) {
            text.append(". ").append(activity.getString(R.string.a11yEditorRange,
                    displayRawValue(node, formatRawNumber(node.fieldMin)),
                    displayRawValue(node, formatRawNumber(node.fieldMax))));
            if (node.fieldStep > 0) {
                text.append(". ").append(activity.getString(R.string.a11yEditorStep,
                        formatRawNumber(node.fieldStep)));
            }
        }
        if (includeDetail && !node.hasFieldRange && node.fieldStep > 0) {
            text.append(". ").append(activity.getString(R.string.a11yEditorStep,
                    formatRawNumber(node.fieldStep)));
        }
        if (includeDetail && !node.detail.isEmpty()) { text.append(". ").append(node.detail); }
        return text.toString();
    }

    private static String formatRawNumber(double value) {
        if (value == Math.floor(value) && Math.abs(value) < 1e15) {
            return Long.toString(Math.round(value));
        }
        return java.math.BigDecimal.valueOf(value).stripTrailingZeros().toPlainString();
    }

    /** Display text for a raw form value: the choice label when there is one. */
    private static String displayRawValue(ReaderSnapshot.Node node, String raw) {
        ReaderSnapshot.Choice choice = node.choiceFor(raw);
        return choice != null && !choice.label.isEmpty() ? choice.label : raw;
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
            if (controlIndex >= labels.length) {
                controlIndex = 0;
            }
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
        // A pending form draft on the focused field gets explicit Apply and
        // Discard controls (R02.3); browsing alone never applies it.
        final ReaderSnapshot.Node focusedNode = snapshot.find(focus);
        if (focusedNode != null && focusedNode.hasDraft) {
            final ReaderSnapshot ownerSnapshot = snapshot;
            if (focusedNode.actions.contains("apply")) {
                Button apply = button(rows, R.string.a11yReaderApplyValue, () -> {
                    if (snapshot == ownerSnapshot && focusedNode.id.equals(focus)) {
                        send("apply", "");
                    }
                });
                rowViews.add(apply);
            }
            if (focusedNode.actions.contains("cancel_draft")) {
                Button discard = button(rows, R.string.a11yReaderCancelDraft, () -> {
                    if (snapshot == ownerSnapshot && focusedNode.id.equals(focus)) {
                        send("cancel_draft", "");
                    }
                });
                rowViews.add(discard);
            }
        }
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
    public boolean leftHandedNavigation() {
        return preferences.isLeftHandedNavigation();
    }

    @Override
    public int horizontalSwipeAction() {
        return preferences.getHorizontalSwipeAction();
    }

    @Override
    public int verticalSwipeAction() {
        return preferences.getVerticalSwipeAction();
    }

    @Override
    public float swipeDistancePx() {
        return preferences.getSwipeDistanceDp()
                * activity.getResources().getDisplayMetrics().density;
    }

    @Override
    public long swipeDurationLimitMs() {
        return preferences.getSwipeDurationLimitMs();
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
        // Practice deliberately does not require snapshot.swipeMovementAllowed:
        // it never sends a gameplay intent, it only reads direction labels
        // and counts practiced directions (see practiceMovementDirection).
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
        // The engine decides whether walking gestures are meaningful for the
        // current game state (on foot, not driving/remote/death camera, no
        // activity or automatic travel) and publishes that decision as
        // snapshot.swipeMovementAllowed (R00.2). The presence of the
        // "world:here" node alone is not sufficient: it also exists for the
        // death camera. The user's fast-movement preference and the local
        // editor/modal/practice checks still apply on top.
        return !controls && !movementPractice && editor == null
                && preferences.isFastMovementEnabled()
                && snapshot != null && snapshot.swipeMovementAllowed
                && snapshot.find("world:here") != null
                && hasAllMovementDirections(snapshot);
    }

    /** Walking swipes need the full eight-direction set the practice teaches. */
    private static boolean hasAllMovementDirections(ReaderSnapshot snapshot) {
        String[] directions = { "UP", "RIGHTUP", "RIGHT", "RIGHTDOWN",
                "DOWN", "LEFTDOWN", "LEFT", "LEFTUP" };
        for (String direction : directions) {
            if (snapshot.find(direction) == null) {
                return false;
            }
        }
        return true;
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
        // Command group headers (R04.3) are browsing landmarks only:
        // activating one reads it, it never sends an intent.
        if (node.id.startsWith("group:")) {
            feedback(activity.getString(R.string.a11yCommandGroup) + ". " + node.label);
            return;
        }
        // Tree parents expand and collapse through their dedicated intents,
        // never through a gameplay activation (R02.1).
        if (node.hasChildren && (node.actions.contains("expand")
                || node.actions.contains("collapse"))) {
            String action = node.expanded ? "collapse" : "expand";
            if (node.actions.contains(action)) {
                send(action, "");
                return;
            }
        }
        // Form fields (R02.3): choosing or toggling only writes a draft on
        // the engine side; the separate Apply control commits it.
        if (!node.fieldType.isEmpty()) {
            if ("choice".equals(node.fieldType) && node.actions.contains("select_choice")
                    && !node.choices.isEmpty()) {
                String current = node.effectiveRawValue();
                int index = -1;
                for (int i = 0; i < node.choices.size(); i++) {
                    if (node.choices.get(i).value.equals(current)) { index = i; break; }
                }
                ReaderSnapshot.Choice next = node.choices.get(
                        Math.floorMod(index + 1, node.choices.size()));
                send("select_choice", next.value);
                return;
            }
            if ("bool".equals(node.fieldType) && node.actions.contains("set_value")) {
                send("set_value", "true".equals(node.effectiveRawValue()) ? "false" : "true");
                return;
            }
            if (node.actions.contains("set_value")) {
                openFormEditor(node);
                return;
            }
        }
        if (node.actions.contains("set_text")) {
            openEditor(node);
            return;
        }
        if ("document".equals(node.role)) {
            // Long documents are read in paragraphs with a resumable
            // session position (R04.4), not as one whole-text utterance.
            speakDocument(documentReader.open(snapshot.contextKey(), node.id, node.detail));
            return;
        }
        send("activate", "");
    }

    private void openEditor(ReaderSnapshot.Node node) {
        editorContext = snapshot.contextKey();
        editorRequest = 0;
        // Number popups publish their edit step (and no range) through the
        // form contract since R04.2; text fields keep the plain editor.
        editor = new ReaderTextEditor(activity, node.value, "number".equals(node.role),
                "int".equals(node.fieldType), null, null,
                node.fieldStep > 0 ? node.fieldStep : null,
                new ReaderTextEditor.Listener() {
                    @Override
                    public void speak(String text) { feedback(text); }
                    @Override
                    public void submit(String text) {
                        // A callback of an editor that was already closed
                        // must not mint a fresh intent (R03.2); the views
                        // are gone by then, this is the explicit guard.
                        if (editor != null && editorRequest == 0) {
                            editorRequest = send("set_text", text);
                        }
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

    /**
     * Draft editor for numeric and text form fields (R02.3). Submitting only
     * sends a set_value intent: the engine stores it as an unapplied draft
     * and the refreshed snapshot speaks it as such. The editor closes as
     * soon as the intent is accepted by the mailbox; an invalid_value
     * receipt is matched through formRequest and spoken separately.
     */
    private void openFormEditor(ReaderSnapshot.Node node) {
        formContext = snapshot.contextKey();
        formRequest = 0;
        final String nodeId = node.id;
        // The field's published domain reaches the editor (R04.2): the
        // user hears the range/step and an invalid draft is refused with
        // its specific reason before any intent is sent; the engine
        // re-validates at its gate regardless.
        editor = new ReaderTextEditor(activity, node.effectiveRawValue(),
                "int".equals(node.fieldType) || "float".equals(node.fieldType),
                "int".equals(node.fieldType),
                node.hasFieldRange ? node.fieldMin : null,
                node.hasFieldRange ? node.fieldMax : null,
                node.hasFieldRange && node.fieldStep > 0 ? node.fieldStep : null,
                new ReaderTextEditor.Listener() {
                    @Override
                    public void speak(String text) { feedback(text); }
                    @Override
                    public void submit(String text) {
                        if (editor == null || formRequest != 0 || snapshot == null
                                || !nodeId.equals(focus)) { return; }
                        formRequest = send("set_value", text);
                        if (formRequest != 0) {
                            editor = null;
                            root.cancelGesture();
                            render();
                        }
                    }
                    @Override
                    public void cancel() {
                        editor = null;
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

    private void performDocumentControl(int docIndex) {
        switch (docIndex) {
            case 0: speakDocument(documentReader.continueReading()); break;
            case 1: speakDocument(documentReader.nextParagraph()); break;
            case 2: speakDocument(documentReader.previousParagraph()); break;
            case 3: speakDocument(documentReader.repeatLastSentence()); break;
            case 4: speakDocument(documentReader.spellCurrent()); break;
            case 5: feedback(documentReader.bookmarkHere()); break;
            case 6: speakDocument(documentReader.goToBookmark()); break;
            case 7: openDocumentSearch(); break;
            case 8: speakDocument(documentReader.findNext()); break;
            case 9: speakDocument(documentReader.findPrevious()); break;
            default: break;
        }
    }

    /** In-document search (R04.4): same editor as screen search, but the
     *  query is kept verbatim, recorded in the document's session search
     *  history, and matched against the document text itself. */
    private void openDocumentSearch() {
        if (editor != null || !documentReader.isOpen()) { return; }
        controls = false;
        editorRequest = 0;
        editor = new ReaderTextEditor(activity, documentReader.lastQuery(), false,
                new ReaderTextEditor.Listener() {
                    @Override
                    public void speak(String text) { feedback(text); }

                    @Override
                    public void submit(String text) {
                        editor = null;
                        root.cancelGesture();
                        render();
                        speakDocument(documentReader.find(text));
                    }

                    @Override
                    public void cancel() { editor = null; render(); readCurrent(); }
                });
        root.cancelGesture();
        render();
        feedback(activity.getString(R.string.a11yDocSearch) + ". "
                + documentReader.searchHistorySpeech());
    }

    private long send(String action, String value) {
        ReaderSnapshot owner = snapshot;
        // The closed check is explicit, not implied by the lifecycle flags:
        // after close() no intent of this controller may reach the game,
        // even if a stray lifecycle callback ran first (R03.4).
        if (closed || owner == null || !foreground || !windowFocused || lastSuspended) { return 0; }
        final long request = REQUEST_SEQUENCE.incrementAndGet();
        try {
            if (!CataclysmDDA.nativeReaderIntent(owner.session, owner.frame, owner.generation,
                    owner.revision, request, focus, action, value)) {
                // Focus intents are re-sent on every navigation and race
                // screen changes routinely; a rejected one is dropped
                // silently instead of announcing a stale entry (R03.1).
                if (!"focus".equals(action)) {
                    feedback(activity.getString(R.string.a11yReaderStale));
                }
            } else { return request; }
        } catch (UnsatisfiedLinkError unavailable) {
            feedback(activity.getString(R.string.a11yReaderNativeUnavailable));
        }
        return 0;
    }

    @Override
    public void readCurrent() {
        if (editor != null) { editor.readCurrent(); return; }
        if (controls) {
            int[] labels = controlLabels();
            controlIndex = Math.floorMod(controlIndex, labels.length);
            feedback(activity.getString(labels[controlIndex]));
            return;
        }
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

    /**
     * Spell the focused item grapheme by grapheme (R21.3). Names that
     * have no translation are spoken exactly as written; spelling is
     * how the listener checks the precise letters, including names in
     * another language or script.
     */
    private void spellCurrentItem() {
        if (editor != null) { feedback(editor.valueDescription()); return; }
        ReaderSnapshot.Node node = snapshot == null ? null : snapshot.find(focus);
        if (node == null) {
            feedback(activity.getString(R.string.a11yReaderWaiting));
            return;
        }
        feedback(ReaderDocumentReader.spellText(nodeText(node, true),
                activity.getString(R.string.a11yDocSpellTruncated)));
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

    /** Document speech stays at LONG_TEXT priority: warnings may cut it
     *  off, and the document reader then resumes at the same paragraph. */
    private void speakDocument(String text) {
        if (text == null || text.isEmpty()) { return; }
        if (usesTouchExploration()) {
            feedback(text);
        } else {
            speech.say(documentReader.speechKey(), text, ReaderSpeechQueue.LONG_TEXT);
        }
    }

    @Override
    public void stopSpeech() { speech.stop(); }

    /** Document controls appended while a document session is open (R04.4). */
    private static final int[] DOC_CONTROLS = {
            R.string.a11yDocContinue, R.string.a11yDocNextParagraph,
            R.string.a11yDocPreviousParagraph, R.string.a11yDocRepeatSentence,
            R.string.a11yDocSpellParagraph, R.string.a11yDocBookmarkSet,
            R.string.a11yDocBookmarkGo, R.string.a11yDocSearch,
            R.string.a11yDocSearchNext, R.string.a11yDocSearchPrevious };

    private int[] controlLabels() {
        int[] labels = new int[] { R.string.a11yReaderReturn, R.string.a11yReaderWhere,
                R.string.a11yReaderDetail, R.string.a11yReaderRepeat,
                R.string.a11yStopSpeech, R.string.a11yReaderCancel,
                R.string.a11yReaderSetup, R.string.a11yReaderGestureHelp,
                R.string.a11yReaderSearch, R.string.a11yReaderAllCommands, R.string.a11yReaderAllItems,
                preferences.isFastMovementEnabled() ? R.string.a11yReaderFastMovementOn
                : preferences.hasCompletedMovementPractice() ? R.string.a11yReaderFastMovementOffReady
                : R.string.a11yReaderFastMovementOffPractice,
                R.string.a11yReaderMovementPractice,
                R.string.a11yReaderSpell };
        if (!preferences.isFastMovementEnabled()
                && !preferences.hasCompletedMovementPractice()) {
            int[] expanded = new int[labels.length + 1];
            System.arraycopy(labels, 0, expanded, 0, labels.length);
            expanded[labels.length] = R.string.a11yReaderSkipMovementPracticeEnable;
            labels = expanded;
        }
        if (documentReader.isOpen()) {
            int[] expanded = new int[labels.length + DOC_CONTROLS.length];
            System.arraycopy(labels, 0, expanded, 0, labels.length);
            System.arraycopy(DOC_CONTROLS, 0, expanded, labels.length, DOC_CONTROLS.length);
            labels = expanded;
        }
        if (preferences.isGestureMapTrialActive()) {
            int[] expanded = new int[labels.length + 2];
            System.arraycopy(labels, 0, expanded, 0, labels.length);
            expanded[labels.length] = R.string.a11yReaderConfirmGestureMap;
            expanded[labels.length + 1] = R.string.a11yReaderRevertGestureMap;
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
        if (closed) { return; }
        if (movementPractice) { endMovementPractice(); return; }
        if (editor != null) { editor.back(); return; }
        if (controls) {
            controls = false;
            root.cancelGesture();
            render();
            readCurrent();
        } else { showControls(); }
    }

    private void performControl(int index) {
        boolean trialWasPending = preferences.isGestureMapTrialPending();
        boolean trialIsActive = preferences.isGestureMapTrialActive();
        if (trialWasPending && !trialIsActive) {
            controlIndex = 0;
            render();
            feedback(activity.getString(R.string.a11yReaderGestureMapReverted));
            return;
        }
        int[] availableControls = controlLabels();
        controlIndex = Math.floorMod(controlIndex, availableControls.length);
        if (index < 0 || index >= availableControls.length) {
            index = 0;
        }
        if (trialIsActive) {
            int confirmIndex = availableControls.length - 2;
            if (index == confirmIndex) {
                confirmGestureMapTrial();
                return;
            }
            if (index == confirmIndex + 1) {
                revertGestureMapTrial();
                return;
            }
        }
        // Document controls (R04.4) sit after the base controls and the
        // optional skip-practice entry, before the trial entries that
        // controlLabels() always appends last.
        if (documentReader.isOpen()) {
            int docStart = 14 + (!preferences.isFastMovementEnabled()
                    && !preferences.hasCompletedMovementPractice() ? 1 : 0);
            if (index >= docStart && index < docStart + DOC_CONTROLS.length) {
                performDocumentControl(index - docStart);
                return;
            }
        }
        switch (index) {
            case 0: controls = false; root.cancelGesture(); render(); readCurrent(); break;
            case 1:
                feedback(snapshot == null ? activity.getString(R.string.a11yReaderWaiting)
                        : snapshot.title + ". " + snapshot.summary);
                break;
            case 2: detail(); break;
            case 3: if (usesTouchExploration()) { readCurrent(); } else { speech.repeat(); } break;
            case 4: stopSpeech(); break;
            case 5: controls = false; root.cancelGesture(); render(); send("cancel", ""); break;
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
            case 13: spellCurrentItem(); break;
            case 14:
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

    private void scheduleGestureMapTrialTimeout() {
        if (gestureMapTrialTimeout != null) {
            handler.removeCallbacks(gestureMapTrialTimeout);
        }
        final long remaining = preferences.gestureMapTrialRemainingMs();
        gestureMapTrialTimeout = new Runnable() {
            @Override
            public void run() {
                if (closed) {
                    gestureMapTrialTimeout = null;
                    return;
                }
                if (!preferences.isGestureMapTrialPending()) {
                    gestureMapTrialTimeout = null;
                    if (foreground) {
                        controls = true;
                        controlIndex = 0;
                        root.cancelGesture();
                        render();
                        feedback(activity.getString(R.string.a11yReaderGestureMapReverted));
                    }
                    return;
                }
                long timeLeft = preferences.gestureMapTrialRemainingMs();
                if (timeLeft > 0L) {
                    handler.postDelayed(this, timeLeft);
                    return;
                }
                preferences.revertGestureMapTrial();
                gestureMapTrialTimeout = null;
                // Never leave a hybrid state: if practice was somehow active
                // when the trial expired, it ends together with the trial
                // instead of surviving under the forced controls screen.
                movementPractice = false;
                movementPracticeDirections = 0;
                root.cancelGesture();
                if (foreground) {
                    controls = true;
                    controlIndex = 0;
                    render();
                    feedback(activity.getString(R.string.a11yReaderGestureMapReverted));
                }
            }
        };
        handler.postDelayed(gestureMapTrialTimeout, Math.max(1L, remaining));
    }

    private void confirmGestureMapTrial() {
        if (gestureMapTrialTimeout != null) {
            handler.removeCallbacks(gestureMapTrialTimeout);
            gestureMapTrialTimeout = null;
        }
        if (preferences.confirmGestureMapTrial()) {
            root.cancelGesture();
            controlIndex = 0;
            render();
            feedback(activity.getString(R.string.a11yReaderGestureMapConfirmed));
        }
    }

    private void revertGestureMapTrial() {
        if (gestureMapTrialTimeout != null) {
            handler.removeCallbacks(gestureMapTrialTimeout);
            gestureMapTrialTimeout = null;
        }
        preferences.revertGestureMapTrial();
        root.cancelGesture();
        controlIndex = 0;
        render();
        feedback(activity.getString(R.string.a11yReaderGestureMapReverted));
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
        if (preferences.isGestureMapTrialPending()) {
            // The two exercises must never be live at once (R00.3): practice
            // captures every directional swipe, so a gesture-map trial left
            // running underneath it could not be tested and its timeout
            // would fire mid-practice. Entering practice ends the trial and
            // returns the confirmed gesture map.
            revertGestureMapTrial();
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
        if (gestureMapTrialTimeout != null) {
            handler.removeCallbacks(gestureMapTrialTimeout);
            gestureMapTrialTimeout = null;
        }
        root.cancelGesture();
        documentReader.clear();
        cues.close();
        speech.close();
        if (accessibility != null) {
            accessibility.removeTouchExplorationStateChangeListener(explorationListener);
        }
        updateNativeState(true);
        handler.removeCallbacksAndMessages(null);
        if (root.getParent() instanceof ViewGroup) { ((ViewGroup) root.getParent()).removeView(root); }
    }
}

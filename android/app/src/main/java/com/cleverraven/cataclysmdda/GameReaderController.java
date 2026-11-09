package com.cleverraven.cataclysmdda;

import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import android.view.View;
import android.view.ViewGroup;
import android.view.accessibility.AccessibilityManager;
import android.view.accessibility.AccessibilityNodeInfo;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import org.json.JSONException;
import org.json.JSONObject;

import java.math.BigDecimal;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.atomic.AtomicLong;

/** Main-looper presentation/input owner. Native callbacks only enqueue intents. */
public final class GameReaderController implements AutoCloseable, ReaderGestureSurface.Listener {
    private static final AtomicLong REQUEST_SEQUENCE = new AtomicLong();
    private static final AtomicLong ANNOUNCEMENT_SEQUENCE = new AtomicLong();
    private static final int HUB_PAGE_SIZE = 30;
    private final CataclysmDDA activity;
    private final ReaderPreferences preferences;
    private final ReaderGestureBindings gestureBindings;
    private final ReaderFavorites favorites;
    private final ReaderSpeechQueue speech;
    private final ReaderCuePlayer cues;
    private final AccessibilityManager accessibility;
    private final ReaderGestureSurface root;
    private final TextView status;
    private final LinearLayout rows;
    private final Handler handler = new Handler(Looper.getMainLooper());
    private final LinkedHashMap<Long, Long> pendingIntentTimes = new LinkedHashMap<>();
    // Native actions invalidate their snapshot briefly. Messages arriving in
    // this gap must be read after the next snapshot instead of disappearing.
    private static final int MAX_BUFFERED_MESSAGES = 8;
    private final ArrayDeque<BufferedGameMessage> pendingGameMessages =
            new ArrayDeque<>();

    private static final class BufferedGameMessage {
        final String text;
        final int priority;
        final long created;

        BufferedGameMessage(String text, int priority) {
            this.text = text;
            this.priority = priority;
            created = SystemClock.elapsedRealtime();
        }
    }

    private void replayBufferedGameMessages() {
        while (!pendingGameMessages.isEmpty()) {
            BufferedGameMessage buffered = pendingGameMessages.removeFirst();
            if (SystemClock.elapsedRealtime() - buffered.created < 15000) {
                // The new screen context must be set before speech is sent:
                // setContext() deliberately cancels the previous context.
                announceGameMessage(buffered.text, buffered.priority);
            } else {
                SessionDebugLog.event("GAME_MESSAGE_DROPPED", "reason=expired"
                        + " text=" + buffered.text);
            }
        }
    }
    private final AccessibilityManager.TouchExplorationStateChangeListener explorationListener;
    private ReaderSnapshot snapshot;
    // Loading screens do not publish semantic snapshots. Keep their last
    // visible status available to repeat, even after the main menu clears.
    private String lastLoadingStatus = "";
    private final ReaderDocumentReader documentReader;
    private ReaderTextEditor editor;
    private String editorContext = "";
    private long editorRequest;
    private String formContext = "";
    private long formRequest;
    private String focus = "";
    // Only the requested comprehensive menu appears as an Android overlay;
    // routine gameplay keeps the transparent, dashboard-free surface.
    private boolean comprehensiveMenu;
    private int hubSection = GameActionHub.ROOT;
    private int hubIndex;
    private int hubRootIndex;
    private String hubQuery = "";
    private boolean choosingGesture;
    private GameActionHub.Entry bindingItem;
    private int gestureChoice;
    // A long press opens these two accessible actions before gesture setup.
    private boolean choosingHubAction;
    private GameActionHub.Entry hubActionItem;
    private int hubActionChoice;
    // Only rendered when SDL is showing a native debug message.
    private String debugReport;
    private int debugFocus;
    private boolean debugResponsePending;
    // One spoken/TalkBack level at a time, independent of the engine's
    // visual expansion state. Parent IDs are supplied by semantic snapshots.
    // Global contextual command drawer: native commands stay published and validated,
    // but never interrupt the primary list of items, answers or form fields.
    private boolean commandPaletteOpen;
    private String commandPaletteReturnFocus = "";
    private String commandPaletteReturnParent = "";
    private String readerLayerParent = "";
    private String pendingReaderLayer = "";
    private int pendingReaderLayerRevisions;
    // A native modal may temporarily cover a parent screen. Frame identity
    // survives that push/pop, so return to the last visited item, not item 1.
    private final LinkedHashMap<String, String> savedScreenFocus = new LinkedHashMap<>();
    private boolean foreground;
    private boolean windowFocused;
    private boolean closed;
    private boolean nativeConfigured;
    private boolean lastEnabled;
    private boolean lastSuspended = true;
    private volatile long presentationEpoch;

    public GameReaderController(CataclysmDDA activity, ViewGroup parent) {
        this.activity = activity;
        preferences = new ReaderPreferences(activity);
        gestureBindings = new ReaderGestureBindings(activity);
        favorites = new ReaderFavorites(activity);
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
                announceAccessibility(
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
        // Keep the engine and gesture bridge, never draw the old reader dashboard.
        root.setBackgroundColor(android.graphics.Color.TRANSPARENT);
        root.setPadding(0, 0, 0, 0);
        status = new TextView(activity);
        // Off-screen state holders, not visible labels or control buttons.
        ScrollView scroll = new ScrollView(activity);
        scroll.setBackgroundColor(android.graphics.Color.TRANSPARENT);
        rows = new LinearLayout(activity);
        rows.setOrientation(LinearLayout.VERTICAL);
        scroll.addView(rows);
        root.addView(scroll, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, 0, 1));
        parent.addView(root, new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT));
        root.setVisibility(View.GONE);
        explorationListener = enabled -> {
            root.cancelGesture();
            speech.stop();
            if (enabled && preferences.isGestureMapTrialPending()) {
                preferences.revertGestureMapTrial();
            }
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
        ReaderDiagnostics.get().event("speech_" + state.name().toLowerCase(Locale.ROOT));
        ReaderDiagnostics.get().setSpeechStats(speech.snapshotStats());
        updateNativeState(false);
        if (state == AndroidSpeech.State.READY && !usesTouchExploration()) {
            readCurrent();
        } else if (state == AndroidSpeech.State.FAILED || state == AndroidSpeech.State.MISSING_LANGUAGE
                || state == AndroidSpeech.State.MISSING_VOICE
                || state == AndroidSpeech.State.NETWORK_REQUIRED) {
            status.setText(R.string.a11yReaderVoiceRecovery);
            announceAccessibility(activity.getString(R.string.a11yReaderVoiceRecovery));
        }
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
        // Do not reopen the deleted dashboard for an old gesture trial.
        if (preferences.isGestureMapTrialPending()) {
            preferences.revertGestureMapTrial();
        }
        render();
        updateNativeState(true);
    }

    public void onPause() {
        ReaderDiagnostics.get().event("lifecycle_pause");
        ReaderDiagnostics.get().setSpeechStats(speech.snapshotStats());
        foreground = false;
        presentationEpoch++;
        root.cancelGesture();
        // Lifecycle boundary (R20.3): not a plain stop — the queue also
        // forgets its last utterance, its context and any pending TTS
        // retry, so nothing queued or repeatable from this foreground
        // session can be spoken after a later resume.
        speech.clearForLifecycle();
        pendingGameMessages.clear();
        documentReader.clear();
        snapshot = null;
        lastLoadingStatus = "";
        readerLayerParent = "";
        pendingReaderLayer = "";
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
            pendingGameMessages.clear();
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
                    long requestId = object.optLong("request", 0L);
                    Long since = pendingIntentTimes.remove(requestId);
                    SessionDebugLog.event("INTENT_RESULT", "request=" + requestId
                            + " status=" + code + " elapsed_ms="
                            + (since == null ? "unknown"
                            : (SystemClock.elapsedRealtime() - since)));
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
                    comprehensiveMenu = false;
                    choosingGesture = false;
                    bindingItem = null;
                    choosingHubAction = false;
                    hubActionItem = null;
                    snapshot = null;
                    commandPaletteOpen = false;
                    commandPaletteReturnFocus = "";
                    commandPaletteReturnParent = "";
                    lastLoadingStatus = "";
                                editor = null;
                    editorRequest = 0;
                    root.cancelGesture();
                    speech.setContext("");
                    rows.removeAllViews();
                    status.setText(R.string.a11yReaderWaiting);
                    return;
                }
                ReaderSnapshot next = new ReaderSnapshot(object);
                SessionDebugLog.event("SCREEN", "category=" + next.category
                        + " title=" + next.title + " focus=" + next.focus
                        + " nodes=" + next.nodes.size());
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
                if (!changedContext && !selectionChanged) {
                    ReaderSnapshot.Node previousField = snapshot.find(focus);
                    ReaderSnapshot.Node nextField = next.find(focus);
                    if (previousField != null && nextField != null && !nextField.fieldType.isEmpty()
                            && (previousField.hasDraft != nextField.hasDraft
                                || !previousField.draftValue.equals(nextField.draftValue)
                                || !previousField.appliedValue.equals(nextField.appliedValue)
                                || !previousField.fieldError.equals(nextField.fieldError))) {
                        selectedItemFocus = focus;
                        selectionChanged = true;
                    }
                }
                // Native menus may visually keep the parent tabs in place,
                // but their semantic snapshot now publishes only the active
                // layer. Switching between tab and child must use the new
                // snapshot's first item rather than an unrelated old sibling.
                boolean nativeLayerChanged = !changedContext
                        && "MAIN_MENU".equals(next.category) && snapshot != null
                        && !snapshot.nodes.isEmpty() && !next.nodes.isEmpty()
                        && (snapshot.nodes.get(0).id.startsWith("tab:")
                                != next.nodes.get(0).id.startsWith("tab:"));
                String previous = commandPaletteOpen && !commandPaletteReturnFocus.isEmpty()
                        ? commandPaletteReturnFocus : focus;
                ReaderSnapshot previousSnapshot = snapshot;
                if (changedContext) {
                    rememberScreenFocus(previousSnapshot, previous);
                }
                snapshot = next;
                lastLoadingStatus = "";
                speech.setContext(next.contextKey());
                replayBufferedGameMessages();
                boolean enteredTreeLayer = false;
                if (changedContext) {
                    commandPaletteOpen = false;
                    commandPaletteReturnFocus = "";
                    commandPaletteReturnParent = "";
                    readerLayerParent = "";
                    pendingReaderLayer = "";
                    pendingReaderLayerRevisions = 0;
                } else if (!pendingReaderLayer.isEmpty()) {
                    ReaderSnapshot.Node parent = next.find(pendingReaderLayer);
                    ReaderSnapshot.Node child = parent == null ? null
                            : firstDirectChild(next, parent.id);
                    if (parent != null && parent.expanded && child != null) {
                        readerLayerParent = parent.id;
                        selectedItemFocus = child.id;
                        selectionChanged = true;
                        enteredTreeLayer = true;
                        pendingReaderLayer = "";
                        pendingReaderLayerRevisions = 0;
                    } else if (parent == null || ++pendingReaderLayerRevisions > 8) {
                        // Publication may race ahead of the engine's actual
                        // expand/tab intent. Never drop it after one stale
                        // snapshot, but bound the pending focus request.
                        pendingReaderLayer = "";
                        pendingReaderLayerRevisions = 0;
                    }
                }
                if (!readerLayerParent.isEmpty()) {
                    ReaderSnapshot.Node currentParent = next.find(readerLayerParent);
                    if (currentParent == null || !currentParent.expanded
                            || firstDirectChild(next, readerLayerParent) == null) {
                        readerLayerParent = currentParent == null ? ""
                                : currentParent.parentId;
                    }
                }
                if (commandPaletteOpen && !changedContext && next.find(focus) != null) {
                    // A screen update must not steal focus from an open command drawer.
                    readerLayerParent = "";
                } else if (nativeLayerChanged) {
                    focus = next.find(next.focus) != null ? next.focus
                            : next.nodes.get(0).id;
                } else if (selectionChanged) {
                    focus = selectedItemFocus;
                } else if (changedContext) {
                    String remembered = savedScreenFocus.get(nativeScreenKey(next));
                    focus = remembered != null && next.find(remembered) != null
                            ? remembered : next.find(next.focus) != null ? next.focus
                            : next.nodes.isEmpty() ? "" : next.nodes.get(0).id;
                } else if (next.find(focus) == null) {
                    // A vanished item returns to its closest surviving
                    // ancestor/sibling, never a stale native screen.
                    focus = resolveFocusFallback(previousSnapshot, previous, next);
                }
                // The engine may initially focus a hidden command. Start on
                // primary content instead of reading the long command tail.
                ReaderSnapshot.Node focusedNode = next.find(focus);
                if (!commandPaletteOpen && focusedNode != null
                        && isAuxiliaryCommand(focusedNode)) {
                    List<ReaderSnapshot.Node> primary = visibleNodes();
                    if (!primary.isEmpty() && !isAuxiliaryCommand(primary.get(0))) {
                        focus = primary.get(0).id;
                    }
                }
                ReaderSnapshot.Node currentFocus = next.find(focus);
                if (currentFocus != null && !currentFocus.parentId.equals(readerLayerParent)) {
                    // The engine may open a new tree branch directly, or
                    // select a new child after a modal changes state.
                    readerLayerParent = currentFocus.parentId;
                }
                if (changedContext) {
                    // The game context now determines the gesture focus.
                    // A mode change kills any gesture still in flight
                    // (R05.3): the surface owner string also changes with
                    // the context, and cancelling here makes the kill
                    // explicit instead of relying on that check alone.
                    root.cancelGesture();
                    editor = null;
                    editorRequest = 0;
                    formRequest = 0;
                    // Document sessions are per screen context (R04.4);
                    // the saved position is kept for a later visit.
                    documentReader.clear();
                }
                boolean enteredMenu = nativeLayerChanged || enteredTreeLayer
                        || (changedContext && next.find("world:here") == null);
                ReaderSnapshot.Node nextTile = next.find("world:here");
                ReaderSnapshot.Node previousTile = previousSnapshot == null ? null
                        : previousSnapshot.find("world:here");
                boolean worldView = nextTile != null;
                if (comprehensiveMenu) {
                    if (!worldView) {
                        // A native modal supersedes the action hub; never
                        // keep stale commands over a different game screen.
                        comprehensiveMenu = false;
                        choosingGesture = false;
                        bindingItem = null;
                        choosingHubAction = false;
                        hubActionItem = null;
                    } else {
                        render();
                        return;
                    }
                }
                // Ordinary floor-to-floor footsteps are silent. A newly
                // entered notable tile speaks its observed objects or hazards.
                // Position matters: two adjacent floor tiles holding the same
                // item must each announce when the avatar enters them.
                ReaderSnapshot.Node previousPosition = previousSnapshot == null ? null
                        : previousSnapshot.find("world:tile_position");
                ReaderSnapshot.Node nextPosition = next.find("world:tile_position");
                boolean steppedToAnotherTile = previousPosition != null && nextPosition != null
                        && !previousPosition.value.equals(nextPosition.value);
                boolean newlyNoteworthy = worldView && previousTile != null
                        && "alert".equals(nextTile.role)
                        && (steppedToAnotherTile || !"alert".equals(previousTile.role)
                            || !nextTile.value.equals(previousTile.value));
                render();
                if (worldView) {
                    if (newlyNoteworthy) { feedback(nextTile.value); }
                } else if (enteredMenu && next.find(focus) != null) {
                    // YESNO choices are meaningless without the question.
                    // Announce it once on entry, not again for every swipe.
                    String introduction = next.title;
                    if ("YESNO".equals(next.category) && !next.summary.trim().isEmpty()) {
                        introduction += ". " + next.summary.trim();
                    }
                    // Places intentionally starts speaking at the first actual
                    // destination: no title, preamble, or item counter.
                    if ("places".equals(next.category)
                            || "nearby-target".equals(next.category)
                            || "pickup-here".equals(next.category)) {
                        feedback(nodeText(next.find(focus), false));
                    } else {
                        feedback(introduction + ". " + nodeText(next.find(focus), false));
                    }
                } else if (selectionChanged) {
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
        final boolean loading = message.startsWith("A11Y_LOADING:");
        final boolean extendedScan = message.startsWith("A11Y_SCAN:");
        final String spokenMessage = loading ? message.substring("A11Y_LOADING:".length())
                : extendedScan ? message.substring("A11Y_SCAN:".length()) : message;
        final int level = priority >= ReaderSpeechQueue.URGENT
                ? ReaderSpeechQueue.URGENT : ReaderSpeechQueue.INFORMATION;
        handler.post(() -> {
            if (closed || !foreground || !windowFocused || !preferences.isEnabled()) {
                return;
            }
            if (snapshot == null && !loading) {
                // An action intentionally publishes an inactive snapshot
                // while the native command executes. In the supplied log,
                // "Nothing to open here" arrived in exactly this gap.
                if (pendingGameMessages.size() >= MAX_BUFFERED_MESSAGES) {
                    BufferedGameMessage discarded = pendingGameMessages.removeFirst();
                    SessionDebugLog.event("GAME_MESSAGE_DROPPED",
                            "reason=overflow text=" + discarded.text);
                }
                pendingGameMessages.addLast(new BufferedGameMessage(message, priority));
                SessionDebugLog.event("GAME_MESSAGE_BUFFERED", "text=" + message);
                return;
            }
            if (loading) {
                // This is a temporary progress notification, not a game node.
                // It must be audible precisely when the old screen is inactive.
                lastLoadingStatus = spokenMessage;
                        String statusText = activity.getString(R.string.a11yLoadingStatus, spokenMessage);
                if (usesTouchExploration()) {
                    status.setText(statusText);
                    announceAccessibility(statusText);
                } else {
                    speech.say("loading-progress", statusText,
                            ReaderSpeechQueue.INFORMATION, 30000);
                }
                ReaderDiagnostics.get().event("loading_progress");
                return;
            }
            ReaderDiagnostics.get().count(level == ReaderSpeechQueue.URGENT
                    ? "announce_urgent" : "announce_info");
            if (usesTouchExploration()) {
                status.setText(spokenMessage);
                announceAccessibility(spokenMessage);
            } else if (extendedScan) {
                // Reading sixteen unrestricted map cells may outlive the
                // eight-second event TTL; use the document-length channel.
                speech.say("world-scan:" + ANNOUNCEMENT_SEQUENCE.incrementAndGet(),
                        spokenMessage, ReaderSpeechQueue.LONG_TEXT);
            } else {
                // Every new game message has priority over currently
                // focused text: waiting behind navigation speech and an
                // eight-second TTL previously silenced ordinary messages
                // such as "Nothing to open here." Urgent events still win.
                final int messageLevel = level == ReaderSpeechQueue.URGENT
                        ? ReaderSpeechQueue.URGENT : ReaderSpeechQueue.GAME_EVENT;
                SessionDebugLog.event("GAME_MESSAGE_POLICY", "priority=" + messageLevel
                        + " text=" + message);
                speech.say("game-message:" + ANNOUNCEMENT_SEQUENCE.incrementAndGet(), message,
                        messageLevel, 15000);
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
        // Inventory quantity rows already say how many are selected. Adding
        // "checked/unchecked" repeats the same fact on every swipe.
        boolean quantityRow = snapshot != null && "inventory".equals(snapshot.category)
                && "checkbox".equals(node.role) && !node.fieldType.isEmpty();
        if (!quantityRow) {
            if (node.checked) { text.append(". ").append(activity.getString(R.string.a11yChecked)); }
            else if ("checkbox".equals(node.role)) {
                text.append(". ").append(activity.getString(R.string.a11yUnchecked));
            }
        }
        // Selection is already represented by focus. Do not say "selected"
        // on every swipe; the full state is still available on Detail.
        if (includeDetail && node.selected) {
            text.append(". ").append(activity.getString(R.string.a11ySelected));
        }
        // Position and total are available on explicit Detail, but repeatedly
        // announcing "item 5 of 40" makes quick inventory/menu browsing noisy.
        if (includeDetail && node.position > 0 && node.total > 0) {
            text.append(", ").append(activity.getString(
                    R.string.a11yPosition, node.position, node.total));
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

    private static String nativeScreenKey(ReaderSnapshot data) {
        return data.session + ":" + data.frame;
    }

    private void rememberScreenFocus(ReaderSnapshot data, String nodeId) {
        if (data == null || data.find(nodeId) == null) { return; }
        String key = nativeScreenKey(data);
        savedScreenFocus.remove(key);
        savedScreenFocus.put(key, nodeId);
        if (savedScreenFocus.size() > 64) {
            savedScreenFocus.remove(savedScreenFocus.keySet().iterator().next());
        }
    }

    private static ReaderSnapshot.Node firstDirectChild(ReaderSnapshot data, String parent) {
        for (ReaderSnapshot.Node node : data.nodes) {
            if (parent.equals(node.parentId)) { return node; }
        }
        return null;
    }

    /** Only auxiliary screen controls go into the hold menu, never dialogue choices. */
    private static boolean isAuxiliaryCommand(ReaderSnapshot.Node node) {
        return "command".equals(node.role) || node.id.startsWith("group:")
                || node.id.startsWith("action:") || node.id.startsWith("category:")
                || "filter".equals(node.id) || "filter:clear".equals(node.id);
    }

    private List<ReaderSnapshot.Node> contextualCommands() {
        ArrayList<ReaderSnapshot.Node> result = new ArrayList<>();
        if (snapshot != null) {
            for (ReaderSnapshot.Node node : snapshot.nodes) {
                if (node.parentId.isEmpty() && isAuxiliaryCommand(node)) {
                    result.add(node);
                }
            }
        }
        return result;
    }

    private List<ReaderSnapshot.Node> visibleNodes() {
        ArrayList<ReaderSnapshot.Node> result = new ArrayList<>();
        if (snapshot == null) { return result; }
        if (commandPaletteOpen) { return contextualCommands(); }
        for (ReaderSnapshot.Node node : snapshot.nodes) {
            if (readerLayerParent.equals(node.parentId)) { result.add(node); }
        }
        // Command-only screens keep their complete list. A mixed screen
        // announces primary content first and moves all commands to long press.
        boolean containsContent = false;
        for (ReaderSnapshot.Node node : result) {
            if (!isAuxiliaryCommand(node)) { containsContent = true; break; }
        }
        if (containsContent) {
            result.removeIf(GameReaderController::isAuxiliaryCommand);
        }
        return result;
    }

    @Override
    public void openContextActions() {
        if (debugReport != null || closed) { return; }
        if (commandPaletteOpen) { closeContextActions(); return; }
        if (comprehensiveMenu) {
            showHubActionChoices();
            return;
        }
        if (editor != null || snapshot == null || directWorldGesturesEnabled()) { return; }
        List<ReaderSnapshot.Node> commands = contextualCommands();
        boolean hasContent = false;
        for (ReaderSnapshot.Node node : snapshot.nodes) {
            if (node.parentId.equals(readerLayerParent) && !isAuxiliaryCommand(node)) {
                hasContent = true;
                break;
            }
        }
        if (!hasContent || commands.isEmpty()) {
            feedback(activity.getString(R.string.a11yNoContextCommands));
            return;
        }
        commandPaletteReturnFocus = focus;
        commandPaletteReturnParent = readerLayerParent;
        commandPaletteOpen = true;
        readerLayerParent = "";
        focus = commands.get(0).id;
        root.cancelGesture();
        render();
        feedback(activity.getString(R.string.a11yContextCommands) + ". "
                + nodeText(commands.get(0), false));
    }

    private void closeContextActions() {
        if (!commandPaletteOpen) { return; }
        commandPaletteOpen = false;
        readerLayerParent = commandPaletteReturnParent;
        if (snapshot != null && snapshot.find(commandPaletteReturnFocus) != null) {
            focus = commandPaletteReturnFocus;
        } else if (snapshot != null && snapshot.find(snapshot.focus) != null) {
            focus = snapshot.focus;
        }
        commandPaletteReturnFocus = "";
        commandPaletteReturnParent = "";
        root.cancelGesture();
        render();
        readCurrent();
    }

    private boolean enterReaderLayer(ReaderSnapshot.Node parent) {
        ReaderSnapshot.Node first = snapshot == null ? null
                : firstDirectChild(snapshot, parent.id);
        if (first == null) { return false; }
        readerLayerParent = parent.id;
        pendingReaderLayer = "";
        pendingReaderLayerRevisions = 0;
        focus = first.id;
        root.cancelGesture();
        render();
        feedback(parent.label + ". " + nodeText(first, false));
        return true;
    }

    /** Close one child layer, or defer to its owning native modal. */
    private boolean backReaderLayer() {
        if (!readerLayerParent.isEmpty() && snapshot != null) {
            ReaderSnapshot.Node parent = snapshot.find(readerLayerParent);
            readerLayerParent = parent == null ? "" : parent.parentId;
            pendingReaderLayer = "";
            pendingReaderLayerRevisions = 0;
            focus = parent == null ? snapshot.focus : parent.id;
            root.cancelGesture();
                render();
            readCurrent();
            return true;
        }
        if (snapshot != null && "MAIN_MENU".equals(snapshot.category)
                && (focus.startsWith("item:") || focus.startsWith("document:"))) {
            send("cancel", "");
            return true;
        }
        return false;
    }

    private void renderContextCommands() {
        TextView heading = new TextView(activity);
        heading.setText(R.string.a11yContextCommands);
        heading.setTextColor(0xffeeeeee);
        heading.setTextSize(20);
        rows.addView(heading);
        Button close = new Button(activity);
        close.setText(R.string.a11yContextCommandsClose);
        close.setOnClickListener(view -> closeContextActions());
        rows.addView(close);
        List<ReaderSnapshot.Node> commands = contextualCommands();
        int selected = 0;
        for (int i = 0; i < commands.size(); i++) {
            if (commands.get(i).id.equals(focus)) { selected = i; break; }
        }
        final int start = (selected / HUB_PAGE_SIZE) * HUB_PAGE_SIZE;
        final int end = Math.min(start + HUB_PAGE_SIZE, commands.size());
        if (start > 0) {
            Button previous = new Button(activity);
            previous.setText(R.string.a11yPreviousPage);
            previous.setOnClickListener(view -> {
                focus = commands.get(start - 1).id;
                render();
                readCurrent();
            });
            rows.addView(previous);
        }
        for (int i = start; i < end; i++) {
            ReaderSnapshot.Node command = commands.get(i);
            Button option = new Button(activity);
            option.setText(nodeText(command, false));
            option.setSelected(i == selected);
            option.setEnabled(command.enabled);
            option.setOnClickListener(view -> {
                if (!commandPaletteOpen || snapshot == null || snapshot.find(command.id) == null) {
                    return;
                }
                focus = command.id;
                activate();
            });
            rows.addView(option);
        }
        if (end < commands.size()) {
            Button next = new Button(activity);
            next.setText(R.string.a11yNextPage);
            next.setOnClickListener(view -> {
                focus = commands.get(end).id;
                render();
                readCurrent();
            });
            rows.addView(next);
        }
    }

    private void render() {
        if (closed) { return; }
        rows.removeAllViews();
        if (debugReport != null) {
            root.setBackgroundColor(0xff161616);
            TextView report = new TextView(activity);
            report.setText(activity.getString(R.string.a11yDebugPromptTitle) + "\n" + debugReport);
            report.setTextColor(0xffeeeeee);
            report.setTextSize(16);
            report.setPadding(12, 12, 12, 18);
            report.setContentDescription(report.getText());
            rows.addView(report);
            final int[] labels = {
                R.string.a11yDebugPromptContinue,
                R.string.a11yDebugPromptIgnore,
                R.string.a11yDebugPromptCopy
            };
            for (int i = 0; i < labels.length; i++) {
                final int action = i + 1;
                Button button = new Button(activity);
                button.setText(labels[i]);
                button.setOnClickListener(v -> submitDebugResponse(action));
                rows.addView(button);
            }
            return;
        }
        // Native text entry is still needed for character names and game fields.
        if (editor != null) {
            root.setBackgroundColor(0xff161616);
            rows.addView(editor.getView());
        } else if (comprehensiveMenu) {
            root.setBackgroundColor(0xff161616);
            renderHub();
        } else if (commandPaletteOpen) {
            root.setBackgroundColor(0xff161616);
            renderContextCommands();
        } else {
            root.setBackgroundColor(android.graphics.Color.TRANSPARENT);
            // Touch exploration lets TalkBack intercept multi-finger gestures.
            // Offer the same actions as explicit native accessibility buttons.
            if (usesTouchExploration() && snapshot != null && !directWorldGesturesEnabled()) {
                Button commands = new Button(activity);
                commands.setText(R.string.a11yContextCommands);
                commands.setOnClickListener(view -> openContextActions());
                commands.setOnLongClickListener(view -> {
                    openContextActions();
                    return true;
                });
                rows.addView(commands);
                Button details = new Button(activity);
                details.setText(R.string.a11yGestureActionDetail);
                details.setOnClickListener(view -> detail());
                rows.addView(details);
                Button accept = new Button(activity);
                accept.setText(R.string.a11yContextConfirm);
                accept.setOnClickListener(view -> confirm());
                rows.addView(accept);
                Button back = new Button(activity);
                back.setText(R.string.a11yContextBack);
                back.setOnClickListener(view -> goBack());
                rows.addView(back);
            }
        }
    }

    @Override
    public boolean usesTouchExploration() {
        return accessibility != null && accessibility.isEnabled() && accessibility.isTouchExplorationEnabled();
    }

    @Override
    public String gestureContext() {
        if (debugReport != null) { return presentationEpoch + ":debug"; }
        if (comprehensiveMenu) {
            return presentationEpoch + ":hub:" + hubSection + ":" + choosingGesture
                    + ":" + choosingHubAction
                    + ":" + (bindingItem == null ? "" : bindingItem.shortcutTarget())
                    + ":" + (hubActionItem == null ? "" : hubActionItem.shortcutTarget())
                    + (editor == null ? "" : ":editor:" + editor.contextKey());
        }
        return presentationEpoch + ":" + (snapshot == null ? "waiting"
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
        SessionDebugLog.event("NAVIGATE", "delta=" + delta + " focus=" + focus
                + " hub=" + comprehensiveMenu);
        if (delta == 0) { readCurrent(); return; }
        if (debugReport != null) {
            debugFocus = Math.floorMod(debugFocus + delta, 4);
            readCurrent();
            return;
        }
        if (editor != null) { editor.navigate(delta); return; }
        if (comprehensiveMenu) {
            if (choosingHubAction) {
                hubActionChoice = Math.floorMod(hubActionChoice + delta, 2);
            } else if (choosingGesture) {
                gestureChoice = Math.floorMod(gestureChoice + delta,
                        ReaderGestureBindings.IDS.length + 2);
            } else {
                List<GameActionHub.Entry> options = hubEntries();
                if (options.isEmpty()) { return; }
                hubIndex = Math.floorMod(hubIndex + delta, options.size());
            }
            render();
            readCurrent();
            return;
        }
        List<ReaderSnapshot.Node> filtered = visibleNodes();
        if (filtered.isEmpty()) { return; }
        // All spoken lists wrap both ways: end -> first, first -> end.
        // This also applies to the contextual command drawer and nested lists.
        int selected = Math.floorMod(index() + delta, filtered.size());
        focus = filtered.get(selected).id;
        render();
        // Command focus changes are local: native command scopes do not
        // handle focus intents and would report unsupported for every swipe.
        if (!isAuxiliaryCommand(filtered.get(selected))) { send("focus", ""); }
        readCurrent();
    }

    @Override
    public boolean adjustFocusedSetting(int delta) {
        if (debugReport != null || comprehensiveMenu) { return false; }
        if (editor != null || snapshot == null || delta == 0
                || !foreground || !windowFocused || lastSuspended) { return false; }
        ReaderSnapshot.Node node = snapshot.find(focus);
        if (node == null || !node.enabled) { return false; }

        // These are the same semantic choices used by game settings pages.
        if ("choice".equals(node.fieldType) && node.actions.contains("select_choice")
                && !node.choices.isEmpty()) {
            int current = -1;
            for (int i = 0; i < node.choices.size(); i++) {
                if (node.choices.get(i).value.equals(node.effectiveRawValue())) {
                    current = i;
                    break;
                }
            }
            int next = Math.max(0, Math.min(node.choices.size() - 1, current + delta));
            if (next == current) { readCurrent(); }
            else { send("select_choice", node.choices.get(next).value); }
            return true;
        }

        // Only a declared numeric form range with a positive step may be
        // adjusted. No guessing units, bounds or text-field semantics.
        if (!("int".equals(node.fieldType) || "float".equals(node.fieldType))
                || !node.actions.contains("set_value") || !node.hasFieldRange
                || !(node.fieldStep > 0) || !Double.isFinite(node.fieldStep)
                || !Double.isFinite(node.fieldMin) || !Double.isFinite(node.fieldMax)) {
            return false;
        }
        try {
            BigDecimal current = new BigDecimal(node.effectiveRawValue());
            BigDecimal minimum = BigDecimal.valueOf(node.fieldMin);
            BigDecimal maximum = BigDecimal.valueOf(node.fieldMax);
            BigDecimal step = BigDecimal.valueOf(node.fieldStep);
            BigDecimal candidate = current.add(step.multiply(BigDecimal.valueOf(delta)));
            if (candidate.compareTo(minimum) < 0 || candidate.compareTo(maximum) > 0) {
                readCurrent();
                return true;
            }
            if ("int".equals(node.fieldType)) {
                candidate = candidate.stripTrailingZeros();
                if (candidate.scale() > 0) {
                    // Never submit a fractional draft to an integer field.
                    readCurrent();
                    return true;
                }
            }
            send("set_value", candidate.stripTrailingZeros().toPlainString());
        } catch (NumberFormatException invalidValue) {
            readCurrent();
        }
        return true;
    }

    @Override
    public void goBack() { onBackPressed(); }

    @Override
    public void confirm() {
        if (closed || debugReport != null) { return; }
        if (editor != null) {
            editor.submitCurrent();
            return;
        }
        if (comprehensiveMenu || commandPaletteOpen) {
            activate();
            return;
        }
        if (snapshot == null || directWorldGesturesEnabled()) { return; }
        ReaderSnapshot.Node chosen = snapshot.find(focus);
        if (chosen == null) { return; }
        // Native explicit approval takes priority over the focused "No" option.
        if ("YESNO".equals(snapshot.category) || "YESNOQUIT".equals(snapshot.category)) {
            for (ReaderSnapshot.Node candidate : snapshot.nodes) {
                String label = candidate.label.trim().toLowerCase(Locale.ROOT);
                if (candidate.enabled && candidate.actions.contains("activate")
                        && ("option:0".equals(candidate.id) || "YES".equals(candidate.id)
                            || "action:YES".equals(candidate.id)
                            || label.equals("yes") || label.equals("có")
                            || label.startsWith("yes ") || label.startsWith("có "))) {
                    focus = candidate.id;
                    send("activate", "");
                    return;
                }
            }
            feedback(activity.getString(R.string.a11yContextConfirmUnavailable));
            return;
        }
        if (chosen.hasDraft && chosen.actions.contains("apply")) {
            send("apply", "");
            return;
        }
        for (String id : new String[] { "CONFIRM", "TEXT.CONFIRM", "confirm",
                                        "action:CONFIRM", "pickup-here:confirm" }) {
            ReaderSnapshot.Node accept = snapshot.find(id);
            if (accept != null && accept.enabled && accept.actions.contains("activate")) {
                String previous = focus;
                focus = accept.id;
                send("activate", "");
                focus = previous;
                return;
            }
        }
        // A list selection is the only safe implicit confirmation. Never
        // fire arbitrary world commands or status/document nodes.
        if (chosen.enabled && chosen.actions.contains("activate")
                && ("listitem".equals(chosen.role)
                    || chosen.id.startsWith("option:")
                    || chosen.id.startsWith("response:")
                    || "dialogue".equals(snapshot.category)
                    || "MAIN_MENU".equals(snapshot.category))) {
            activate();
        } else {
            feedback(activity.getString(R.string.a11yContextConfirmUnavailable));
        }
    }

    @Override
    public boolean movementPracticeEnabled() {
        return false;
    }

    @Override
    public void practiceMovementDirection(int dx, int dy) {
        // The former training panel no longer exists.
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
        // Native gameplay owns the safety gate (no vehicle, automation,
        // sleep, modal or death camera). No practice or opt-in is required.
        return directWorldGesturesEnabled() && snapshot.swipeMovementAllowed
                && hasCardinalDirections(snapshot);
    }

    private static boolean hasCardinalDirections(ReaderSnapshot snapshot) {
        for (String direction : new String[] { "UP", "DOWN", "LEFT", "RIGHT" }) {
            if (snapshot.find(direction) == null) { return false; }
        }
        return true;
    }

    @Override
    public boolean directWorldGesturesEnabled() {
        return debugReport == null && !comprehensiveMenu && !closed && foreground
                && windowFocused && !lastSuspended && editor == null
                && snapshot != null && snapshot.find("world:here") != null
                && snapshot.find("reader:coordinates") != null;
    }

    private void requestLiveWorldCommand(String id) {
        if (!directWorldGesturesEnabled() || snapshot.find(id) == null) { return; }
        String previous = focus;
        focus = id;
        send("activate", "");
        focus = previous;
    }

    @Override
    public boolean handleCustomGesture(String gestureId) {
        if (!directWorldGesturesEnabled()) { return false; }
        String target = gestureBindings.targetFor(gestureId);
        SessionDebugLog.event("GESTURE_BINDING", "gesture=" + gestureId
                + " target=" + target);
        if (target == null) { return false; }
        // Assignments override the built-in gesture only in the live world.
        // Do not fall through to the default action on stale/unavailable nodes.
        executeShortcutTarget(target);
        return true;
    }

    @Override
    public void unassignedMultiGesture(String gestureId) {
        if (directWorldGesturesEnabled()) {
            feedback(activity.getString(R.string.a11yGestureUnassigned));
        }
    }

    private void executeShortcutTarget(String target) {
        if (target.startsWith("action:")) {
            String id = target.substring("action:".length());
            ReaderSnapshot.Node node = snapshot == null ? null : snapshot.find(id);
            if (node == null || !node.enabled || (!node.actions.contains("activate")
                    && !"document".equals(node.role))) {
                feedback(activity.getString(R.string.a11yBindUnavailable));
                return;
            }
            String previous = focus;
            focus = id;
            activate();
            focus = previous;
            return;
        }
        if (target.startsWith("hub:group:")) {
            try {
                int section = Integer.parseInt(target.substring("hub:group:".length()));
                if (section > 0 && section < GameActionHub.TITLES.length) {
                    openHubSection(section);
                    return;
                }
            } catch (NumberFormatException ignored) {
                // Unknown old custom binding: do not execute an unrelated command.
            }
        } else if ("hub:directions".equals(target)) {
            openHubSection(GameActionHub.DIRECTIONS);
            hubRootIndex = 3;
            return;
        } else if ("hub:favorites".equals(target)) {
            openHubSection(GameActionHub.FAVORITES);
            hubRootIndex = 0;
            return;
        } else if ("hub:search".equals(target)) {
            showComprehensiveMenu();
            if (comprehensiveMenu) { searchHub(); }
            return;
        } else if ("hub:all".equals(target)) {
            showComprehensiveMenu();
            if (comprehensiveMenu) {
                hubSection = GameActionHub.SEARCH;
                hubQuery = "";
                hubIndex = 0;
                render();
                readCurrent();
            }
            return;
        } else if ("hub:tts".equals(target)) {
            activity.onAccessibilityOpenTtsSettings();
            return;
        } else if ("hub:reader_setup".equals(target)) {
            activity.startActivity(new Intent(activity, ReaderSetupActivity.class));
            return;
        }
        feedback(activity.getString(R.string.a11yBindUnavailable));
    }

    private void openHubSection(int section) {
        showComprehensiveMenu();
        if (!comprehensiveMenu) { return; }
        hubSection = section;
        hubRootIndex = section;
        hubIndex = 0;
        root.cancelGesture();
        render();
        readCurrent();
    }

    @Override
    public void scanDirection(int dx, int dy) {
        SessionDebugLog.event("GESTURE_ACTION", "scan dx=" + dx + " dy=" + dy);
        if (Math.abs(dx) + Math.abs(dy) != 1) { return; }
        String id = dy < 0 ? "reader:scan:north" : dy > 0 ? "reader:scan:south"
                : dx < 0 ? "reader:scan:west" : "reader:scan:east";
        requestLiveWorldCommand(id);
    }

    @Override
    public void twoFingerTap(int count) {
        if (count == 1) { detail(); return; }
        if (!directWorldGesturesEnabled()) { return; }
        if (count == 3) { openActionMenu(); return; }
        ReaderSnapshot.Node document = snapshot.find(count == 1
                ? "world:status" : "world:messages");
        String spoken = document == null ? "" : document.detail;
        if (spoken.isEmpty()) {
            spoken = count == 2 ? activity.getString(R.string.a11yHubNoMessages)
                    : activity.getString(R.string.a11yReaderWaiting);
        }
        if (usesTouchExploration()) {
            feedback(spoken);
        } else {
            speech.say("world-two-tap:" + count, spoken, ReaderSpeechQueue.LONG_TEXT);
        }
    }

    @Override
    public void threeFingerTap() {
        requestLiveWorldCommand("reader:coordinates");
    }

    @Override
    public void openActionMenu() {
        // Two fingers, three taps: the approved 16-section action hub.
        showComprehensiveMenu();
    }

    @Override
    public void moveDirection(int dx, int dy) {
        SessionDebugLog.event("GESTURE_ACTION", "move dx=" + dx + " dy=" + dy
                + " enabled=" + worldMovementEnabled());
        if (!worldMovementEnabled()) { return; }
        String action = actionForDirection(dx, dy);
        ReaderSnapshot.Node node = snapshot.find(action);
        if (node == null || !node.enabled) {
            feedback(activity.getString(R.string.a11yFastMovementUnavailable));
            return;
        }
        String previous = focus;
        focus = action;
        send("activate", "");
        // Leave the reading cursor on the tile for an explicit long press.
        focus = previous;
    }

    @Override
    public void activate() {
        SessionDebugLog.event("ACTION", "activate focus=" + focus
                + " hub=" + comprehensiveMenu);
        if (debugReport != null) {
            if (debugFocus == 0) { readCurrent(); }
            else { submitDebugResponse(debugFocus); }
            return;
        }
        if (editor != null) { editor.activate(); return; }
        if (comprehensiveMenu) {
            if (choosingHubAction) { applyHubActionChoice(); }
            else if (choosingGesture) { applyGestureChoice(); }
            else { activateHub(); }
            return;
        }
        ReaderSnapshot.Node node = snapshot == null ? null : snapshot.find(focus);
        if (node == null) { return; }
        if (!node.enabled) { feedback(nodeText(node, true)); return; }
        // Command group headers (R04.3) are browsing landmarks only:
        // activating one reads it, it never sends an intent.
        if (node.id.startsWith("group:")) {
            feedback(activity.getString(R.string.a11yCommandGroup) + ". " + node.label);
            return;
        }
        // Enter the child list, not an interleaved parent+children tree.
        // Native expansion remains validated by the game before entry.
        if (node.hasChildren) {
            if (node.expanded && enterReaderLayer(node)) { return; }
            if (node.actions.contains("expand")) {
                pendingReaderLayer = node.id;
                pendingReaderLayerRevisions = 0;
                send("expand", "");
                return;
            }
        }
        // A tab can have *no published children yet* when it is inactive,
        // so its hasChildren flag may be false. Remember the requested tab
        // before native activates it, then enter its first newly published
        // child in onSnapshot().
        if ("tab".equals(node.role) && node.actions.contains("activate")) {
            pendingReaderLayer = node.id;
            pendingReaderLayerRevisions = 0;
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
        if ("document".equals(node.role) || "world:here".equals(node.id)) {
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

    private long send(String action, String value) {
        ReaderSnapshot owner = snapshot;
        // The closed check is explicit, not implied by the lifecycle flags:
        // after close() no intent of this controller may reach the game,
        // even if a stray lifecycle callback ran first (R03.4).
        if (closed || owner == null || !foreground || !windowFocused || lastSuspended) { return 0; }
        final long request = REQUEST_SEQUENCE.incrementAndGet();
        long begun = SystemClock.elapsedRealtime();
        // Text and number editors can contain user-authored/private values;
        // record action metadata, not the typed contents.
        SessionDebugLog.event("INTENT", "id=" + request + " screen="
                + owner.category + " node=" + focus + " action=" + action
                + " value=" + ("focus".equals(action) || "activate".equals(action)
                        ? "<none>" : "<redacted>"));
        try {
            if (!CataclysmDDA.nativeReaderIntent(owner.session, owner.frame, owner.generation,
                    owner.revision, request, focus, action, value)) {
                SessionDebugLog.event("INTENT_RESULT", "id=" + request
                        + " accepted=false elapsed_ms="
                        + (SystemClock.elapsedRealtime() - begun));
                // Focus intents are re-sent on every navigation and race
                // screen changes routinely; a rejected one is dropped
                // silently instead of announcing a stale entry (R03.1).
                if (!"focus".equals(action)) {
                    feedback(activity.getString(R.string.a11yReaderStale));
                }
            } else {
                if (pendingIntentTimes.size() > 64) {
                    pendingIntentTimes.remove(pendingIntentTimes.keySet().iterator().next());
                }
                pendingIntentTimes.put(request, begun);
                SessionDebugLog.event("INTENT_RESULT", "id=" + request
                        + " accepted=true send_ms="
                        + (SystemClock.elapsedRealtime() - begun));
                return request;
            }
        } catch (UnsatisfiedLinkError unavailable) {
            SessionDebugLog.event("ERROR", "JNI nativeReaderIntent " + unavailable);
            feedback(activity.getString(R.string.a11yReaderNativeUnavailable));
        }
        return 0;
    }


    private List<GameActionHub.Entry> hubEntries() {
        return GameActionHub.entries(activity, snapshot, hubSection, hubQuery, favorites);
    }

    private String hubActionLabel(int choice) {
        if (choice == 0) { return activity.getString(R.string.a11yHubGestureOption); }
        String target = hubActionItem == null ? null : hubActionItem.shortcutTarget();
        return activity.getString(target != null && favorites.contains(target)
                ? R.string.a11yHubRemoveFavorite : R.string.a11yHubAddFavorite);
    }

    private String hubSpoken(List<GameActionHub.Entry> entries, int selected) {
        // Repeated menu browsing speaks the command name only.
        return entries.get(selected).label;
    }

    private String gestureChoiceLabel(int index) {
        if (index == ReaderGestureBindings.IDS.length) {
            return activity.getString(R.string.a11yBindDefault);
        }
        if (index == ReaderGestureBindings.IDS.length + 1
                || index < 0 || index >= ReaderGestureBindings.IDS.length) {
            return activity.getString(R.string.a11yBindCancel);
        }
        return activity.getString(ReaderGestureBindings.LABELS[index]);
    }

    /** Only render this full-screen layer while the hub is explicitly open. */
    private void renderHub() {
        boolean binding = choosingGesture && bindingItem != null;
        String heading = choosingHubAction && hubActionItem != null
                ? activity.getString(R.string.a11yHubActionMenu, hubActionItem.label)
                : binding ? activity.getString(R.string.a11yBindTitle, bindingItem.label)
                : hubSection == GameActionHub.ROOT
                ? activity.getString(R.string.a11yHubTitle)
                : hubSection == GameActionHub.DIRECTIONS
                ? activity.getString(R.string.a11yHubChooseDirection)
                : hubSection == GameActionHub.FAVORITES
                ? activity.getString(R.string.a11yHubFavorites)
                : activity.getString(GameActionHub.TITLES[hubSection]);
        TextView titleView = new TextView(activity);
        titleView.setText(heading);
        titleView.setTextColor(0xffeeeeee);
        titleView.setTextSize(20);
        titleView.setPadding(12, 16, 12, 12);
        titleView.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
        rows.addView(titleView);

        List<GameActionHub.Entry> entries = binding || choosingHubAction ? null : hubEntries();
        int count = choosingHubAction ? 2
                : binding ? ReaderGestureBindings.IDS.length + 2 : entries.size();
        int selected = choosingHubAction ? hubActionChoice : binding ? gestureChoice : hubIndex;
        if (count == 0) {
            TextView empty = new TextView(activity);
            empty.setText(hubSection == GameActionHub.FAVORITES
                    ? R.string.a11yHubFavoritesEmpty : R.string.a11yHubEmpty);
            empty.setTextColor(0xffeeeeee);
            rows.addView(empty);
            return;
        }
        selected = Math.max(0, Math.min(selected, count - 1));
        if (choosingHubAction) { hubActionChoice = selected; }
        else if (binding) { gestureChoice = selected; }
        else { hubIndex = selected; }
        int start = (selected / HUB_PAGE_SIZE) * HUB_PAGE_SIZE;
        int end = Math.min(start + HUB_PAGE_SIZE, count);
        if (start > 0) {
            Button previous = new Button(activity);
            previous.setText(R.string.a11yPreviousPage);
            previous.setOnClickListener(view -> {
                if (choosingHubAction) { hubActionChoice = start - 1; }
                else if (binding) { gestureChoice = start - 1; }
                else { hubIndex = start - 1; }
                render();
                readCurrent();
            });
            rows.addView(previous);
        }
        for (int i = start; i < end; i++) {
            final int index = i;
            Button option = new Button(activity);
            option.setText(choosingHubAction ? hubActionLabel(i) + ", "
                    + activity.getString(R.string.a11yPosition, i + 1, count)
                    : binding ? gestureChoiceLabel(i) + ", "
                    + activity.getString(R.string.a11yPosition, i + 1, count)
                    : hubSpoken(entries, i));
            option.setSelected(i == selected);
            option.setOnClickListener(view -> {
                if (!comprehensiveMenu) { return; }
                if (choosingHubAction) {
                    hubActionChoice = index;
                    applyHubActionChoice();
                } else if (binding && choosingGesture) {
                    gestureChoice = index;
                    applyGestureChoice();
                } else if (!binding && !choosingGesture) {
                    hubIndex = index;
                    activateHub();
                }
            });
            if (!binding && !choosingHubAction) {
                option.setOnLongClickListener(view -> {
                    if (!comprehensiveMenu || choosingGesture || choosingHubAction) { return false; }
                    hubIndex = index;
                    showHubActionChoices();
                    return true;
                });
            }
            option.setOnFocusChangeListener((view, focused) -> {
                if (focused && comprehensiveMenu && !usesTouchExploration()) {
                    if (choosingHubAction) { hubActionChoice = index; }
                    else if (binding) { gestureChoice = index; }
                    else { hubIndex = index; }
                    readCurrent();
                }
            });
            rows.addView(option);
        }
        if (end < count) {
            Button next = new Button(activity);
            next.setText(R.string.a11yNextPage);
            next.setOnClickListener(view -> {
                if (choosingHubAction) { hubActionChoice = end; }
                else if (binding) { gestureChoice = end; }
                else { hubIndex = end; }
                render();
                readCurrent();
            });
            rows.addView(next);
        }
    }

    /** Opening the menu starts at Favorites; Search is the last root entry. */
    public void showComprehensiveMenu() {
        if (comprehensiveMenu || debugReport != null || !directWorldGesturesEnabled()) {
            return;
        }
        root.cancelGesture();
        comprehensiveMenu = true;
        choosingGesture = false;
        bindingItem = null;
        choosingHubAction = false;
        hubActionItem = null;
        hubSection = GameActionHub.ROOT;
        hubIndex = 0;
        hubRootIndex = 0;
        hubQuery = "";
        speech.stop();
        render();
        feedback(activity.getString(R.string.a11yHubTitle) + ". "
                + hubSpoken(hubEntries(), 0));
    }

    private void searchHub() {
        if (!comprehensiveMenu || editor != null) { return; }
        editor = new ReaderTextEditor(activity, hubQuery, false,
                new ReaderTextEditor.Listener() {
                    @Override public void speak(String text) { feedback(text); }
                    @Override public void submit(String text) {
                        hubQuery = text;
                        hubSection = GameActionHub.SEARCH;
                        hubIndex = 0;
                        editor = null;
                        root.cancelGesture();
                        render();
                        readCurrent();
                    }
                    @Override public void cancel() {
                        editor = null;
                        root.cancelGesture();
                        render();
                        readCurrent();
                    }
                });
        root.cancelGesture();
        render();
        feedback(activity.getString(R.string.a11yHubSearch));
    }

    private void showHubActionChoices() {
        if (!comprehensiveMenu || choosingGesture || choosingHubAction) { return; }
        List<GameActionHub.Entry> entries = hubEntries();
        if (entries.isEmpty()) { return; }
        hubIndex = Math.max(0, Math.min(hubIndex, entries.size() - 1));
        GameActionHub.Entry entry = entries.get(hubIndex);
        if (entry.shortcutTarget() == null) {
            feedback(activity.getString(R.string.a11yBindUnavailable));
            return;
        }
        hubActionItem = entry;
        choosingHubAction = true;
        hubActionChoice = 0;
        root.cancelGesture();
        render();
        feedback(activity.getString(R.string.a11yHubActionMenu, entry.label) + ". "
                + hubActionLabel(0) + ", "
                + activity.getString(R.string.a11yPosition, 1, 2));
    }

    private void applyHubActionChoice() {
        if (!choosingHubAction || hubActionItem == null) { return; }
        GameActionHub.Entry entry = hubActionItem;
        String target = entry.shortcutTarget();
        int choice = hubActionChoice;
        choosingHubAction = false;
        hubActionItem = null;
        if (choice == 0) {
            showGestureChoices();
            return;
        }
        if (target == null || "hub:favorites".equals(target)) {
            feedback(activity.getString(R.string.a11yBindUnavailable));
            render();
            return;
        }
        String label = entry.label;
        // Strip the non-interactive availability suffix from saved favorites.
        String unavailable = " (" + activity.getString(R.string.a11yHubFavoriteUnavailable) + ")";
        if (label.endsWith(unavailable)) {
            label = label.substring(0, label.length() - unavailable.length());
        }
        String message;
        if (favorites.contains(target)) {
            favorites.remove(target);
            message = activity.getString(R.string.a11yHubFavoriteRemoved, label);
        } else if (favorites.add(target, label)) {
            message = activity.getString(R.string.a11yHubFavoriteAdded, label);
        } else {
            message = activity.getString(R.string.a11yHubFavoritesFull);
        }
        root.cancelGesture();
        render();
        feedback(message);
    }

    private void showGestureChoices() {
        if (!comprehensiveMenu || choosingGesture || choosingHubAction) { return; }
        List<GameActionHub.Entry> entries = hubEntries();
        if (entries.isEmpty()) { return; }
        hubIndex = Math.max(0, Math.min(hubIndex, entries.size() - 1));
        GameActionHub.Entry selected = entries.get(hubIndex);
        String target = selected.shortcutTarget();
        if (target == null) {
            feedback(activity.getString(R.string.a11yBindUnavailable));
            return;
        }
        bindingItem = selected;
        choosingGesture = true;
        String existing = gestureBindings.gestureFor(target);
        gestureChoice = existing == null ? 0 : ReaderGestureBindings.indexOf(existing);
        if (gestureChoice < 0) { gestureChoice = 0; }
        root.cancelGesture();
        render();
        feedback(activity.getString(R.string.a11yBindTitle, selected.label) + ". "
                + gestureChoiceLabel(gestureChoice) + ", "
                + activity.getString(R.string.a11yPosition,
                        gestureChoice + 1, ReaderGestureBindings.IDS.length + 2));
    }

    private String describeShortcutTarget(String target) {
        if (target == null) { return ""; }
        if (target.startsWith("action:")) {
            ReaderSnapshot.Node node = snapshot == null ? null
                    : snapshot.find(target.substring("action:".length()));
            return node == null ? target.substring("action:".length()) : node.label;
        }
        if (target.startsWith("hub:group:")) {
            try {
                int section = Integer.parseInt(target.substring("hub:group:".length()));
                if (section > 0 && section < GameActionHub.TITLES.length) {
                    return activity.getString(GameActionHub.TITLES[section]);
                }
            } catch (NumberFormatException ignored) {
                // Stale custom gesture target.
            }
        }
        if ("hub:directions".equals(target)) {
            return activity.getString(R.string.a11yHubChooseDirection);
        }
        if ("hub:favorites".equals(target)) {
            return activity.getString(R.string.a11yHubFavorites);
        }
        if ("hub:search".equals(target)) { return activity.getString(R.string.a11yHubSearch); }
        if ("hub:all".equals(target)) { return activity.getString(R.string.a11yHubAllCommands); }
        if ("hub:tts".equals(target)) { return activity.getString(R.string.a11yHubTts); }
        if ("hub:reader_setup".equals(target)) {
            return activity.getString(R.string.a11yHubReaderSetup);
        }
        return target;
    }

    private void applyGestureChoice() {
        if (!choosingGesture || bindingItem == null) { return; }
        GameActionHub.Entry entry = bindingItem;
        String target = entry.shortcutTarget();
        int chosen = gestureChoice;
        if (chosen == ReaderGestureBindings.IDS.length + 1) {
            choosingGesture = false;
            bindingItem = null;
            root.cancelGesture();
            render();
            readCurrent();
            return;
        }
        String message;
        if (chosen == ReaderGestureBindings.IDS.length) {
            gestureBindings.restoreDefault(target);
            message = activity.getString(R.string.a11yBindRestored, entry.label);
        } else if (chosen >= 0 && chosen < ReaderGestureBindings.IDS.length) {
            ReaderGestureBindings.Assignment result = gestureBindings.assign(
                    ReaderGestureBindings.IDS[chosen], target);
            message = activity.getString(R.string.a11yBindSaved,
                    gestureChoiceLabel(chosen), entry.label);
            if (result.displacedTarget != null) {
                message += " " + activity.getString(R.string.a11yBindDisplaced,
                        describeShortcutTarget(result.displacedTarget));
            }
        } else { return; }
        choosingGesture = false;
        bindingItem = null;
        root.cancelGesture();
        render();
        feedback(message);
    }

    private void activateHub() {
        if (!comprehensiveMenu || choosingGesture || choosingHubAction) { return; }
        List<GameActionHub.Entry> entries = hubEntries();
        if (entries.isEmpty()) { return; }
        hubIndex = Math.max(0, Math.min(hubIndex, entries.size() - 1));
        GameActionHub.Entry item = entries.get(hubIndex);
        switch (item.kind) {
            case GameActionHub.OPEN_SEARCH:
                if (hubSection == GameActionHub.ROOT) {
                    hubRootIndex = GameActionHub.TITLES.length;
                }
                searchHub();
                return;
            case GameActionHub.OPEN_FAVORITES:
                hubRootIndex = 0;
                hubSection = GameActionHub.FAVORITES;
                hubIndex = 0;
                root.cancelGesture();
                render();
                readCurrent();
                return;
            case GameActionHub.OPEN_GROUP:
                hubRootIndex = hubIndex;
                hubSection = item.group;
                hubIndex = 0;
                root.cancelGesture();
                render();
                readCurrent();
                return;
            case GameActionHub.OPEN_DIRECTIONS:
                hubSection = GameActionHub.DIRECTIONS;
                hubIndex = 0;
                root.cancelGesture();
                render();
                readCurrent();
                return;
            case GameActionHub.OPEN_ALL:
                hubSection = GameActionHub.SEARCH;
                hubQuery = "";
                hubIndex = 0;
                root.cancelGesture();
                render();
                readCurrent();
                return;
            case GameActionHub.OPEN_TTS:
                // Preserve the menu location across Android settings screens.
                activity.onAccessibilityOpenTtsSettings();
                return;
            case GameActionHub.OPEN_READER_SETUP:
                activity.startActivity(new Intent(activity, ReaderSetupActivity.class));
                return;
            case GameActionHub.EXECUTE:
                ReaderSnapshot.Node node = snapshot == null ? null : snapshot.find(item.node);
                if (node == null || !node.enabled || (!node.actions.contains("activate")
                        && !"document".equals(node.role)
                        && !"world:here".equals(node.id))) {
                    feedback(node == null ? activity.getString(R.string.a11yHubFavoriteUnavailable)
                            : nodeText(node, true));
                    return;
                }
                comprehensiveMenu = false;
                root.cancelGesture();
                focus = item.node;
                render();
                activate();
                return;
            default:
                return;
        }
    }

    @Override
    public void readCurrent() {
        if (debugReport != null) {
            if (debugFocus == 0) {
                if (usesTouchExploration()) {
                    announceAccessibility(debugReport);
                } else {
                    speech.say("debug-report", debugReport, ReaderSpeechQueue.LONG_TEXT);
                }
            } else {
                int label = debugFocus == 1 ? R.string.a11yDebugPromptContinue
                        : debugFocus == 2 ? R.string.a11yDebugPromptIgnore
                        : R.string.a11yDebugPromptCopy;
                feedback(activity.getString(label));
            }
            return;
        }
        if (editor != null) { editor.readCurrent(); return; }
        if (comprehensiveMenu) {
            if (choosingHubAction) {
                feedback(hubActionLabel(hubActionChoice));
            } else if (choosingGesture) {
                feedback(gestureChoiceLabel(gestureChoice));
            } else {
                List<GameActionHub.Entry> items = hubEntries();
                if (items.isEmpty()) {
                    feedback(activity.getString(hubSection == GameActionHub.FAVORITES
                            ? R.string.a11yHubFavoritesEmpty : R.string.a11yHubEmpty));
                }
                else {
                    hubIndex = Math.max(0, Math.min(hubIndex, items.size() - 1));
                    feedback(hubSpoken(items, hubIndex));
                }
            }
            return;
        }
        ReaderSnapshot.Node node = snapshot == null ? null : snapshot.find(focus);
        if (node == null) {
            if (snapshot == null && !lastLoadingStatus.isEmpty()) {
                feedback(activity.getString(R.string.a11yLoadingStatus, lastLoadingStatus));
            }
            return;
        }
        String spoken = nodeText(node, false);
        if (usesTouchExploration()) {
            announceAccessibility(spoken);
        } else {
            speech.focus(node.id, spoken);
        }
    }

    @Override
    public void detail() {
        if (debugReport != null) {
            if (usesTouchExploration()) { announceAccessibility(debugReport); }
            else { speech.say("debug-detail", debugReport, ReaderSpeechQueue.LONG_TEXT); }
            return;
        }
        if (editor != null) { feedback(editor.valueDescription()); return; }
        if (comprehensiveMenu) {
            if (choosingGesture || choosingHubAction) { readCurrent(); }
            else { showHubActionChoices(); }
            return;
        }
        if (snapshot == null && !lastLoadingStatus.isEmpty()) {
            feedback(activity.getString(R.string.a11yLoadingStatus, lastLoadingStatus));
            return;
        }
        ReaderSnapshot.Node node = snapshot == null ? null : snapshot.find(focus);
        if (node != null) {
            String information = nodeText(node, true);
            if ("YESNO".equals(snapshot.category) && !snapshot.summary.trim().isEmpty()
                    && !"dialog:message".equals(node.id)) {
                information = snapshot.summary.trim() + ". " + information;
            }
            feedback(information);
        }
    }

    /**
     * Spell the focused item grapheme by grapheme (R21.3). Names that
     * have no translation are spoken exactly as written; spelling is
     * how the listener checks the precise letters, including names in
     * another language or script.
     */


    private void announceAccessibility(String text) {
        SessionDebugLog.event("ACCESSIBILITY_ANNOUNCE", "requested=" + text);
        root.announceForAccessibility(text);
    }

    private void feedback(String text) {
        if (!foreground || closed) { return; }
        if (usesTouchExploration()) {
            status.setText(text);
            announceAccessibility(text);
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

    public void onBackPressed() {
        SessionDebugLog.event("ACTION", "back focus=" + focus
                + " hub=" + comprehensiveMenu);
        if (closed) { return; }
        if (debugReport != null) { submitDebugResponse(1); return; }
        if (editor != null) { editor.cancelInput(); return; }
        if (commandPaletteOpen) { closeContextActions(); return; }
        if (comprehensiveMenu) {
            if (choosingHubAction) {
                choosingHubAction = false;
                hubActionItem = null;
            } else if (choosingGesture) {
                choosingGesture = false;
                bindingItem = null;
            } else if (hubSection == GameActionHub.DIRECTIONS) {
                hubSection = 3;
                hubIndex = 0;
            } else if (hubSection != GameActionHub.ROOT) {
                hubSection = GameActionHub.ROOT;
                hubIndex = hubRootIndex;
            } else {
                comprehensiveMenu = false;
            }
            root.cancelGesture();
            render();
            if (comprehensiveMenu) { readCurrent(); }
            return;
        }
        if (backReaderLayer()) { return; }
        if (snapshot != null) {
            // A mandatory Yes/No dialog may have no registered Cancel.
            // Treat Back as the native No response, never as Yes.
            if ("YESNO".equals(snapshot.category)) {
                ReaderSnapshot.Node no = snapshot.find("option:1");
                if (no != null && no.enabled && no.actions.contains("activate")) {
                    String previous = focus;
                    focus = no.id;
                    send("activate", "");
                    focus = previous;
                    return;
                }
            }
            // Informational popups sometimes expose Continue but no Quit.
            ReaderSnapshot.Node continueNode = snapshot.find("continue");
            if (continueNode != null && continueNode.enabled
                    && continueNode.actions.contains("activate")) {
                String previous = focus;
                focus = continueNode.id;
                send("activate", "");
                focus = previous;
                return;
            }
            send("cancel", "");
        }
    }

    /**
     * SDL error screens are independent of native game accessibility snapshots.
     * Read a short introduction first; navigating to the report reads it all.
     */
    public void showDebugPrompt(String report) {
        SessionDebugLog.event("DEBUG_PROMPT", report);
        if (closed || report == null || report.isEmpty() || !preferences.isEnabled()) {
            return;
        }
        debugReport = report;
        debugFocus = 0;
        debugResponsePending = false;
        root.cancelGesture();
        root.setVisibility(View.VISIBLE);
        render();
        String firstLine = report.split("\\n", 2)[0];
        feedback(activity.getString(R.string.a11yDebugPromptTitle) + ". "
                + firstLine + ". " + activity.getString(R.string.a11yDebugPromptInstructions));
    }

    private void submitDebugResponse(int action) {
        if (debugReport == null || debugResponsePending || action < 1 || action > 3) {
            return;
        }
        if (action != 3) { debugResponsePending = true; }
        try {
            CataclysmDDA.nativeDebugPromptResponse(action);
            if (action == 3) { feedback(activity.getString(R.string.a11yDebugPromptCopied)); }
        } catch (UnsatisfiedLinkError missing) {
            debugResponsePending = false;
            feedback(activity.getString(R.string.a11yReaderNativeUnavailable));
        }
    }

    public void dismissDebugPrompt() {
        if (debugReport == null) { return; }
        debugReport = null;
        debugResponsePending = false;
        debugFocus = 0;
        root.cancelGesture();
        render();
        readCurrent();
    }

    public boolean isEnabled() { return preferences.isEnabled(); }

    @Override
    public void close() {
        if (closed) { return; }
        closed = true;
        foreground = false;
        presentationEpoch++;
        root.cancelGesture();
        documentReader.clear();
        pendingGameMessages.clear();
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

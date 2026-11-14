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
    private static final String POCKET_CONFIRM_NODE = "reader-pocket:confirm";
    private static final String POCKET_CANCEL_NODE = "reader-pocket:cancel";
    private final CataclysmDDA activity;
    private final ReaderPreferences preferences;
    private final ReaderGestureBindings gestureBindings;
    private final ReaderFavorites favorites;
    private final ReaderSpeechQueue speech;
    private final ReaderCuePlayer cues;
    private final AlignmentSonarPlayer sonar;
    private final AccessibilityManager accessibility;
    private final ReaderGestureSurface root;
    private final TextView status;
    private final LinearLayout rows;
    private final Handler handler = new Handler(Looper.getMainLooper());
    private static final String GESTURE_READER_OFF_KEY = "gesture-reader-off";
    private boolean readerOffAfterAnnouncement;
    private boolean announceReaderEnabledAfterInit;
    private final Runnable finishReaderOffGesture = () -> {
        if (!readerOffAfterAnnouncement) { return; }
        readerOffAfterAnnouncement = false;
        applyReaderGestureEnabled(false);
    };
    private final LinkedHashMap<Long, Long> pendingIntentTimes = new LinkedHashMap<>();
    // Native actions invalidate their snapshot briefly. Messages arriving in
    // this gap must be read after the next snapshot instead of disappearing.
    // A native action can temporarily hide the world snapshot. Retain all
    // game events until the next stable screen instead of dropping the ninth
    // message (or deleting a message after an arbitrary fifteen seconds).
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

    private static final String COMBAT_ALERT_PREFIX = "A11Y_COMBAT_ALERT:";

    // Do not repeat a menu title already contained in its first item.
    // Preserve position, current value and disabled reason as originally read.
    static String menuEntrySpeech(String introduction, String entry) {
        String title = introduction == null ? "" : introduction.trim();
        String item = entry == null ? "" : entry.trim();
        if (title.isEmpty() || item.equals(title)
                || item.startsWith(title + ". ") || item.startsWith(title + ": ")) {
            return item;
        }
        if (item.isEmpty()) { return title; }
        return title + ". " + item;
    }

    private void replayBufferedGameMessages() {
        while (!pendingGameMessages.isEmpty()) {
            // A quick-shot failure can originate in a transient targeting
            // frame. Speak it only after returning to the world, so the
            // reader's setContext() cannot cancel the warning immediately.
            BufferedGameMessage first = pendingGameMessages.peekFirst();
            if (first.text.startsWith(COMBAT_ALERT_PREFIX)
                    && (snapshot == null || !"DEFAULTMODE".equals(snapshot.category))) {
                return;
            }
            BufferedGameMessage buffered = pendingGameMessages.removeFirst();
            // Game events are FIFO and survive temporary screen transitions.
            // Do not evict an older message because a newer one arrived.
            announceGameMessage(buffered.text, buffered.priority, true);
        }
    }
    private final AccessibilityManager.TouchExplorationStateChangeListener explorationListener;
    private ReaderSnapshot snapshot;
    // Loading screens do not publish semantic snapshots. Keep their last
    // visible status available to repeat, even after the main menu clears.
    private String lastLoadingStatus = "";
    // Only world snapshots carry the recent-messages node. Remember the
    // last one so two-finger double-tap can re-read notifications on any
    // screen; the game message log itself is never changed here.
    private String lastRecentMessages = "";
    private final ReaderDocumentReader documentReader;
    private ReaderTextEditor editor;
    private String editorContext = "";
    private long editorRequest;
    private String formContext = "";
    private long formRequest;
    private String focus = "";
    // Switching a nearby category speaks its name/count first. The first
    // right swipe then reads item one, instead of silently skipping it.
    private boolean nearbyFirstItemPending;
    private boolean craftingNavigationPending;
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
    // A hold exposes every original aim/cursor command; the default is eight directions.
    private boolean targetingOriginalControls;
    // Local to the accessible world. Custom map bindings remain untouched.
    private boolean combatMode;
    private boolean vehicleMode;
    private boolean pendingVehicleRoadReport;
    private boolean pendingVehicleSpeedReport;
    private String commandPaletteReturnFocus = "";
    private String commandPaletteReturnParent = "";
    private String readerLayerParent = "";
    private String pendingReaderLayer = "";
    private int pendingReaderLayerRevisions;
    // Read the correct selected-row document after the game thread has
    // acknowledged its focus. Never speak the previous row's details.
    private String pendingLinkedDetail = "";
    private String pendingLinkedDetailContext = "";
    // A native read-only item request is scoped to the currently focused
    // row and screen. Long UTF-8 descriptions are assembled before entering
    // the bounded speech queue; stale replies must not name another item.
    private String pendingItemDetailNode = "";
    private String pendingItemDetailContext = "";
    private StringBuilder pendingItemDetailParts;
    // Inventory container picker: all edits stay local until the parent
    // pocket receives one atomic native commit. 0=outer, 1=hold menu,
    // 2=pockets, 3=contents.
    private int pocketStage;
    private int pocketMenuChoice;
    private String pocketContainerId = "";
    private String pocketCurrentId = "";
    private String pocketReturnParent = "";
    private String pocketContext = "";
    private final LinkedHashMap<String, Integer> pocketDrafts = new LinkedHashMap<>();
    // A native modal may temporarily cover a parent screen. Frame identity
    // survives that push/pop, so return to the last visited item, not item 1.
    private final LinkedHashMap<String, String> savedScreenFocus = new LinkedHashMap<>();
    private boolean foreground;
    private boolean windowFocused;
    private boolean closed;
    private boolean nativeConfigured;
    private boolean lastEnabled;
    private boolean lastSuspended = true;
    private boolean lastSpokenOnlyWorld;
    private boolean lastPerformanceProfile;
    private volatile long presentationEpoch;
    private long lastWorldBusyNotice;
    // Travel controls are a local two-button reader panel, with a third
    // explicit ignore-warning option only when native safe mode blocked the
    // route. The native game thread exclusively owns route changes.
    private int travelChoice;
    private int announcedTravelState;
    // Automatic travel does not necessarily publish a new semantic snapshot
    // on each native step. Poll only the tiny JNI atomic state (never game
    // data) so Pause/Cancel become visible immediately, even after starting
    // from the overmap or observation screen. Do not reannounce steady state.
    private final Runnable travelStatePoll = new Runnable() {
        @Override
        public void run() {
            if (closed) { return; }
            if (foreground && windowFocused && preferences.isEnabled()
                    && !lastSuspended) {
                int state = travelState();
                if (state != announcedTravelState) {
                    announcedTravelState = state;
                    travelChoice = 0;
                    clearPendingWorldDirection();
                    root.cancelGesture();
                    render();
                    if (state != 0) {
                        feedback(travelStatus(state) + ". " + travelSpoken(state));
                    } else if (snapshot != null
                            && ("DEFAULTMODE".equals(snapshot.category)
                                || "activity".equals(snapshot.category))) {
                        // Modal questions have their own one-shot original
                        // announcement. A travel-state transition must not
                        // interrupt it with an unrelated focus utterance.
                        readCurrent();
                    }
                }
            }
            handler.postDelayed(this, 200);
        }
    };
    // One short-lived *gesture*, never a native movement backlog. When a
    // swipe arrives during the previous turn, try its latest direction once
    // after the next live world publication, then discard it. Native safety,
    // context and movement permissions remain authoritative.
    private String pendingWorldDirection = "";
    private String pendingWorldContext = "";
    private long pendingWorldRevision;
    private long pendingWorldSince;
    private boolean retryingWorldDirection;
    // The native one-step gate opens only on the next game input wait.
    // Poll this tiny thread-safe flag only while ONE gesture is pending;
    // never queue more than the most recent gesture or cross a screen.
    private final Runnable pendingWorldRetryPoll = this::retryPendingWorldDirection;

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
        sonar = new AlignmentSonarPlayer(activity);
        documentReader = new ReaderDocumentReader(activity);
        // Document position survives interruptions (R04.4): the queue
        // only reports whether the current paragraph finished or was cut
        // off; the document reader decides what Continue replays.
        speech.setCompletionListener(new ReaderSpeechQueue.CompletionListener() {
            @Override
            public void onFinished(String key) {
                if (GESTURE_READER_OFF_KEY.equals(key)) {
                    handler.post(finishReaderOffGesture);
                }
                if (key.equals(documentReader.speechKey())) {
                    documentReader.markFinished();
                }
            }

            @Override
            public void onInterrupted(String key) {
                if (GESTURE_READER_OFF_KEY.equals(key)) {
                    handler.post(finishReaderOffGesture);
                }
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
            presentationEpoch++;
            updateNativeState(true);
            render();
        };
        if (accessibility != null) {
            accessibility.addTouchExplorationStateChangeListener(explorationListener);
        }
        handler.postDelayed(travelStatePoll, 200);
    }

    private void onSpeechHealth(AndroidSpeech.State state) {
        if (closed) { return; }
        ReaderDiagnostics.get().event("speech_" + state.name().toLowerCase(Locale.ROOT));
        updateNativeState(false);
        if (state == AndroidSpeech.State.READY && !usesTouchExploration()) {
            if (announceReaderEnabledAfterInit && preferences.isEnabled()) {
                announceReaderEnabledAfterInit = false;
                speech.say("gesture-reader-on", activity.getString(R.string.a11yReaderGestureOn),
                        ReaderSpeechQueue.FOCUS);
            } else {
                readCurrent();
            }
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
        render();
        updateNativeState(true);
    }

    public void onPause() {
        ReaderDiagnostics.get().event("lifecycle_pause");
        foreground = false;
        sonar.setWorldActive(false);
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
        lastRecentMessages = "";
        readerLayerParent = "";
        pendingReaderLayer = "";
        pendingLinkedDetail = "";
        pendingLinkedDetailContext = "";
        pendingItemDetailNode = "";
        pendingItemDetailContext = "";
        pendingItemDetailParts = null;
        targetingOriginalControls = false;
        pocketStage = 0;
        pocketDrafts.clear();
        pocketContainerId = "";
        pocketCurrentId = "";
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
            sonar.setWorldActive(false);
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
        sonar.setEnabled(enabled && preferences.isDirectionalSonarEnabled());
        boolean suspended = !foreground || !windowFocused || (!usesTouchExploration() && !speech.isReady());
        if (!enabled || suspended) {
            sonar.setWorldActive(false);
        }
        boolean spokenOnlyWorld = preferences.isSpokenOnlyWorldEnabled();
        boolean profile = SessionDebugLog.isRecording();
        if (force || !nativeConfigured || lastEnabled != enabled || lastSuspended != suspended
                || lastSpokenOnlyWorld != spokenOnlyWorld || lastPerformanceProfile != profile) {
            try {
                CataclysmDDA.nativeConfigureReaderPerformance(spokenOnlyWorld, profile);
                CataclysmDDA.nativeConfigureDirectionalSonar(
                        enabled && preferences.isDirectionalSonarEnabled());
                if (force || !nativeConfigured || lastEnabled != enabled
                        || lastSuspended != suspended) {
                    CataclysmDDA.nativeConfigureReader(enabled, suspended);
                }
                nativeConfigured = true;
                lastEnabled = enabled;
                lastSuspended = suspended;
                lastSpokenOnlyWorld = spokenOnlyWorld;
                lastPerformanceProfile = profile;
            } catch (UnsatisfiedLinkError unavailable) {
                nativeConfigured = false;
                status.setText(R.string.a11yReaderNativeUnavailable);
            }
        }
    }

    public void receive(String json) {
        final long owner = presentationEpoch;
        final long receivedAt = SessionDebugLog.isRecording()
                ? SystemClock.elapsedRealtime() : 0L;
        handler.post(() -> {
            if (closed || !foreground || !preferences.isEnabled() || owner != presentationEpoch) {
                return;
            }
            final long dispatchedAt = receivedAt != 0L
                    ? SystemClock.elapsedRealtime() : 0L;
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
                    // A rejection for an already closed native screen must
                    // never interrupt speech on a *different* interface.
                    // This applies to every reader adapter, not just skill
                    // headings. Do not hide a live rejection on the current
                    // screen or retry a potentially consequential command.
                    if (requestId != 0 && ("stale".equals(code)
                            || "unsupported".equals(code) || "disabled".equals(code))) {
                        String receiptContext = object.optString("session") + ":"
                                + object.optString("frame") + ":"
                                + object.optString("generation");
                        if (since == null || snapshot == null
                                || !receiptContext.equals(snapshot.contextKey())) {
                            SessionDebugLog.event("INTENT_RESULT_IGNORED",
                                    "request=" + requestId + " reason=former_screen");
                            return;
                        }
                    }
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
                    sonar.setWorldActive(false);
                    comprehensiveMenu = false;
                    choosingGesture = false;
                    bindingItem = null;
                    choosingHubAction = false;
                    hubActionItem = null;
                    snapshot = null;
                    lastRecentMessages = "";
                    commandPaletteOpen = false;
                    targetingOriginalControls = false;
                    pendingLinkedDetail = "";
                    pendingLinkedDetailContext = "";
                    pendingItemDetailNode = "";
                    pendingItemDetailContext = "";
                    pendingItemDetailParts = null;
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
                final long parsedAt = receivedAt != 0L
                        ? SystemClock.elapsedRealtime() : 0L;
                SessionDebugLog.event("SCREEN", "category=" + next.category
                        + " title=" + next.title + " focus=" + next.focus
                        + " nodes=" + next.nodes.size());
                if (snapshot != null && next.session == snapshot.session && next.frame == snapshot.frame
                        && (next.generation < snapshot.generation ||
                        (next.generation == snapshot.generation && next.revision < snapshot.revision))) {
                    return;
                }
                boolean changedContext = snapshot == null || !snapshot.contextKey().equals(next.contextKey());
                boolean nearbyCategoryChanged = !changedContext && snapshot != null
                        && "surroundings".equals(next.category)
                        && "surroundings".equals(snapshot.category)
                        && !next.summary.equals(snapshot.summary);
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
                        || "OPTIONS".equals(next.category)
                        // Character creation was never re-read when
                        // the selection changed, so +/-1 adjustments were
                        // silent on the Java side (the C++ announce is the
                        // other channel; both are kept, they say different
                        // things). The same gap hid
                        // advanced-inventory and armor-layer updates.
                        || "CHARACTER_CREATION".equals(next.category)
                        || next.category.startsWith("character-creator-step-")
                        || "advanced-inventory".equals(next.category)
                        || "armor-layers".equals(next.category);
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
                            || "OPTIONS".equals(next.category)
                            // The selected-flag loop above
                            // cannot see a +/-1 or command that only changes
                            // the focused row's value (points left, sort or
                            // filter state, armor position), so re-read the
                            // focused row when its value/detail changes, the
                            // same way the world-generation screens do.
                            || "CHARACTER_CREATION".equals(next.category)
                            || next.category.startsWith("character-creator-step-")
                            || "advanced-inventory".equals(next.category)
                            || "armor-layers".equals(next.category))) {
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
                if (pocketStage != 0 &&
                        (changedContext || next.find(pocketContainerId) == null
                         || (pocketStage == 3 && next.find(pocketCurrentId) == null))) {
                    // A different native screen or disappearing container
                    // invalidates the entire local draft without committing.
                    pocketStage = 0;
                    pocketCurrentId = "";
                    pocketContainerId = "";
                    pocketContext = "";
                    pocketReturnParent = "";
                    pocketDrafts.clear();
                }
                boolean craftingNavigationCompleted = craftingNavigationPending
                        && snapshot != null && "crafting".equals(next.category)
                        && (!next.summary.equals(snapshot.summary)
                            || !next.focus.equals(snapshot.focus)
                            || next.nodes.size() != snapshot.nodes.size());
                snapshot = next;
                // A vehicle may disappear or the character may dismount between
                // snapshots. Never leave a stale driving mode armed on foot.
                if ("DEFAULTMODE".equals(next.category)
                        && next.find("reader:vehicle:seat") == null) {
                    vehicleMode = false;
                    pendingVehicleRoadReport = false;
                    pendingVehicleSpeedReport = false;
                }
                if (pendingVehicleSpeedReport && "DEFAULTMODE".equals(next.category)) {
                    pendingVehicleSpeedReport = false;
                    ReaderSnapshot.Node speed = next.find("reader:vehicle:speed");
                    if (speed != null && !speed.detail.isEmpty()) {
                        final String report = speed.detail;
                        handler.post(() -> {
                            if (vehicleMode && directWorldGesturesEnabled()) {
                                feedback(report);
                            }
                        });
                    }
                }
                if (pendingVehicleRoadReport && "DEFAULTMODE".equals(next.category)) {
                    pendingVehicleRoadReport = false;
                    ReaderSnapshot.Node road = next.find("reader:vehicle:road");
                    if (road != null && !road.detail.isEmpty()) {
                        final String report = road.detail;
                        handler.post(() -> {
                            if (vehicleMode && directWorldGesturesEnabled()) {
                                feedback(report);
                            }
                        });
                    }
                }
                // Only world snapshots publish world:messages. Remember the
                // last non-empty copy so two-finger double-tap can re-read
                // notifications from any screen. An empty node never wipes
                // the cache; a lifecycle boundary clears it instead.
                ReaderSnapshot.Node recentMessages = next.find("world:messages");
                if (recentMessages != null && !recentMessages.detail.isEmpty()) {
                    lastRecentMessages = recentMessages.detail;
                }
                if (!"crafting".equals(next.category)) {
                    craftingNavigationPending = false;
                }
                if (!"surroundings".equals(next.category)) {
                    nearbyFirstItemPending = false;
                } else if (changedContext || nearbyCategoryChanged) {
                    nearbyFirstItemPending = true;
                }
                lastLoadingStatus = "";
                speech.setContext(next.contextKey());
                replayBufferedGameMessages();
                boolean enteredTreeLayer = false;
                if (changedContext) {
                    clearPendingWorldDirection();
                    commandPaletteOpen = false;
                    targetingOriginalControls = false;
                    commandPaletteReturnFocus = "";
                    commandPaletteReturnParent = "";
                    readerLayerParent = "";
                    pendingReaderLayer = "";
                    pendingReaderLayerRevisions = 0;
                    pendingLinkedDetail = "";
                    pendingLinkedDetailContext = "";
                    pendingItemDetailNode = "";
                    pendingItemDetailContext = "";
                    pendingItemDetailParts = null;
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
                } else if (craftingNavigationCompleted) {
                    // Category and page gestures must start on the selected
                    // recipe of the NEW native list, not an old details field.
                    focus = next.find(next.focus) != null ? next.focus
                            : next.nodes.isEmpty() ? "" : next.nodes.get(0).id;
                    craftingNavigationPending = false;
                } else if (nearbyCategoryChanged) {
                    // Even if the old target also exists in the new category
                    // (notably when switching to All), start at item one.
                    focus = next.find(next.focus) != null ? next.focus
                            : next.nodes.isEmpty() ? "" : next.nodes.get(0).id;
                } else if (changedContext) {
                    String remembered = savedScreenFocus.get(nativeScreenKey(next));
                    focus = remembered != null && next.find(remembered) != null
                            ? remembered : next.find(next.focus) != null ? next.focus
                            : next.nodes.isEmpty() ? "" : next.nodes.get(0).id;
                } else if (next.find(focus) == null
                        && !(pocketStage == 3 && (POCKET_CONFIRM_NODE.equals(focus)
                                || POCKET_CANCEL_NODE.equals(focus)))) {
                    // Changing a surroundings page always focuses its native
                    // first/last entry. Other screens keep their usual
                    // ancestor/sibling fallback.
                    focus = "surroundings".equals(next.category)
                            && next.find(next.focus) != null ? next.focus
                            : resolveFocusFallback(previousSnapshot, previous, next);
                }
                // The engine may initially focus a hidden command. Start on
                // primary content instead of reading the long command tail.
                ReaderSnapshot.Node focusedNode = next.find(focus);
                if (!commandPaletteOpen && focusedNode != null
                        && isAuxiliaryCommand(focusedNode)
                        // A focused direction command in a minigame
                        // is a real control, not the command tail: keep the
                        // focus there so the player can double-tap again to
                        // keep moving in the same direction.
                        && !(isMinigameCategory(next.category)
                                && isDirectionCommand(focusedNode))) {
                    List<ReaderSnapshot.Node> primary = visibleNodes();
                    if (!primary.isEmpty() && !isAuxiliaryCommand(primary.get(0))) {
                        focus = primary.get(0).id;
                    }
                }
                // Re-entering targeting must start at the simplified eight
                // directions, even if an old aiming cursor was remembered
                // or a native selected-target event changed the focus.
                if ("targeting".equals(next.category)
                        && next.find("target:fire-direction:0") != null
                        && !targetingOriginalControls
                        && !focus.startsWith("target:fire-direction:")) {
                    focus = "target:fire-direction:0";
                }
                ReaderSnapshot.Node currentFocus = next.find(focus);
                if (currentFocus != null && !currentFocus.parentId.equals(readerLayerParent)) {
                    // The engine may open a new tree branch directly, or
                    // select a new child after a modal changes state.
                    // Inventory pocket browsing is the exception: swiping
                    // onto a pocket-content row must not trap the reader
                    // inside that pocket's own child layer.
                    if (!isFlattenedPocketEntry(currentFocus)) {
                        readerLayerParent = currentFocus.parentId;
                    }
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
                // Opening a recipe detail pane does not change the native
                // "crafting" context. Detect the new intro node explicitly
                // so its actual description speaks without a two-finger tap.
                // Returning from the batch-size chooser must stay silent.
                boolean enteredCraftRecipeDetail = "crafting".equals(next.category)
                        && next.find("craft:detail:intro") != null
                        && (previousSnapshot == null
                            || (previousSnapshot.find("craft:detail:intro") == null
                                && previousSnapshot.find("craft:detail:qty:current") == null));
                boolean enteredMenu = nativeLayerChanged || enteredTreeLayer
                        || (changedContext && next.find("world:here") == null);
                ReaderSnapshot.Node nextTile = next.find("world:here");
                ReaderSnapshot.Node previousTile = previousSnapshot == null ? null
                        : previousSnapshot.find("world:here");
                boolean worldView = nextTile != null;
                sonar.setWorldActive(worldView && !comprehensiveMenu
                        && foreground && windowFocused && preferences.isEnabled()
                        && debugReport == null);
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
                int activeTravel = travelState();
                if (activeTravel != 0) {
                    boolean enteredTravel = announcedTravelState != activeTravel
                            || changedContext;
                    if (announcedTravelState != activeTravel) { travelChoice = 0; }
                    announcedTravelState = activeTravel;
                    render();
                    clearPendingWorldDirection();
                    if (enteredTravel) {
                        feedback(travelStatus(activeTravel) + ". " + travelSpoken(activeTravel));
                    }
                    return;
                }
                announcedTravelState = 0;
                render();
                if (worldView) {
                    retryPendingWorldDirection();
                } else {
                    clearPendingWorldDirection();
                }
                if (worldView && receivedAt != 0L && SessionDebugLog.isRecording()) {
                    SessionDebugLog.event("WORLD_JAVA_TIMING",
                            "jni_to_ui_ms=" + (dispatchedAt - receivedAt)
                            + " parse_ms=" + (parsedAt - dispatchedAt)
                            + " ui_update_ms=" + (SystemClock.elapsedRealtime() - parsedAt)
                            + " nodes=" + next.nodes.size());
                }
                // A summary that changed while the context and the
                // focus stayed the same is new information (a minigame move,
                // win or loss; a new Source/Destination; a new armor tab).
                // Only read it when it really changed, so a repeatedly
                // published game-over summary is spoken once, and never
                // while a document or the command palette owns the swipes.
                boolean summaryChanged = !changedContext && previousSnapshot != null
                        && readsSummaryOnChange(next.category)
                        && !next.summary.trim().isEmpty()
                        && !next.summary.equals(previousSnapshot.summary)
                        && previous.equals(focus)
                        && !documentReader.isOpen() && !commandPaletteOpen;
                if (worldView) {
                    if (newlyNoteworthy) { feedback(nextTile.value); }
                } else if (enteredCraftRecipeDetail
                        && next.find("craft:detail:name") != null) {
                    // Speak name + actual description exactly once on entry.
                    // Keep the name focused, so normal left/right swipes,
                    // Cancel, quantity, and Craft remain unchanged.
                    ReaderSnapshot.Node title = next.find("craft:detail:name");
                    ReaderSnapshot.Node intro = next.find("craft:detail:intro");
                    StringBuilder introduction = new StringBuilder(nodeText(title, false));
                    if (intro != null) {
                        String details = intro.detail.isEmpty() ? intro.label : intro.detail;
                        if (!details.trim().isEmpty()) {
                            introduction.append(". ").append(details);
                        }
                    }
                    speakInlineDetail(introduction.toString());
                } else if (enteredMenu && "SCROLLABLE_TEXT".equals(next.category)
                        && next.find(focus) != null
                        && "document".equals(next.find(focus).role)) {
                    // Narrative screens such as "Your followers" should not
                    // speak only their title and wait for a hidden activation.
                    ReaderSnapshot.Node document = next.find(focus);
                    speakDocument(next.title + ". " + documentReader.open(
                            next.contextKey(), document.id, document.detail));
                } else if ("crafting".equals(next.category) && craftingNavigationCompleted
                        && next.find(focus) != null) {
                    feedback(next.summary.trim() + ". " + nodeText(next.find(focus), false));
                } else if ("surroundings".equals(next.category)
                        && (enteredMenu || nearbyCategoryChanged)) {
                    // Announce the active category and its *total* number
                    // of entries once, not the first item's name. Horizontal
                    // browsing speaks entries with direction and distance.
                    feedback((enteredMenu ? next.title + ". " : "") + next.summary);
                } else if (enteredMenu && next.find(focus) != null) {
                    // Yes/No questions carry their own text. Speak it once
                    // on entering the modal, then only the actual choice
                    // and position for subsequent swipes; never prepend the
                    // generic "Dialog" label or a keyboard case hint.
                    boolean yesNo = "YESNO".equals(next.category)
                            || "YESNOQUIT".equals(next.category);
                    String introduction = yesNo ? next.summary.trim() : next.title;
                    // Places intentionally starts speaking at the first actual
                    // destination: no title, preamble, or item counter.
                    if (next.category.startsWith("character-creator-step-")
                            || "CHARACTER_CREATION".equals(next.category)) {
                        // A creator step has a deliberately short introduction;
                        // the first choice is read when the player browses it.
                        // On the summary tab (CHARACTER_CREATION)
                        // the snapshot.summary ("Character: … Scenario: …
                        // Profession: …") was never spoken on entry because
                        // only the title was read. The C++ side currently
                        // appends the summary to the title as a bridge, so
                        // speak it as well only when the title does not
                        // already contain it: the line is heard exactly
                        // once either way, and stays heard if the C++
                        // bridge is ever removed.
                        String creatorIntroduction = introduction;
                        if ("CHARACTER_CREATION".equals(next.category)
                                && !next.summary.trim().isEmpty()
                                && !next.title.contains(next.summary.trim())) {
                            creatorIntroduction = introduction + ". " + next.summary.trim();
                        }
                        feedback(creatorIntroduction);
                    } else if ("places".equals(next.category)
                            || "nearby-target".equals(next.category)
                            || "pickup-here".equals(next.category)) {
                        feedback(nodeText(next.find(focus), false));
                    } else {
                        feedback(menuEntrySpeech(introduction,
                                nodeText(next.find(focus), false)));
                    }
                } else if (selectionChanged) {
                    readCurrent();
                } else if (summaryChanged) {
                    // The summary itself is the
                    // feedback for this update; reading the focused row
                    // instead would repeat its old label. Blocked moves and
                    // level wins keep their own C++ announces, which say
                    // something different and arrive first.
                    feedback(next.summary.trim());
                } else if (!usesTouchExploration() && (changedContext || !previous.equals(focus))) {
                    readCurrent();
                }
                // A requested linked document must match the selection on
                // the newly published native snapshot. Do not read a previous
                // mission/recipe after a fast swipe followed by two fingers.
                if (!pendingLinkedDetail.isEmpty() && snapshot != null
                        && pendingLinkedDetailContext.equals(snapshot.contextKey())
                        && pendingLinkedDetail.equals(focus)) {
                    ReaderSnapshot.Node ready = snapshot.find(pendingLinkedDetail);
                    if (ready != null && ready.selected) {
                        pendingLinkedDetail = "";
                        pendingLinkedDetailContext = "";
                        readLinkedDetails(ready);
                    }
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
        announceGameMessage(text, priority, false);
    }

    private boolean expectingItemDetail() {
        return snapshot != null && !pendingItemDetailNode.isEmpty()
                && pendingItemDetailContext.equals(snapshot.contextKey())
                && pendingItemDetailNode.equals(focus);
    }

    private void announceGameMessage(String text, int priority, boolean immediate) {
        // Framed long documents are assembled on the UI thread, before they
        // enter ReaderSpeechQueue (MAX_PENDING=8). Never silently discard
        // early chunks when a long book/item description is requested.
        if (text == null || text.trim().isEmpty()
                || (text.length() > 16000 && !text.startsWith("A11Y_DETAIL:"))) {
            return;
        }
        final String message = text;
        final boolean loading = message.startsWith("A11Y_LOADING:");
        final boolean extendedScan = message.startsWith("A11Y_SCAN:");
        final boolean requestedDetail = message.startsWith("A11Y_DETAIL:");
        final boolean detailBegin = message.startsWith("A11Y_DETAIL_BEGIN:");
        final boolean detailPart = message.startsWith("A11Y_DETAIL_PART:");
        final boolean detailEnd = message.startsWith("A11Y_DETAIL_END:");
        final boolean combatAlert = message.startsWith(COMBAT_ALERT_PREFIX);
        final String spokenMessage = ReaderSpokenText.clean(
                loading ? message.substring("A11Y_LOADING:".length())
                : extendedScan ? message.substring("A11Y_SCAN:".length())
                : requestedDetail ? message.substring("A11Y_DETAIL:".length())
                : combatAlert ? message.substring(COMBAT_ALERT_PREFIX.length()) : message);
        // During movement, generating a new map area can emit this repeatedly.
        // Keep the actual map generation and all other gameplay messages;
        // do not let this one sentence enter the buffered or spoken FIFO.
        if ("Đang tạo khu vực bản đồ mới. Vui lòng chờ.".equals(spokenMessage)
                || "Generating a new map area. Please wait.".equals(spokenMessage)) {
            SessionDebugLog.event("MAPGEN_SPEECH_SUPPRESSED", "movement_mapgen_notice");
            return;
        }
        final int level = priority >= ReaderSpeechQueue.URGENT
                ? ReaderSpeechQueue.URGENT : ReaderSpeechQueue.INFORMATION;
        Runnable deliver = () -> {
            if (closed || !foreground || !windowFocused || !preferences.isEnabled()) {
                return;
            }
            if (detailBegin || detailPart || detailEnd) {
                if (!expectingItemDetail()) {
                    pendingItemDetailParts = null;
                    return;
                }
                if (detailBegin) {
                    pendingItemDetailParts = new StringBuilder();
                } else if (pendingItemDetailParts == null) {
                    return;
                }
                String marker = detailBegin ? "A11Y_DETAIL_BEGIN:"
                        : detailPart ? "A11Y_DETAIL_PART:" : "A11Y_DETAIL_END:";
                String part = message.substring(marker.length());
                if (pendingItemDetailParts.length() > 1000000 - part.length()) {
                    pendingItemDetailParts = null;
                    return;
                }
                pendingItemDetailParts.append(part);
                if (detailEnd) {
                    String assembled = pendingItemDetailParts.toString();
                    pendingItemDetailParts = null;
                    announceGameMessage("A11Y_DETAIL:" + assembled, priority, true);
                }
                return;
            }
            if (requestedDetail && !expectingItemDetail()) { return; }
            if (!loading && (snapshot == null
                    || (combatAlert && !"DEFAULTMODE".equals(snapshot.category)))) {
                // A combat failure produced in targeting must wait until
                // the world reappears before speaking. Do not lose it to
                // the immediate modal-to-world context change.
                // An action intentionally publishes an inactive snapshot
                // while the native command executes. In the supplied log,
                // "Nothing to open here" arrived in exactly this gap.
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
            } else if (requestedDetail) {
                // Two fingers request speech, not a new navigation mode.
                // ReaderSpeechQueue chunks this ONE request internally,
                // so the whole item description can be heard without
                // changing the focused row or requiring an extra Back.
                speakInlineDetail(spokenMessage);
            } else if (extendedScan) {
                // Reading sixteen unrestricted map cells may outlive the
                // eight-second event TTL; use the document-length channel.
                speech.say("world-scan:" + ANNOUNCEMENT_SEQUENCE.incrementAndGet(),
                        spokenMessage, ReaderSpeechQueue.LONG_TEXT);
            } else {
                // Native game announcements are read in strict arrival order
                // by ReaderSpeechQueue. No newer hit, sound, status or
                // reload message may interrupt an older game announcement.
                // The priority value is retained only for diagnostics.
                final int messageLevel = level == ReaderSpeechQueue.URGENT
                        ? ReaderSpeechQueue.URGENT : ReaderSpeechQueue.GAME_EVENT;
                SessionDebugLog.event("GAME_MESSAGE_POLICY", "priority=" + messageLevel
                        + " text=" + message);
                speech.say("game-message:" + ANNOUNCEMENT_SEQUENCE.incrementAndGet(), spokenMessage,
                        messageLevel, 15000);
            }
        };
        if (immediate) {
            // Snapshot callbacks already run on the UI looper. Replaying an
            // older buffered event by posting a SECOND runnable lets a new
            // native event overtake it. Dispatch synchronously to the FIFO.
            deliver.run();
        } else {
            handler.post(deliver);
        }
    }

    /** Per-ray native callback; only the UI looper touches SoundPool. */
    public void onDirectionalSonarTargets(int[] creatures, int[] items) {
        handler.post(() -> {
            if (closed) {
                return;
            }
            boolean active = foreground && windowFocused && preferences.isEnabled()
                    && snapshot != null && snapshot.find("world:here") != null
                    && !comprehensiveMenu && debugReport == null;
            sonar.setWorldActive(active);
            if (active) {
                sonar.updateTargets(creatures, items);
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
        // A semantic value identical to its label adds no information.
        if (!node.value.isEmpty() && !node.value.equals(node.label)) {
            text.append(". ").append(node.value);
        }
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
        // Read the position on every focus move, not only on Detail.
        // Nearby entries may have the same label (e.g. two distinct doors),
        // and their 1/2, 2/2 markers prevent an endless-list impression.
        int spokenPosition = node.position;
        int spokenTotal = node.total;
        // Minigame snapshots assign position/total over every
        // node, including commands hidden behind long press, so the C++
        // numbers count rows the player can never swipe to. Recompute
        // from the swipe list instead, so the spoken position always
        // matches what swiping reaches (status rows plus the direction
        // commands now kept in the list).
        boolean minigamePositions = snapshot != null
                && isMinigameCategory(snapshot.category);
        if (minigamePositions) {
            spokenPosition = 0;
            spokenTotal = 0;
        }
        // Speak positions within the correct axis: main items horizontally,
        // pocket entries vertically inside the selected top-level item.
        boolean inventoryPositions = flattenedInventoryBrowse();
        if (inventoryPositions && isFlattenedPocketEntry(node)) {
            ReaderSnapshot.Node root = inventoryRootItem(node);
            if (root != null) {
                List<ReaderSnapshot.Node> contents = flattenedPocketContents(root.id);
                for (int i = 0; i < contents.size(); i++) {
                    if (contents.get(i).id.equals(node.id)) {
                        spokenPosition = i + 1;
                        spokenTotal = contents.size();
                        break;
                    }
                }
            }
        } else if ((spokenPosition <= 0 || spokenTotal <= 0 || inventoryPositions)
                && snapshot != null
                && !"DEFAULTMODE".equals(snapshot.category)) {
            List<ReaderSnapshot.Node> siblings = visibleNodes();
            for (int i = 0; i < siblings.size(); i++) {
                if (siblings.get(i).id.equals(node.id)) {
                    spokenPosition = i + 1;
                    spokenTotal = siblings.size();
                    break;
                }
            }
        }
        // Expose the actual control type, in addition to the value and position.
        // The live world cursor is descriptive, not a menu button.
        int roleLabel = 0;
        if (!"DEFAULTMODE".equals(snapshot == null ? "" : snapshot.category)) {
            if ("checkbox".equals(node.role)) {
                roleLabel = R.string.a11yControlCheckbox;
            } else if ("choice".equals(node.fieldType) || "combobox".equals(node.role)) {
                roleLabel = R.string.a11yControlChoice;
            } else if ("int".equals(node.fieldType) || "float".equals(node.fieldType)
                    || "slider".equals(node.role)) {
                roleLabel = R.string.a11yControlSlider;
            } else if ("string".equals(node.fieldType) || "textbox".equals(node.role)) {
                roleLabel = R.string.a11yControlTextField;
            } else if ("button".equals(node.role) && node.actions.contains("activate")) {
                roleLabel = R.string.a11yControlButton;
            }
        }
        if (roleLabel != 0) { text.append(". ").append(activity.getString(roleLabel)); }
        if (spokenPosition > 0 && spokenTotal > 0) {
            text.append(", ").append(activity.getString(
                    R.string.a11yPosition, spokenPosition, spokenTotal));
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

    /** Minigame screens: their screen_scope category is the
     * game id, not an input-context id. */
    private static boolean isMinigameCategory(String category) {
        return "robot-finds-kitten".equals(category) || "snake".equals(category)
                || "sokoban".equals(category) || "lightson".equals(category);
    }

    /**
     * Screens whose summary is the event itself: a minigame
     * summary changes after every move and on win/lose, an
     * advanced-inventory summary carries the new Source/Destination,
     * and an armor-layers summary is the new tab name. Other screens
     * keep their existing behaviour: the calendar summary would repeat
     * the old/new date already spoken from the focused row's value,
     * and dialogue already announces the spoken line as a game event.
     */
    private static boolean readsSummaryOnChange(String category) {
        return isMinigameCategory(category)
                || "advanced-inventory".equals(category)
                || "armor-layers".equals(category);
    }

    /** Only auxiliary screen controls go into the hold menu, never dialogue choices. */
    private static boolean isAuxiliaryCommand(ReaderSnapshot.Node node) {
        return "command".equals(node.role) || node.id.startsWith("group:")
                || node.id.startsWith("action:") || node.id.startsWith("category:")
                || "filter".equals(node.id) || "filter:clear".equals(node.id);
    }

    /**
     * A direction command of a minigame (node id is the action
     * id from a11y::command_snapshot, e.g. "UP"). Directions are the
     * controls the player uses on every move, so in minigames they stay
     * in the swipe list instead of hiding behind long press. Every other
     * auxiliary command still hides behind long press.
     */
    private static boolean isDirectionCommand(ReaderSnapshot.Node node) {
        if (!"command".equals(node.role)) { return false; }
        switch (node.id) {
            case "UP": case "DOWN": case "LEFT": case "RIGHT":
            case "LEFTUP": case "RIGHTUP": case "LEFTDOWN": case "RIGHTDOWN":
                return true;
            default: return false;
        }
    }

    private List<ReaderSnapshot.Node> pocketNodes(String parent) {
        ArrayList<ReaderSnapshot.Node> found = new ArrayList<>();
        if (snapshot == null) { return found; }
        for (ReaderSnapshot.Node node : snapshot.nodes) {
            if (parent.equals(node.parentId) && node.id.startsWith("pocket:")) {
                found.add(node);
            }
        }
        return found;
    }

    private List<ReaderSnapshot.Node> pocketItems() {
        ArrayList<ReaderSnapshot.Node> found = new ArrayList<>();
        if (snapshot == null || pocketStage != 3) { return found; }
        for (ReaderSnapshot.Node node : snapshot.nodes) {
            if (pocketCurrentId.equals(node.parentId) && node.id.startsWith("pocket-entry:")) {
                found.add(node);
            }
        }
        return found;
    }

    private int pocketAmount(ReaderSnapshot.Node node) {
        Integer draft = pocketDrafts.get(node.id);
        if (draft != null) { return draft; }
        try { return Integer.parseInt(node.appliedValue); }
        catch (NumberFormatException invalid) { return 0; }
    }

    private boolean pocketAllowsSelection() {
        ReaderSnapshot.Node parent = snapshot == null ? null : snapshot.find(pocketCurrentId);
        return parent != null && parent.actions.contains("set_value");
    }

    private String pocketItemSpeech(ReaderSnapshot.Node node, int position, int total) {
        if (!pocketAllowsSelection()) {
            // Ordinary Inventory is a read-only browse, not a pickup draft.
            String detail = node.detail.isEmpty() || node.detail.equals(node.label)
                    ? "" : ". " + node.detail;
            return node.label + detail + ". "
                    + activity.getString(R.string.a11yPosition, position, total);
        }
        return activity.getString(R.string.a11yPocketRow, node.label,
                pocketAmount(node), (int) node.fieldMax, position, total)
                + (node.enabled ? "" : ". " + node.disabledReason);
    }

    private String pocketChoiceSpeech() {
        if (pocketStage == 1) {
            String name = pocketMenuChoice == 0
                    ? activity.getString(R.string.a11yPocketOpen)
                    : activity.getString(R.string.a11yPocketOtherActions);
            return name + ". " + activity.getString(R.string.a11yControlButton) + ", "
                    + activity.getString(R.string.a11yPosition, pocketMenuChoice + 1, 2);
        }
        if (pocketStage == 2) {
            List<ReaderSnapshot.Node> pockets = pocketNodes(pocketContainerId);
            for (ReaderSnapshot.Node pocket : pockets) {
                if (pocket.id.equals(focus)) {
                    return nodeText(pocket, false);
                }
            }
        }
        if (pocketStage == 3) {
            List<ReaderSnapshot.Node> items = pocketItems();
            if (POCKET_CONFIRM_NODE.equals(focus)) {
                return activity.getString(R.string.a11yPocketConfirm) + ". "
                        + activity.getString(R.string.a11yControlButton) + ", "
                        + activity.getString(R.string.a11yPosition, items.size() + 1, items.size() + 2);
            }
            if (POCKET_CANCEL_NODE.equals(focus)) {
                int cancelPosition = items.size() + (pocketAllowsSelection() ? 2 : 1);
                return activity.getString(R.string.a11yPocketCancel) + ". "
                        + activity.getString(R.string.a11yControlButton) + ", "
                        + activity.getString(R.string.a11yPosition, cancelPosition, cancelPosition);
            }
            for (int i = 0; i < items.size(); i++) {
                if (items.get(i).id.equals(focus)) {
                    return pocketItemSpeech(items.get(i), i + 1, items.size());
                }
            }
        }
        return activity.getString(R.string.a11yPocketEmpty);
    }

    private void resetPocketPicker(boolean announce) {
        String parent = pocketContainerId;
        String layer = pocketReturnParent;
        pocketStage = 0;
        pocketMenuChoice = 0;
        pocketCurrentId = "";
        pocketContainerId = "";
        pocketContext = "";
        pocketDrafts.clear();
        pocketReturnParent = "";
        readerLayerParent = layer;
        if (snapshot != null && snapshot.find(parent) != null) { focus = parent; }
        root.cancelGesture();
        render();
        if (announce) { readCurrent(); }
    }

    /**
     * Cancelling item selection goes back one level, to the
     * pocket list (stage 2), instead of leaving the picker entirely,
     * so the player can pick another pocket without reopening from the
     * start. Only when there is a list to return to: with a single
     * pocket the list would immediately reopen the same pocket, so
     * exiting is still the right outcome. Uncommitted amounts for the
     * abandoned pocket are dropped, like a full reset. Back uses the
     * same rollback as Cancel.
     */
    private void cancelPocketPicker() {
        if (pocketStage == 3 && pocketNodes(pocketContainerId).size() > 1) {
            String pocket = pocketCurrentId;
            pocketDrafts.clear();
            pocketCurrentId = "";
            pocketStage = 2;
            focus = pocket;
            root.cancelGesture();
            render();
            readCurrent();
        } else {
            resetPocketPicker(true);
        }
    }

    private void openPocketChoices() {
        List<ReaderSnapshot.Node> pockets = pocketNodes(pocketContainerId);
        if (pockets.isEmpty()) {
            resetPocketPicker(false);
            feedback(activity.getString(R.string.a11yPocketEmpty));
            return;
        }
        if (pockets.size() == 1) {
            openPocketContents(pockets.get(0));
            return;
        }
        pocketStage = 2;
        focus = pockets.get(0).id;
        root.cancelGesture();
        render();
        feedback(activity.getString(R.string.a11yPocketChoose) + ". " + pocketChoiceSpeech());
    }

    private void openPocketContents(ReaderSnapshot.Node pocket) {
        if ("sealed".equals(pocket.value)) {
            feedback(activity.getString(R.string.a11yPocketSealed) + ". " + pocket.detail);
            return;
        }
        pocketStage = 3;
        pocketCurrentId = pocket.id;
        pocketDrafts.clear();
        List<ReaderSnapshot.Node> items = pocketItems();
        if (items.isEmpty()) {
            boolean onePocket = pocketNodes(pocketContainerId).size() == 1;
            if (onePocket) {
                resetPocketPicker(false);
            } else {
                pocketStage = 2;
                focus = pocket.id;
                render();
            }
            int totalContained;
            try { totalContained = Integer.parseInt(pocket.appliedValue); }
            catch (NumberFormatException missing) { totalContained = 0; }
            feedback(activity.getString(totalContained > 0
                    ? R.string.a11yPocketNotSelectable : R.string.a11yPocketEmpty));
            return;
        }
        focus = items.get(0).id;
        root.cancelGesture();
        render();
        feedback(activity.getString(R.string.a11yPocketItems) + ". " + pocketChoiceSpeech());
    }

    private void selectPocketAmount(ReaderSnapshot.Node item, int amount) {
        int maximum = (int) item.fieldMax;
        if (!item.enabled || amount < 0 || amount > maximum) {
            feedback(item.disabledReason.isEmpty()
                    ? activity.getString(R.string.a11yPocketSelectionError) : item.disabledReason);
            return;
        }
        int applied;
        try { applied = Integer.parseInt(item.appliedValue); }
        catch (NumberFormatException missing) { applied = 0; }
        if (amount == applied) {
            pocketDrafts.remove(item.id);
        } else {
            pocketDrafts.put(item.id, amount);
        }
        if (pocketItems().size() == 1) {
            commitPocketChoices();
        } else {
            render();
            readCurrent();
        }
    }

    private void openPocketQuantityEditor(ReaderSnapshot.Node item) {
        final String itemId = item.id;
        editor = new ReaderTextEditor(activity, Integer.toString(pocketAmount(item)),
                true, true, 0.0, item.fieldMax, 1.0, true, new ReaderTextEditor.Listener() {
            @Override
            public void speak(String text) { feedback(text); }
            @Override
            public void submit(String text) {
                ReaderSnapshot.Node current = snapshot == null ? null : snapshot.find(itemId);
                if (pocketStage != 3 || current == null || !itemId.equals(focus)) { return; }
                int amount;
                try { amount = Integer.parseInt(text); }
                catch (NumberFormatException invalid) {
                    feedback(activity.getString(R.string.a11yEditorInvalid));
                    return;
                }
                editor = null;
                root.cancelGesture();
                selectPocketAmount(current, amount);
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
        feedback(activity.getString(R.string.a11yQuantityInstructions));
    }

    private void commitPocketChoices() {
        if (snapshot == null || pocketStage != 3 ||
                !pocketContext.equals(snapshot.contextKey())) {
            resetPocketPicker(false);
            feedback(activity.getString(R.string.a11yPocketSelectionError));
            return;
        }
        StringBuilder payload = new StringBuilder();
        for (ReaderSnapshot.Node item : pocketItems()) {
            Integer amount = pocketDrafts.get(item.id);
            if (amount == null) { continue; }
            if (!item.enabled || amount < 0 || amount > item.fieldMax) {
                feedback(activity.getString(R.string.a11yPocketSelectionError));
                return;
            }
            if (payload.length() != 0) { payload.append(';'); }
            payload.append(item.id).append('=').append(amount);
        }
        if (payload.length() == 0) {
            resetPocketPicker(true);
            return;
        }
        // The native inbox validates every entry and its live item/pocket
        // before applying any part of this batch. No item is moved here;
        // ordinary outer CONFIRM still performs the pickup.
        String previous = focus;
        focus = pocketCurrentId;
        long request = send("set_value", payload.toString());
        focus = previous;
        if (request == 0) {
            feedback(activity.getString(R.string.a11yPocketSelectionError));
            return;
        }
        resetPocketPicker(false);
        feedback(activity.getString(R.string.a11yPocketSelectionSent));
    }

    private void activatePocketPicker() {
        if (pocketStage == 1) {
            if (pocketMenuChoice == 0) {
                openPocketChoices();
            } else {
                resetPocketPicker(false);
                openGlobalContextActions();
            }
        } else if (pocketStage == 2) {
            ReaderSnapshot.Node pocket = snapshot == null ? null : snapshot.find(focus);
            if (pocket != null) { openPocketContents(pocket); }
        } else if (pocketStage == 3) {
            if (POCKET_CONFIRM_NODE.equals(focus) && pocketAllowsSelection()) {
                commitPocketChoices();
                return;
            }
            if (POCKET_CANCEL_NODE.equals(focus)) {
                cancelPocketPicker();
                return;
            }
            ReaderSnapshot.Node item = snapshot == null ? null : snapshot.find(focus);
            if (item == null) { return; }
            if (!pocketAllowsSelection()) {
                // Inventory pocket entries publish an activate action only
                // when their live item_location can open the normal item menu.
                // Other contexts remain informational as before.
                if (item.enabled && item.actions.contains("activate")) {
                    send("activate", "");
                } else {
                    feedback(pocketChoiceSpeech());
                }
                return;
            }
            if (!item.enabled) {
                feedback(item.disabledReason);
            } else if (item.fieldMax > 1) {
                openPocketQuantityEditor(item);
            } else {
                selectPocketAmount(item, pocketAmount(item) == 0 ? 1 : 0);
            }
        }
    }

    /**
     * Double-tap on a flat-browsed pickup pocket row. A single unit
     * toggles at once; a stack opens the quantity editor. Both commit
     * immediately to the parent "pocket:" row, exactly like a top-level
     * pickup row commits to its own row, and unlike the staged picker,
     * which holds drafts until Confirm. The refreshed snapshot re-reads
     * this row with its new amount (the int field's appliedValue
     * changed), so no extra announcement is needed here.
     */
    private void activateFlatPickupPocketEntry(ReaderSnapshot.Node item) {
        ReaderSnapshot.Node pocket = pickupPocketParent(item);
        if (pocket == null) {
            feedback(activity.getString(R.string.a11yPocketSelectionError));
            return;
        }
        if (item.fieldMax > 1) {
            openFlatPickupQuantityEditor(item);
        } else {
            commitFlatPickupAmount(item, pocket, pocketAmount(item) == 0 ? 1 : 0);
        }
    }

    /** Commit one flat pickup pocket row through its parent pocket.
     *  The native inbox validates the entry against the live pocket
     *  before applying it; entries not mentioned keep their amounts,
     *  so a single-row payload never disturbs other selections. */
    private void commitFlatPickupAmount(ReaderSnapshot.Node item, ReaderSnapshot.Node pocket,
            int amount) {
        if (snapshot == null || amount < 0 || amount > item.fieldMax) {
            feedback(activity.getString(R.string.a11yPocketSelectionError));
            return;
        }
        String previous = focus;
        focus = pocket.id;
        long request = send("set_value", item.id + "=" + amount);
        focus = previous;
        if (request == 0) {
            feedback(activity.getString(R.string.a11yPocketSelectionError));
        }
    }

    /** Quantity editor for a flat-browsed pickup pocket row. It mirrors
     *  the staged picker's editor (immediate commit, TalkBack number
     *  entry by design) but submits to the parent "pocket:" row, the
     *  only node the native side accepts a pocket set_value on. The
     *  parent is re-resolved from the fresh snapshot on submit, never
     *  kept from when the editor opened. */
    private void openFlatPickupQuantityEditor(ReaderSnapshot.Node item) {
        final String itemId = item.id;
        formContext = snapshot.contextKey();
        formRequest = 0;
        editor = new ReaderTextEditor(activity, Integer.toString(pocketAmount(item)),
                true, true, 0.0, item.fieldMax, 1.0, true, new ReaderTextEditor.Listener() {
            @Override
            public void speak(String text) { feedback(text); }
            @Override
            public void submit(String text) {
                if (editor == null || formRequest != 0 || snapshot == null
                        || !itemId.equals(focus)) { return; }
                int amount;
                try { amount = Integer.parseInt(text); }
                catch (NumberFormatException invalid) {
                    feedback(activity.getString(R.string.a11yEditorInvalid));
                    return;
                }
                ReaderSnapshot.Node current = snapshot.find(itemId);
                ReaderSnapshot.Node parent = current == null ? null
                        : pickupPocketParent(current);
                if (parent == null || amount < 0 || amount > current.fieldMax) {
                    feedback(activity.getString(R.string.a11yPocketSelectionError));
                    return;
                }
                String previous = focus;
                focus = parent.id;
                formRequest = send("set_value", itemId + "=" + amount);
                focus = previous;
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
        feedback(activity.getString(R.string.a11yQuantityInstructions));
    }

    private void navigatePocketPicker(int delta) {
        if (pocketStage == 1) {
            pocketMenuChoice = Math.floorMod(pocketMenuChoice + delta, 2);
        } else {
            List<ReaderSnapshot.Node> choices = pocketStage == 2
                    ? pocketNodes(pocketContainerId) : pocketItems();
            if (choices.isEmpty()) { return; }
            int count = choices.size() + (pocketStage == 3
                    ? pocketAllowsSelection() ? 2 : 1 : 0);
            int index = 0;
            if (pocketStage == 3 && POCKET_CONFIRM_NODE.equals(focus)) {
                index = choices.size();
            } else if (pocketStage == 3 && POCKET_CANCEL_NODE.equals(focus)) {
                // Read-only pockets have no Confirm row. Their Cancel row
                // follows the items immediately, not an invisible button.
                index = choices.size() + (pocketAllowsSelection() ? 1 : 0);
            } else {
                for (int i = 0; i < choices.size(); i++) {
                    if (choices.get(i).id.equals(focus)) { index = i; break; }
                }
            }
            int next = Math.floorMod(index + delta, count);
            focus = next < choices.size() ? choices.get(next).id
                    : pocketAllowsSelection() && next == choices.size()
                    ? POCKET_CONFIRM_NODE : POCKET_CANCEL_NODE;
        }
        render();
        feedback(pocketChoiceSpeech());
    }

    private void renderPocketPicker() {
        TextView title = new TextView(activity);
        title.setText(pocketStage == 1 ? R.string.a11yPocketActions :
                pocketStage == 2 ? R.string.a11yPocketChoose : R.string.a11yPocketItems);
        title.setTextColor(0xffeeeeee);
        title.setTextSize(20);
        rows.addView(title);
        if (pocketStage == 1) {
            int[] ids = { R.string.a11yPocketOpen, R.string.a11yPocketOtherActions };
            for (int i = 0; i < ids.length; i++) {
                final int choice = i;
                Button option = new Button(activity);
                option.setText(ids[i]);
                option.setSelected(i == pocketMenuChoice);
                option.setOnClickListener(v -> {
                    pocketMenuChoice = choice;
                    activatePocketPicker();
                });
                rows.addView(option);
            }
        } else {
            List<ReaderSnapshot.Node> items = pocketStage == 2
                    ? pocketNodes(pocketContainerId) : pocketItems();
            int selected = 0;
            for (int i = 0; i < items.size(); i++) {
                if (items.get(i).id.equals(focus)) { selected = i; break; }
            }
            int start = (selected / HUB_PAGE_SIZE) * HUB_PAGE_SIZE;
            int end = Math.min(start + HUB_PAGE_SIZE, items.size());
            if (start > 0) {
                Button previous = new Button(activity);
                previous.setText(R.string.a11yPreviousPage);
                previous.setOnClickListener(v -> {
                    focus = items.get(start - 1).id;
                    render();
                    feedback(pocketChoiceSpeech());
                });
                rows.addView(previous);
            }
            for (int i = start; i < end; i++) {
                ReaderSnapshot.Node item = items.get(i);
                Button button = new Button(activity);
                button.setText(pocketStage == 2 ? nodeText(item, false)
                        : pocketItemSpeech(item, i + 1, items.size()));
                button.setSelected(item.id.equals(focus));
                button.setEnabled(pocketStage == 2 || item.enabled);
                button.setOnClickListener(v -> {
                    focus = item.id;
                    activatePocketPicker();
                });
                rows.addView(button);
            }
            if (end < items.size()) {
                Button next = new Button(activity);
                next.setText(R.string.a11yNextPage);
                next.setOnClickListener(v -> {
                    focus = items.get(end).id;
                    render();
                    feedback(pocketChoiceSpeech());
                });
                rows.addView(next);
            }
            if (pocketStage == 3 && pocketAllowsSelection()) {
                Button confirm = new Button(activity);
                confirm.setText(R.string.a11yPocketConfirm);
                confirm.setSelected(POCKET_CONFIRM_NODE.equals(focus));
                confirm.setOnClickListener(v -> {
                    focus = POCKET_CONFIRM_NODE;
                    commitPocketChoices();
                });
                rows.addView(confirm);
            }
        }
        Button cancel = new Button(activity);
        cancel.setText(R.string.a11yPocketCancel);
        cancel.setSelected(POCKET_CANCEL_NODE.equals(focus));
        cancel.setOnClickListener(v -> cancelPocketPicker());
        rows.addView(cancel);
    }

    private void openGlobalContextActions() {
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

    /** True while inventory items use distinct horizontal and vertical axes: the
     *  pocket picker is closed and no tree layer is entered. */
    private boolean flattenedInventoryBrowse() {
        return snapshot != null && "inventory".equals(snapshot.category)
                && pocketStage == 0 && readerLayerParent.isEmpty() && !commandPaletteOpen;
    }

    /** A pocket-content row reached by flat inventory browsing: its
     *  parent is a "pocket:" status row, never a list of its own. */
    private boolean isFlattenedPocketEntry(ReaderSnapshot.Node node) {
        return node != null && node.id.startsWith("pocket-entry:")
                && (!"checkbox".equals(node.role) || pickupPocketParent(node) != null)
                && flattenedInventoryBrowse();
    }

    /** The "pocket:" parent of a pickup pocket-entry row, or null.
     *  Only pickup publishes checkbox pocket rows, and only there does
     *  the parent accept the atomic set_value commit that selects
     *  pocket contents. Ordinary-inventory pocket rows are buttons
     *  whose parent has no set_value action, so they never match. */
    private ReaderSnapshot.Node pickupPocketParent(ReaderSnapshot.Node node) {
        if (snapshot == null || node == null || !node.id.startsWith("pocket-entry:")) {
            return null;
        }
        ReaderSnapshot.Node parent = snapshot.find(node.parentId);
        return parent != null && parent.id.startsWith("pocket:")
                && parent.actions.contains("set_value") ? parent : null;
    }

    /** A pickup pocket-content row in the flat swipe list: a checkbox
     *  "pocket-entry:" row whose parent "pocket:" row accepts the
     *  atomic set_value commit. Unlike the staged picker's drafts,
     *  which are committed together on Confirm, a flat row commits
     *  immediately on double-tap, exactly like a top-level pickup row. */
    private boolean isFlatPickupPocketEntry(ReaderSnapshot.Node node) {
        return node != null && "checkbox".equals(node.role)
                && pickupPocketParent(node) != null && flattenedInventoryBrowse();
    }

    /** Pocket contents of one inventory item, flattened in snapshot
     *  order: every "pocket-entry:" descendant of the item, across all
     *  of its pockets and nested containers, skipping the "pocket:"
     *  status rows themselves. A pickup checkbox row joins the list
     *  only when its parent "pocket:" row accepts the atomic set_value
     *  commit, so double-tap can select it; any other checkbox row
     *  stays out, and the staged picker itself is untouched. */
    private List<ReaderSnapshot.Node> flattenedPocketContents(String itemId) {
        ArrayList<ReaderSnapshot.Node> contents = new ArrayList<>();
        if (snapshot == null) { return contents; }
        for (ReaderSnapshot.Node node : snapshot.nodes) {
            if (!node.id.startsWith("pocket-entry:")) {
                continue;
            }
            if ("checkbox".equals(node.role) && pickupPocketParent(node) == null) {
                continue;
            }
            ReaderSnapshot.Node cursor = node;
            while (!cursor.parentId.isEmpty()) {
                if (cursor.parentId.equals(itemId)) { contents.add(node); break; }
                cursor = snapshot.find(cursor.parentId);
                if (cursor == null) { break; }
            }
        }
        return contents;
    }

    /** Resolve a vertically focused pocket entry back to its root item. */
    private ReaderSnapshot.Node inventoryRootItem(ReaderSnapshot.Node node) {
        if (snapshot == null) { return null; }
        for (int steps = 0; node != null && !node.parentId.isEmpty()
                && steps < snapshot.nodes.size(); steps++) {
            node = snapshot.find(node.parentId);
        }
        return node != null && node.parentId.isEmpty()
                && node.id.startsWith("item:") ? node : null;
    }

    /** Horizontal swipes visit top-level inventory/pickup rows; vertical
     *  swipes visit the focused container's contents, including nested
     *  pockets. Double-tap continues to select or open the quantity editor. */
    @Override
    public boolean handleInventoryAxisSwipe(int horizontal, int vertical) {
        if (!flattenedInventoryBrowse() || comprehensiveMenu || editor != null
                || debugReport != null || documentReader.isOpen()
                || !foreground || !windowFocused || lastSuspended
                || (horizontal == 0 && vertical == 0)) {
            return false;
        }
        ReaderSnapshot.Node root = inventoryRootItem(snapshot.find(focus));
        if (horizontal != 0) {
            List<ReaderSnapshot.Node> roots = visibleNodes();
            if (roots.isEmpty()) { return false; }
            String rootId = root == null ? focus : root.id;
            int current = -1;
            for (int i = 0; i < roots.size(); i++) {
                if (roots.get(i).id.equals(rootId)) { current = i; break; }
            }
            focus = roots.get(current < 0 ? 0
                    : Math.floorMod(current + horizontal, roots.size())).id;
        } else {
            if (root == null) { return false; }
            List<ReaderSnapshot.Node> contents = flattenedPocketContents(root.id);
            if (contents.isEmpty()) { readCurrent(); return true; }
            int current = -1;
            for (int i = 0; i < contents.size(); i++) {
                if (contents.get(i).id.equals(focus)) { current = i; break; }
            }
            int next = current < 0 ? (vertical > 0 ? 0 : contents.size() - 1)
                    : Math.floorMod(current + vertical, contents.size());
            focus = contents.get(next).id;
        }
        render();
        send("focus", "");
        readCurrent();
        return true;
    }

    private List<ReaderSnapshot.Node> visibleNodes() {
        ArrayList<ReaderSnapshot.Node> result = new ArrayList<>();
        if (snapshot == null) { return result; }
        if (commandPaletteOpen) { return contextualCommands(); }
        for (ReaderSnapshot.Node node : snapshot.nodes) {
            // Group 2 capability markers must not appear as world rows.
            if (node.id.startsWith("reader:nearby:available:")) { continue; }
            if (readerLayerParent.equals(node.parentId)) { result.add(node); }
        }
        if ("targeting".equals(snapshot.category)
                && snapshot.find("target:fire-direction:0") != null) {
            if (targetingOriginalControls) { return result; }
            result.removeIf(node -> !node.id.startsWith("target:fire-direction:")
                    && !"target:auto-reload".equals(node.id));
            return result;
        }
        // Look-around is itself a cursor-control dialog. Hiding every
        // command behind long press leaves only the read-only cursor and
        // makes every swipe repeat "1 of 36". Keep its controls navigable,
        // especially its dedicated return-to-map action.
        if ("look-around".equals(snapshot.category)) { return result; }
        // Command-only screens keep their complete list. A mixed screen
        // announces primary content first and moves all commands to long press.
        boolean containsContent = false;
        for (ReaderSnapshot.Node node : result) {
            if (!isAuxiliaryCommand(node)) { containsContent = true; break; }
        }
        if (containsContent) {
            // In minigames the direction commands are the
            // controls the player uses on every move; keep them in the
            // swipe list (double-tap to move) instead of hiding them
            // behind long press. Every other command stays behind
            // long press, as on every other mixed screen.
            if (isMinigameCategory(snapshot.category)) {
                result.removeIf(node -> isAuxiliaryCommand(node) && !isDirectionCommand(node));
            } else {
                result.removeIf(GameReaderController::isAuxiliaryCommand);
            }
        } else if ("crafting".equals(snapshot.category)
                || "construction".equals(snapshot.category)
                || "inventory".equals(snapshot.category)
                || "zones".equals(snapshot.category)
                || "surroundings".equals(snapshot.category)) {
            // Content-first screens must not become a command palette during
            // an empty/early snapshot. Native adapters publish an explicit
            // empty-state item; auxiliary commands stay behind long press.
            result.clear();
        }
        return result;
    }

    @Override
    public void openContextActions() {
        if (debugReport != null || closed) { return; }
        if (pocketStage != 0) {
            resetPocketPicker(true);
            return;
        }
        if (commandPaletteOpen) { closeContextActions(); return; }
        if (comprehensiveMenu) {
            showHubActionChoices();
            return;
        }
        if (vehicleMode && directWorldGesturesEnabled()) {
            // Drive-mode hold opens the real native vehicle interaction menu,
            // not the global action hub or an extra simulated overlay.
            if (!vehicleDriverReady()) {
                feedback(activity.getString(R.string.a11yDriveNotReady));
                return;
            }
            ReaderSnapshot.Node command = snapshot.find("control_vehicle");
            if (command == null || !command.enabled
                    || !command.actions.contains("activate")) {
                feedback(activity.getString(R.string.a11yDriveActionUnavailable));
                return;
            }
            requestLiveWorldCommand("control_vehicle");
            return;
        }
        if (combatMode && directWorldGesturesEnabled()) {
            // Long press opens the existing combat menu with the aiming
            // mode switch as its first row, not an extra modal overlay.
            openHubSection(GameActionHub.COMBAT);
            return;
        }
        if (snapshot != null && "targeting".equals(snapshot.category)
                && snapshot.find("target:fire-direction:0") != null) {
            targetingOriginalControls = !targetingOriginalControls;
            focus = targetingOriginalControls ? "target:cursor" : "target:fire-direction:0";
            readerLayerParent = "";
            root.cancelGesture();
            render();
            ReaderSnapshot.Node selected = snapshot.find(focus);
            feedback(activity.getString(targetingOriginalControls
                    ? R.string.a11yTargetOriginalControls : R.string.a11yTargetDirections)
                    + (selected == null ? "" : ". " + nodeText(selected, false)));
            return;
        }
        if (travelState() != 0) {
            feedback(travelDetailSpeech(travelState()));
            return;
        }
        if (editor != null || snapshot == null || directWorldGesturesEnabled()) { return; }
        // Long-press on an inventory item opens a local action list with
        // "Open pocket", leaving double-tap reserved for selecting the item.
        ReaderSnapshot.Node focused = snapshot.find(focus);
        if ("inventory".equals(snapshot.category) && focused != null
                && ("checkbox".equals(focused.role) || "button".equals(focused.role))
                && focused.id.startsWith("item:")
                && !pocketNodes(focused.id).isEmpty()) {
            pocketContext = snapshot.contextKey();
            pocketContainerId = focused.id;
            pocketReturnParent = readerLayerParent;
            pocketCurrentId = "";
            pocketDrafts.clear();
            pocketStage = 1;
            pocketMenuChoice = 0;
            root.cancelGesture();
            render();
            feedback(activity.getString(R.string.a11yPocketActions) + ". " + pocketChoiceSpeech());
            return;
        }
        openGlobalContextActions();
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

    private int travelState() {
        if (snapshot == null || debugReport != null || editor != null
                || comprehensiveMenu || pocketStage != 0 || commandPaletteOpen
                || (!"activity".equals(snapshot.category)
                    && !"DEFAULTMODE".equals(snapshot.category)
                    && !"overmap".equals(snapshot.category)
                    && !"look-around".equals(snapshot.category))) {
            return 0;
        }
        try {
            return CataclysmDDA.nativeTravelState();
        } catch (UnsatisfiedLinkError unavailable) {
            return 0;
        }
    }

    private int travelChoiceCount(int state) {
        return state == 3 ? 3 : 2;
    }

    private int travelLabel(int state, int index) {
        if (state == 3) {
            return index == 0 ? R.string.a11yTravelIgnoreWarning
                    : index == 1 ? R.string.a11yTravelResume
                    : R.string.a11yTravelCancel;
        }
        return index == 0 ? (state == 2 ? R.string.a11yTravelResume
                : R.string.a11yTravelPause) : R.string.a11yTravelCancel;
    }

    private String travelStatus(int state) {
        return activity.getString(state == 3 ? R.string.a11yTravelSafetyStopped
                : state == 2 ? R.string.a11yTravelPaused
                : R.string.a11yTravelMoving);
    }

    private String travelSpoken(int state) {
        int count = travelChoiceCount(state);
        travelChoice = Math.max(0, Math.min(travelChoice, count - 1));
        return activity.getString(travelLabel(state, travelChoice)) + ". "
                + activity.getString(R.string.a11yControlButton) + ", "
                + activity.getString(R.string.a11yPosition, travelChoice + 1, count);
    }

    private void activateTravel() {
        int state = travelState();
        if (state == 0) { return; }
        int command = state == 3 ? (travelChoice == 0 ? 4
                : travelChoice == 1 ? 2 : 3)
                : travelChoice == 0 ? (state == 2 ? 2 : 1) : 3;
        try {
            if (CataclysmDDA.nativeTravelControl(command)) {
                SessionDebugLog.event("TRAVEL_CONTROL",
                        "command=" + command + " from_state=" + state);
                feedback(activity.getString(R.string.a11yTravelRequestAccepted));
            } else {
                feedback(activity.getString(R.string.a11yTravelRequestUnavailable));
            }
        } catch (UnsatisfiedLinkError unavailable) {
            feedback(activity.getString(R.string.a11yReaderNativeUnavailable));
        }
    }

    private static void appendTravelSentence(StringBuilder description, String content) {
        if (content == null || content.trim().isEmpty()) { return; }
        String sentence = content.trim();
        if (description.length() > 0) {
            char last = description.charAt(description.length() - 1);
            if (last != '.' && last != '!' && last != '?') {
                description.append('.');
            }
            description.append(' ');
        }
        description.append(sentence);
    }

    private String travelDetailSpeech(int state) {
        StringBuilder description = new StringBuilder(travelStatus(state));
        if (snapshot != null) {
            ReaderSnapshot.Node active = snapshot.find("activity:current");
            ReaderSnapshot.Node messages = snapshot.find("world:messages");
            if (active != null) {
                appendTravelSentence(description, active.value);
                appendTravelSentence(description, active.detail);
            }
            if (messages != null) {
                appendTravelSentence(description, messages.detail);
            }
        }
        return description.toString();
    }

    private void renderTravelPanel(int state) {
        TextView title = new TextView(activity);
        title.setText(travelStatus(state));
        title.setTextColor(0xffeeeeee);
        title.setTextSize(19);
        title.setPadding(12, 12, 12, 16);
        rows.addView(title);
        ReaderSnapshot.Node current = snapshot == null ? null : snapshot.find("activity:current");
        if (current != null && !current.value.isEmpty()) {
            TextView progress = new TextView(activity);
            progress.setText(current.value);
            progress.setTextColor(0xffdddddd);
            rows.addView(progress);
        }
        for (int i = 0; i < travelChoiceCount(state); i++) {
            final int choice = i;
            Button button = new Button(activity);
            button.setText(travelLabel(state, i));
            button.setSelected(i == travelChoice);
            button.setOnClickListener(view -> {
                travelChoice = choice;
                activateTravel();
            });
            rows.addView(button);
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
        } else if (pocketStage != 0) {
            root.setBackgroundColor(0xff161616);
            renderPocketPicker();
        } else if (commandPaletteOpen) {
            root.setBackgroundColor(0xff161616);
            renderContextCommands();
        } else if (travelState() != 0) {
            root.setBackgroundColor(0xff161616);
            renderTravelPanel(travelState());
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
        if (travelState() != 0) {
            return presentationEpoch + ":travel:" + travelState();
        }
        if (comprehensiveMenu) {
            return presentationEpoch + ":hub:" + hubSection + ":" + choosingGesture
                    + ":" + choosingHubAction
                    + ":" + (bindingItem == null ? "" : bindingItem.shortcutTarget())
                    + ":" + (hubActionItem == null ? "" : hubActionItem.shortcutTarget())
                    + (editor == null ? "" : ":editor:" + editor.contextKey());
        }
        // A snapshot revision is not a new input screen. Keep gestures
        // alive when the same native session/frame/generation merely
        // republishes its contents, but invalidate across modal/lifecycle
        // boundaries and across opening or closing a local editor.
        if (snapshot != null) {
            return presentationEpoch + ":screen:" + snapshot.session + ":"
                    + snapshot.frame + ":" + snapshot.generation
                    + ":" + snapshot.category
                    + (pocketStage == 0 ? "" : ":pocket:" + pocketStage + ":" + pocketCurrentId)
                    + (editor == null ? "" : ":editor:" + editor.contextKey());
        }
        return presentationEpoch + ":waiting";
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
        pendingLinkedDetail = "";
        pendingLinkedDetailContext = "";
        pendingItemDetailNode = "";
        pendingItemDetailContext = "";
        pendingItemDetailParts = null;
        SessionDebugLog.event("NAVIGATE", "delta=" + delta + " focus=" + focus
                + " hub=" + comprehensiveMenu);
        if (delta == 0) { readCurrent(); return; }
        if (debugReport != null) {
            debugFocus = Math.floorMod(debugFocus + delta, 4);
            readCurrent();
            return;
        }
        if (editor != null) { editor.navigate(delta); return; }
        if (snapshot == null && !comprehensiveMenu) {
            // Native loading screens do not publish a focusable list.
            // A swipe here is a request for the latest loading status,
            // not a silent attempt to navigate the old menu.
            readCurrent();
            return;
        }
        if (comprehensiveMenu) {
            if (choosingHubAction) {
                hubActionChoice = Math.floorMod(hubActionChoice + delta, 2);
            } else if (choosingGesture) {
                gestureChoice = Math.floorMod(gestureChoice + delta,
                        ReaderGestureBindings.CUSTOM_IDS.length + 2);
            } else {
                List<GameActionHub.Entry> options = hubEntries();
                if (options.isEmpty()) { return; }
                hubIndex = Math.floorMod(hubIndex + delta, options.size());
            }
            render();
            readCurrent();
            return;
        }
        if (pocketStage != 0) {
            navigatePocketPicker(delta);
            return;
        }
        if (travelState() != 0) {
            int state = travelState();
            travelChoice = Math.floorMod(travelChoice + delta, travelChoiceCount(state));
            render();
            readCurrent();
            return;
        }
        // Once a long document is open, ordinary left/right and up/down
        // navigation advances the document itself, not the single parent
        // item. Back returns to that parent list without closing the screen.
        if (documentReader.isOpen() && !commandPaletteOpen) {
            // Replace the still-playing full document when the player asks
            // to navigate a paragraph, rather than queueing speech behind it.
            speech.stop();
            speakDocument(delta > 0 ? documentReader.nextParagraph()
                    : documentReader.previousParagraph());
            return;
        }
        List<ReaderSnapshot.Node> filtered = visibleNodes();
        if (filtered.isEmpty()) { return; }
        // The recipe list and the detail documents share one native snapshot,
        // but one-finger LEFT/RIGHT means recipes ONLY. Never wrap from the
        // first recipe to "Crafting conditions"/tool requirements at the end.
        if ("crafting".equals(snapshot.category) && !commandPaletteOpen) {
            List<ReaderSnapshot.Node> recipeNodes = new ArrayList<>();
            for (ReaderSnapshot.Node node : filtered) {
                if (node.id.startsWith("recipe:")) { recipeNodes.add(node); }
            }
            if (!recipeNodes.isEmpty()) {
                int recipeIndex = -1;
                for (int i = 0; i < recipeNodes.size(); i++) {
                    if (recipeNodes.get(i).id.equals(focus)) {
                        recipeIndex = i;
                        break;
                    }
                }
                if (recipeIndex < 0) {
                    focus = recipeNodes.get(0).id;
                    render();
                    send("focus", "");
                    readCurrent();
                    return;
                }
                int nextRecipe = recipeIndex + (delta > 0 ? 1 : -1);
                if (nextRecipe >= recipeNodes.size()) {
                    if (craftingCommand("craft:page:next")) { return; }
                    readCurrent(); // Already at the last recipe.
                    return;
                }
                if (nextRecipe < 0) {
                    if (craftingCommand("craft:page:previous")) { return; }
                    readCurrent(); // Already at the first recipe.
                    return;
                }
                focus = recipeNodes.get(nextRecipe).id;
                render();
                send("focus", "");
                readCurrent();
                return;
            }
        }
        // Surroundings uses 80-row native pages to bound snapshot size.
        // Left/right advance across a page boundary without making the
        // player navigate artificial "Next page" list items.
        if ("surroundings".equals(snapshot.category) && !commandPaletteOpen) {
            if (delta > 0 && index() == filtered.size() - 1
                    && surroundingsCommand("nearby:page:next")) {
                return;
            }
            if (delta < 0 && index() == 0
                    && surroundingsCommand("nearby:page:previous")) {
                return;
            }
        }
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

    // A stacked item row in inventory/pickup is an immediate quantity
    // picker, not a setting: a one-finger vertical swipe browses to the
    // next/previous item, like every other row, instead of changing the
    // chosen amount. The amount is chosen by double-tapping the row,
    // which opens the quantity editor (openFormEditor's quickQuantity
    // path). The same node shape elsewhere keeps swipe-to-adjust: real
    // sliders use role "slider" (world generation) or carry an "apply"
    // action (options, safemode, smart controller, auto-pickup), and the
    // standalone number-input popup lives in the NUMBER_INPUT category.
    // Published shape (inventory_ui.cpp quantity form, handle_action.cpp
    // pickup-here): role "checkbox", int field, range 0..available,
    // step 1, set_value without apply, only when available > 1.
    private boolean isItemQuantityRow(ReaderSnapshot.Node node) {
        return "checkbox".equals(node.role)
                && "int".equals(node.fieldType) && node.hasFieldRange
                && node.fieldMin == 0.0 && node.fieldMax > 1.0 && node.fieldStep == 1.0
                && node.actions.contains("set_value") && !node.actions.contains("apply")
                && ("inventory".equals(snapshot.category)
                        || "pickup-here".equals(snapshot.category));
    }

    @Override
    public boolean adjustFocusedSetting(int delta) {
        if (debugReport != null || comprehensiveMenu || pocketStage != 0) { return false; }
        if (editor != null || snapshot == null || delta == 0
                || !foreground || !windowFocused || lastSuspended) { return false; }
        ReaderSnapshot.Node node = snapshot.find(focus);
        if (node == null || !node.enabled) { return false; }

        // Stacked item rows are browsed, not adjusted: the vertical
        // swipe falls through to navigation in ReaderGestureSurface.
        if (isItemQuantityRow(node)) { return false; }

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

    // Only the first six character-creation tabs opt in to step gestures.
    // The summary tab and all unrelated menus keep their existing controls.
    private boolean isCreatorMainStep() {
        return snapshot != null
                && snapshot.category.startsWith("character-creator-step-");
    }

    private boolean isCreatorMultiChoiceStep() {
        if (!isCreatorMainStep()) { return false; }
        return "character-creator-step-3".equals(snapshot.category)
                || "character-creator-step-4".equals(snapshot.category)
                || "character-creator-step-5".equals(snapshot.category)
                || "character-creator-step-6".equals(snapshot.category);
    }

    private boolean creatorCalendarAction(String action) {
        if (snapshot == null || !"character-creator-calendar".equals(snapshot.category)) {
            return false;
        }
        ReaderSnapshot.Node command = snapshot.find("action:" + action);
        if (command == null || !command.enabled || !command.actions.contains("activate")) {
            feedback(activity.getString(R.string.a11yContextConfirmUnavailable));
            return true;
        }
        String previous = focus;
        focus = command.id;
        send("activate", "");
        focus = previous;
        return true;
    }

    @Override
    public boolean handleCreatorCalendarSwipe(int horizontal, int vertical) {
        if (snapshot == null || !"character-creator-calendar".equals(snapshot.category)
                || commandPaletteOpen || editor != null || comprehensiveMenu
                || !foreground || !windowFocused || lastSuspended) {
            return false;
        }
        if (horizontal != 0) {
            // The native calendar adjusts the currently highlighted
            // day/hour in response to its existing LEFT/RIGHT actions.
            return creatorCalendarAction(horizontal > 0 ? "RIGHT" : "LEFT");
        }
        if (vertical != 0) {
            // Only day and hour are published in the concise creator calendar.
            navigate(vertical > 0 ? 1 : -1);
            return true;
        }
        return false;
    }

    /** Use native crafting commands, never synthesize a recipe selection. */
    private boolean craftingCommand(String id) {
        if (snapshot == null || !"crafting".equals(snapshot.category)) { return false; }
        ReaderSnapshot.Node node = snapshot.find(id);
        if (node == null || !node.enabled || !node.actions.contains("activate")) {
            return false;
        }
        String previous = focus;
        craftingNavigationPending = true;
        focus = id;
        send("activate", "");
        focus = previous;
        return true;
    }

    @Override
    public boolean handleCraftingMainCategorySwipe(int horizontal) {
        if (horizontal == 0 || snapshot == null || !"crafting".equals(snapshot.category)
                || commandPaletteOpen || comprehensiveMenu || editor != null
                || debugReport != null || !foreground || !windowFocused
                || lastSuspended || documentReader.isOpen()) { return false; }
        return craftingCommand(horizontal < 0 ? "PREV_TAB" : "NEXT_TAB");
    }

    @Override
    public boolean handleCraftingRecipeSwipe(int horizontal) {
        if (horizontal == 0 || snapshot == null || !"crafting".equals(snapshot.category)
                || commandPaletteOpen || comprehensiveMenu || editor != null
                || debugReport != null || !foreground || !windowFocused
                || lastSuspended || documentReader.isOpen()) { return false; }
        navigate(horizontal > 0 ? 1 : -1);
        return true;
    }

    @Override
    public boolean handleCraftingSubcategorySwipe(int vertical) {
        if (vertical == 0 || snapshot == null || !"crafting".equals(snapshot.category)
                || commandPaletteOpen || comprehensiveMenu || editor != null
                || debugReport != null || !foreground || !windowFocused
                || lastSuspended || documentReader.isOpen()) { return false; }
        ReaderSnapshot.Node selected = snapshot.find(focus);
        if (selected != null && ("number".equals(selected.role)
                || !selected.fieldType.isEmpty())) {
            // Numeric and choice fields keep their existing editing gestures.
            return false;
        }
        return craftingCommand(vertical < 0 ? "LEFT" : "RIGHT");
    }

    /** Category and page controls are never targets; send a native,
     * scope-bound activation while preserving the player's current focus.
     * The engine returns an updated list and is the only owner of routing.
     */
    private boolean surroundingsCommand(String id) {
        if (snapshot == null || !"surroundings".equals(snapshot.category)) {
            return false;
        }
        ReaderSnapshot.Node command = snapshot.find(id);
        if (command == null || !command.enabled || !command.actions.contains("activate")) {
            return false;
        }
        String previous = focus;
        focus = command.id;
        send("activate", "");
        focus = previous;
        return true;
    }

    @Override
    public boolean handleSurroundingsSwipe(int horizontal, int vertical) {
        if (snapshot == null || !"surroundings".equals(snapshot.category)
                || commandPaletteOpen || comprehensiveMenu || editor != null
                || debugReport != null || !foreground || !windowFocused
                || lastSuspended || documentReader.isOpen()) {
            return false;
        }
        if (vertical != 0) {
            // One finger up = next category; down = previous, wrapping
            // through All, Creatures/NPCs, Places/Environment, and Items.
            return surroundingsCommand(vertical < 0
                    ? "nearby:filter:next" : "nearby:filter:previous");
        }
        if (horizontal != 0) {
            // The category heading was read on entry. The first right
            // swipe reads item one, then later swipes move to item two.
            // Left immediately wraps to the last item in the category.
            if (horizontal > 0 && nearbyFirstItemPending) {
                nearbyFirstItemPending = false;
                readCurrent();
                return true;
            }
            nearbyFirstItemPending = false;
            // Fixed left/right focus order, independent of movement or
            // user-configured swipe actions on unrelated screens.
            navigate(horizontal > 0 ? 1 : -1);
            return true;
        }
        return false;
    }

    private boolean creatorStepAction(String action) {
        if (!isCreatorMainStep()) { return false; }
        ReaderSnapshot.Node command = snapshot.find("action:" + action);
        if (command == null || !command.enabled || !command.actions.contains("activate")) {
            feedback(activity.getString(R.string.a11yContextConfirmUnavailable));
            return true;
        }
        String previous = focus;
        focus = command.id;
        send("activate", "");
        focus = previous;
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
        if (pocketStage != 0) {
            if (pocketStage == 3 && pocketItems().size() > 1) {
                commitPocketChoices();
            } else {
                activatePocketPicker();
            }
            return;
        }
        if (travelState() != 0) { activateTravel(); return; }
        if (snapshot == null || directWorldGesturesEnabled()) { return; }
        // Two fingers up advances the multi-selection/attribute/trait/skill
        // tabs, instead of confirming whichever row happens to be focused.
        // On the first two tabs, confirmation still activates the row and
        // native code advances only after a valid single choice.
        if (isCreatorMultiChoiceStep() && creatorStepAction("NEXT_TAB")) { return; }
        if (creatorCalendarAction("CALENDAR_DONE")) { return; }
        // Summary publishes native command IDs, not UILIST action: IDs.
        // Two fingers up must invoke the same native NEXT_TAB validation
        // and Yes/No completion prompt as the visible Finish control.
        if ("CHARACTER_CREATION".equals(snapshot.category)) {
            ReaderSnapshot.Node finish = snapshot.find("NEXT_TAB");
            if (finish != null && finish.enabled && finish.actions.contains("activate")) {
                String previous = focus;
                focus = finish.id;
                send("activate", "");
                focus = previous;
                return;
            }
        }
        // The crafting detail pane owns its own explicit Craft action.
        // Confirm must commit its selected quantity through that action,
        // never the generic CRAFTING/CONFIRM command from the command tail.
        if ("crafting".equals(snapshot.category)) {
            // Number-choice screen is not a craft confirmation screen.
            if (snapshot.find("craft:detail:qty:current") != null) {
                feedback(activity.getString(R.string.a11yContextConfirmUnavailable));
                return;
            }
            ReaderSnapshot.Node craft = snapshot.find("craft:detail:confirm");
            if (craft != null) {
                if (!craft.enabled || !craft.actions.contains("activate")) {
                    feedback(activity.getString(R.string.a11yContextConfirmUnavailable));
                    return;
                }
                String previous = focus;
                focus = craft.id;
                send("activate", "");
                focus = previous;
                return;
            }
        }
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

    private static boolean isWorldDirection(String nodeId) {
        return "UP".equals(nodeId) || "DOWN".equals(nodeId)
                || "LEFT".equals(nodeId) || "RIGHT".equals(nodeId)
                || "LEFTUP".equals(nodeId) || "LEFTDOWN".equals(nodeId)
                || "RIGHTUP".equals(nodeId) || "RIGHTDOWN".equals(nodeId);
    }

    private static String actionForDirection(int dx, int dy) {
        if (dy < 0) { return dx < 0 ? "LEFTUP" : dx > 0 ? "RIGHTUP" : "UP"; }
        if (dy > 0) { return dx < 0 ? "LEFTDOWN" : dx > 0 ? "RIGHTDOWN" : "DOWN"; }
        return dx < 0 ? "LEFT" : "RIGHT";
    }

    private boolean worldStepAllowed() {
        // Two-finger combat moves pass through the same native walking
        // gate, with the normal activity, vehicle and travel safety checks.
        return directWorldGesturesEnabled() && snapshot.swipeMovementAllowed
                && hasCardinalDirections(snapshot);
    }

    @Override
    public boolean worldMovementEnabled() {
        // One finger attacks during combat; two fingers move. Outside
        // combat, the original one-finger map movement is unchanged.
        return !combatMode && worldStepAllowed();
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

    private boolean vehicleDriverReady() {
        ReaderSnapshot.Node state = snapshot == null
                ? null : snapshot.find("reader:vehicle:driver");
        return state != null && state.enabled;
    }

    /** Use only actions published by the current native world frame. A
     * passenger cannot command the vehicle, even with stale Android focus. */
    private void requestVehicleAction(String id) {
        if (!vehicleMode || !directWorldGesturesEnabled()) { return; }
        if (!vehicleDriverReady()) {
            feedback(activity.getString(R.string.a11yDriveNotReady));
            return;
        }
        ReaderSnapshot.Node node = snapshot.find(id);
        if (node == null || !node.enabled || !node.actions.contains("activate")) {
            feedback(activity.getString(R.string.a11yDriveActionUnavailable));
            return;
        }
        if ("LEFT".equals(id) || "RIGHT".equals(id)
                || id.startsWith("reader:vehicle:heading:")) {
            pendingVehicleRoadReport = true;
        } else if ("reader:vehicle:pause".equals(id)
                || "reader:vehicle:forward".equals(id)
                || "reader:vehicle:reverse".equals(id)
                || "reader:vehicle:faster".equals(id)
                || "reader:vehicle:slower".equals(id)) {
            pendingVehicleSpeedReport = true;
        }
        requestLiveWorldCommand(id);
    }

    private void speakVehicleDocument(String id) {
        ReaderSnapshot.Node doc = snapshot == null ? null : snapshot.find(id);
        if (doc == null) {
            feedback(activity.getString(R.string.a11yDriveActionUnavailable));
            return;
        }
        String spoken = doc.detail.isEmpty() ? doc.value : doc.detail;
        if (spoken == null || spoken.isEmpty()) {
            feedback(activity.getString(R.string.a11yDriveActionUnavailable));
        } else {
            feedback(spoken);
        }
    }

    /** Two-finger swipes are reserved in driving mode, before map scan. */
    @Override
    public boolean handleVehicleTwoFingerSwipe(int horizontal, int vertical) {
        if (!vehicleMode || !directWorldGesturesEnabled()) { return false; }
        if (vertical < 0) {
            speakVehicleDocument("reader:vehicle:speed");
        } else if (vertical > 0) {
            speakVehicleDocument("reader:vehicle:detail");
        } else if (horizontal < 0) {
            requestVehicleAction("reader:vehicle:brake");
        } else if (horizontal > 0) {
            requestVehicleAction("reader:vehicle:horn");
        }
        return true;
    }

    @Override
    public boolean handleCustomGesture(String gestureId) {
        if (vehicleMode && directWorldGesturesEnabled()) {
            // The native game still handles acceleration, direction and
            // steering physics. Map/Combat bindings cannot override these.
            switch (gestureId) {
                case "swipe_up":
                    requestVehicleAction("reader:vehicle:forward");
                    return true;
                case "swipe_down":
                    requestVehicleAction("reader:vehicle:reverse");
                    return true;
                case "swipe_left":
                    requestVehicleAction("LEFT");
                    return true;
                case "swipe_right":
                    requestVehicleAction("RIGHT");
                    return true;
                case "swipe_up_down":
                    requestVehicleAction("reader:vehicle:faster");
                    return true;
                case "swipe_down_up":
                    requestVehicleAction("reader:vehicle:slower");
                    return true;
                case "swipe_left_right":
                    requestVehicleAction("reader:vehicle:heading:left");
                    return true;
                case "swipe_right_left":
                    requestVehicleAction("reader:vehicle:heading:right");
                    return true;
                default:
                    // Do not fall through to saved map or combat bindings
                    // while the avatar is driving. An unrecognized swipe
                    // must never activate another gameplay action.
                    return true;
            }
        }
        if (combatMode && directWorldGesturesEnabled()) {
            // Old three-finger movement is disabled during combat.
            // On the normal map, saved three-finger shortcuts still work.
            switch (gestureId) {
                case "swipe_3_up":
                case "swipe_3_down":
                case "swipe_3_left":
                case "swipe_3_right":
                    return true;
                default: break;
            }
            // Eight directional strokes use native gameplay. Gun swipes
            // fire; unarmed or melee swipes use the matching regular move
            // key (including the game's bump-to-attack and interactions).
            // Bent strokes retain the four diagonals.
            int direction = -1;
            switch (gestureId) {
                case "swipe_up": direction = 0; break;
                case "swipe_right": direction = 2; break;
                case "swipe_down": direction = 4; break;
                case "swipe_left": direction = 6; break;
                case "swipe_up_left": direction = 7; break;
                case "swipe_up_right": direction = 1; break;
                case "swipe_down_left": direction = 5; break;
                case "swipe_down_right": direction = 3; break;
                case "swipe_up_down":
                    requestCombatCommand("reader:combat:weapon-next");
                    return true;
                case "swipe_down_up":
                    requestCombatCommand("reader:combat:weapon-prev");
                    return true;
                case "swipe_left_right":
                    requestCombatCommand("reader:combat:reload");
                    return true;
                case "swipe_right_left":
                    readCombatWieldedWeapon();
                    return true;
                default:
                    break;
            }
            if (direction >= 0) {
                requestCombatCommand("reader:combat:attack:" + direction);
                return true;
            }
        }
        if (!directWorldGesturesEnabled() || ReaderGestureBindings.isReserved(gestureId)) {
            // A fixed control is never replaced by a persisted binding.
            return false;
        }
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
        } else if ("hub:settings".equals(target)) {
            executeShortcutTarget("action:open_game_settings");
            return;
        } else if ("hub:tts".equals(target)) {
            activity.onAccessibilityOpenTtsSettings();
            return;
        } else if ("hub:reader_setup".equals(target)) {
            executeShortcutTarget("action:open_game_settings");
            return;
        } else if ("hub:debug_log".equals(target)) {
            activity.onAccessibilityOpenDebugLog();
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
        if (combatMode && directWorldGesturesEnabled()) {
            // Two fingers now move cardinally using the ordinary
            // native walk action. They never fire a weapon.
            moveInCombat(dx, dy);
            return;
        }
        String id = dy < 0 ? "reader:scan:north" : dy > 0 ? "reader:scan:south"
                : dx < 0 ? "reader:scan:west" : "reader:scan:east";
        requestLiveWorldCommand(id);
    }

    @Override
    public void twoFingerTap(int count) {
        if (count == 1 && combatMode && directWorldGesturesEnabled()) {
            ReaderSnapshot.Node node = snapshot.find("reader:combat:enemies");
            String report = node == null ? activity.getString(R.string.a11yCombatNoEnemies)
                    : node.detail;
            if (report == null || report.isEmpty()) {
                report = activity.getString(R.string.a11yCombatNoEnemies);
            }
            feedback(report);
            return;
        }
        if (count == 1) { detail(); return; }
        if (count == 2) {
            // Game-wide notification recap. The current snapshot carries
            // recent messages only in the live world, so fall back to the
            // last world snapshot that published them. No new announcement
            // is created; this only re-reads the game's message history,
            // newest first, exactly as the world screen already did.
            ReaderSnapshot.Node recent = snapshot == null
                    ? null : snapshot.find("world:messages");
            String spoken = recent == null ? "" : recent.detail;
            if (spoken.isEmpty()) {
                spoken = lastRecentMessages;
            }
            if (spoken.isEmpty()) {
                spoken = activity.getString(R.string.a11yHubNoMessages);
            }
            if (usesTouchExploration()) {
                feedback(spoken);
            } else {
                speech.say("world-two-tap:" + count, spoken, ReaderSpeechQueue.LONG_TEXT);
            }
            return;
        }
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
    public void toggleCombatMode() {
        if (!directWorldGesturesEnabled()) { return; }
        if (snapshot.find("reader:vehicle:seat") != null) {
            // On board: Map -> Combat -> Drive -> Map. A seat alone is
            // sufficient to offer Drive for inspection; the native driver
            // gate still refuses any control from a passenger.
            int nextMode = combatMode ? 2 : vehicleMode ? 0 : 1;
            combatMode = nextMode == 1;
            vehicleMode = nextMode == 2;
            CataclysmDDA.nativeCancelPendingWorldSteps();
            clearPendingWorldDirection();
            root.cancelGesture();
            SessionDebugLog.event("VEHICLE_MODE", "selected=" + nextMode);
            feedback(activity.getString(nextMode == 0 ? R.string.a11yMapMode
                    : nextMode == 1 ? R.string.a11yCombatOn
                    : R.string.a11yDrivingMode));
            return;
        }
        vehicleMode = false;
        if (!combatMode && !snapshot.swipeMovementAllowed) {
            feedback(activity.getString(R.string.a11yCombatUnavailable));
            return;
        }
        combatMode = !combatMode;
        CataclysmDDA.nativeCancelPendingWorldSteps();
        clearPendingWorldDirection();
        root.cancelGesture();
        SessionDebugLog.event("COMBAT_MODE", "enabled=" + combatMode);
        feedback(activity.getString(combatMode ? R.string.a11yCombatOn : R.string.a11yCombatOff));
    }

    private void readCombatWieldedWeapon() {
        if (!combatMode || !directWorldGesturesEnabled()) { return; }
        ReaderSnapshot.Node node = snapshot.find("reader:combat:weapon-status");
        if (node == null || node.detail == null || node.detail.isEmpty()) {
            feedback(activity.getString(R.string.a11yCombatUnavailable));
            return;
        }
        SessionDebugLog.event("COMBAT_ACTION", "read_wielded_weapon");
        // Reading the current item and ammunition costs no game turn.
        feedback(node.detail);
    }

    private void requestCombatCommand(String id) {
        if (!combatMode || !directWorldGesturesEnabled()) { return; }
        if (!snapshot.swipeMovementAllowed) {
            feedback(activity.getString(R.string.a11yCombatUnavailable));
            return;
        }
        ReaderSnapshot.Node node = snapshot.find(id);
        if (node == null || !node.enabled) {
            feedback(activity.getString(R.string.a11yCombatUnavailable));
            return;
        }
        SessionDebugLog.event("COMBAT_ACTION", "node=" + id);
        requestLiveWorldCommand(id);
    }

    @Override
    public void characterDetails() {
        if (!directWorldGesturesEnabled()) { return; }
        ReaderSnapshot.Node document = snapshot.find("world:character_detail");
        String spoken = document == null ? "" : document.detail;
        if (spoken.isEmpty()) {
            feedback(activity.getString(R.string.a11yReaderWaiting));
        } else if (usesTouchExploration()) {
            feedback(spoken);
        } else {
            // Read the entire current snapshot without spending a game turn.
            speech.say("world-character-detail", spoken, ReaderSpeechQueue.LONG_TEXT);
        }
    }

    @Override
    public void openActionMenu() {
        // Two fingers, three taps: the approved 16-section action hub.
        showComprehensiveMenu();
    }

    private void clearPendingWorldDirection() {
        handler.removeCallbacks(pendingWorldRetryPoll);
        pendingWorldDirection = "";
        pendingWorldContext = "";
        pendingWorldRevision = 0;
        pendingWorldSince = 0;
    }

    private void retryPendingWorldDirection() {
        if (pendingWorldDirection.isEmpty()) { return; }
        final long elapsed = SystemClock.elapsedRealtime() - pendingWorldSince;
        if (snapshot == null || !pendingWorldContext.equals(snapshot.contextKey())
                || elapsed > 1200 || !worldStepAllowed() || closed
                || !foreground || !windowFocused || lastSuspended) {
            SessionDebugLog.event("WORLD_STEP_EXPIRED",
                    "reason=context_or_timeout elapsed_ms=" + elapsed);
            clearPendingWorldDirection();
            return;
        }
        // Publishing a snapshot does NOT mean the native game loop is ready.
        // Wait for both a new world frame and the authoritative input gate.
        if (snapshot.revision <= pendingWorldRevision) {
            schedulePendingWorldRetry();
            return;
        }
        final boolean ready;
        try {
            ready = CataclysmDDA.nativeWorldStepReady();
        } catch (UnsatisfiedLinkError unavailable) {
            clearPendingWorldDirection();
            return;
        }
        if (!ready) {
            schedulePendingWorldRetry();
            return;
        }
        String direction = pendingWorldDirection;
        String oldFocus = focus;
        focus = direction;
        retryingWorldDirection = true;
        try {
            if (send("activate", "") != 0) {
                SessionDebugLog.event("WORLD_STEP_RETRIED", "direction=" + direction);
                clearPendingWorldDirection();
            } else {
                // The game thread can close the gate after our ready probe.
                // Keep the same latest swipe for the next bounded attempt.
                schedulePendingWorldRetry();
            }
        } finally {
            retryingWorldDirection = false;
            focus = oldFocus;
        }
    }

    private void schedulePendingWorldRetry() {
        if (pendingWorldDirection.isEmpty()) { return; }
        handler.removeCallbacks(pendingWorldRetryPoll);
        handler.postDelayed(pendingWorldRetryPoll, 30);
    }

    @Override
    public void moveDirection(int dx, int dy) {
        SessionDebugLog.event("GESTURE_ACTION", "move dx=" + dx + " dy=" + dy
                + " enabled=" + worldMovementEnabled());
        if (!worldMovementEnabled()) { return; }
        dispatchWorldMovement(dx, dy);
    }

    private void moveInCombat(int dx, int dy) {
        boolean enabled = combatMode && worldStepAllowed();
        SessionDebugLog.event("GESTURE_ACTION", "combat_move dx=" + dx + " dy=" + dy
                + " enabled=" + enabled);
        if (!enabled) {
            feedback(activity.getString(R.string.a11yFastMovementUnavailable));
            return;
        }
        // Deliberately no movement distance, speed or step count override:
        // use the same direction command, native safety checks and retry
        // semantics as an ordinary world-map swipe.
        dispatchWorldMovement(dx, dy);
    }

    private void dispatchWorldMovement(int dx, int dy) {
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
        // World double tap becomes Pause/Resume only when Drive is active.
        if (vehicleMode && directWorldGesturesEnabled()) {
            requestVehicleAction("reader:vehicle:pause");
            return;
        }
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
        if (pocketStage != 0) {
            activatePocketPicker();
            return;
        }
        if (travelState() != 0) { activateTravel(); return; }
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
        // Inventory rows may expose read-only pocket details as tree children.
        // Double-tap must still select the item (or open its quantity picker),
        // not trap the reader inside a non-activatable pocket-status list.
        // Other expandable nodes keep their normal nested navigation.
        final boolean actionableItem = node.id.startsWith("item:")
                && node.actions.contains("activate");
        if (node.hasChildren && !actionableItem) {
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
        // A pickup pocket-content row reached by flat browsing is a real
        // selectable row, not a staged-picker draft: double-tap marks it
        // for pickup through the same atomic parent-pocket commit the
        // staged picker uses. A single unit toggles at once; a stack
        // opens the quantity editor, which commits to the parent pocket
        // on submit. The staged picker itself is untouched.
        if (isFlatPickupPocketEntry(node)) {
            activateFlatPickupPocketEntry(node);
            return;
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
        // Every native screen publishes the actions its nodes actually
        // support. A status/heading with only "focus" is informative, not
        // clickable: never send an invalid activate which the game would
        // report as "stale" ("Choose the item again"). This applies equally
        // to skill-category headings, inventory pocket states, tables,
        // journals, health readouts and all other screen adapters.
        if (!node.actions.contains("activate")) {
            feedback(nodeText(node, true));
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
     * Standard fields retain the draft/Apply contract. A checkbox exposing
     * a bounded integer set_value action without Apply is an explicit
     * immediate item-quantity picker: each chosen number commits the amount,
     * closes this panel, and returns to the original item list. The final
     * inventory CONFIRM remains a separate action.
     */
    private void openFormEditor(ReaderSnapshot.Node node) {
        final boolean quickQuantity = "checkbox".equals(node.role)
                && "int".equals(node.fieldType) && node.hasFieldRange
                && node.fieldMin == 0.0 && node.fieldMax > 1.0 && node.fieldStep == 1.0
                && node.actions.contains("set_value") && !node.actions.contains("apply");
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
                quickQuantity, new ReaderTextEditor.Listener() {
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
        feedback(activity.getString(quickQuantity
                ? R.string.a11yQuantityInstructions : R.string.a11yEditorInstructions));
    }

    private long send(String action, String value) {
        ReaderSnapshot owner = snapshot;
        // The closed check is explicit, not implied by the lifecycle flags:
        // after close() no intent of this controller may reach the game,
        // even if a stray lifecycle callback ran first (R03.4).
        if (closed || owner == null || !foreground || !windowFocused || lastSuspended) { return 0; }
        // Universal semantic action gate. Every active native adapter owns
        // the complete set of permitted actions for its current nodes. A
        // reader heading, status, table cell or disabled command must never
        // be sent as "activate" simply because Android received a tap.
        // Cancel is a screen action and deliberately has no node binding.
        if (!"cancel".equals(action)) {
            ReaderSnapshot.Node target = owner.find(focus);
            if (target == null || !target.enabled && !"focus".equals(action)
                    && !"read_detail".equals(action)
                    || target != null && !target.actions.contains(action)) {
                if (!"focus".equals(action) && target != null) {
                    feedback(nodeText(target, true));
                }
                return 0;
            }
        }
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
                final boolean walking = "activate".equals(action)
                        && "DEFAULTMODE".equals(owner.category)
                        && !comprehensiveMenu && ("UP".equals(focus) || "DOWN".equals(focus)
                        || "LEFT".equals(focus) || "RIGHT".equals(focus)
                        || "LEFTUP".equals(focus) || "LEFTDOWN".equals(focus)
                        || "RIGHTUP".equals(focus) || "RIGHTDOWN".equals(focus));
                if (walking) {
                    // Preserve at most the newest swipe for one bounded
                    // retry after the next native world snapshot. Never
                    // queue multiple steps, and never cross screen owners.
                    SessionDebugLog.event("WORLD_STEP_BUSY", "request=" + request);
                    final long now = SystemClock.elapsedRealtime();
                    if (!retryingWorldDirection && worldStepAllowed()) {
                        pendingWorldDirection = focus;
                        pendingWorldContext = owner.contextKey();
                        pendingWorldRevision = owner.revision;
                        pendingWorldSince = now;
                        SessionDebugLog.event("WORLD_STEP_DEFERRED", "request=" + request);
                        schedulePendingWorldRetry();
                    } else if (now - lastWorldBusyNotice >= 2500) {
                        lastWorldBusyNotice = now;
                        feedback(activity.getString(R.string.a11yWorldStepBusy));
                    }
                } else if (!"focus".equals(action)) {
                    feedback(activity.getString(R.string.a11yReaderStale));
                }
            } else {
                if ("activate".equals(action) && "DEFAULTMODE".equals(owner.category)
                        && isWorldDirection(focus)) {
                    if (!retryingWorldDirection && !pendingWorldDirection.isEmpty()) {
                        SessionDebugLog.event("WORLD_STEP_SUPERSEDED",
                                "direction=" + pendingWorldDirection + " by=" + focus);
                    }
                    clearPendingWorldDirection();
                } else if (!"focus".equals(action) && !retryingWorldDirection) {
                    clearPendingWorldDirection();
                }
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


    private int hubEmptyLabel() {
        if (hubSection == GameActionHub.FAVORITES) {
            return R.string.a11yHubFavoritesEmpty;
        }
        if (hubSection == 1) {
            return R.string.a11yHubNearbyNoActions;
        }
        // A search that matches nothing says so explicitly
        // instead of the generic "no available entries", so the player
        // knows to change the keyword. The string already exists in
        // values/a11y_reader.xml (EN) and values-vi (VI).
        if (hubSection == GameActionHub.SEARCH && !hubQuery.trim().isEmpty()) {
            return R.string.a11yHubNoMatches;
        }
        return R.string.a11yHubEmpty;
    }

    private List<GameActionHub.Entry> hubEntries() {
        List<GameActionHub.Entry> result =
                GameActionHub.entries(activity, snapshot, hubSection, hubQuery, favorites);
        // Hide combat-only mode switching outside the live combat controls.
        if (!combatMode) {
            result.removeIf(entry -> "reader:combat:aim-toggle".equals(entry.node));
        }
        return result;
    }

    private String hubActionLabel(int choice) {
        if (choice == 0) { return activity.getString(R.string.a11yHubGestureOption); }
        String target = hubActionItem == null ? null : hubActionItem.shortcutTarget();
        return activity.getString(target != null && favorites.contains(target)
                ? R.string.a11yHubRemoveFavorite : R.string.a11yHubAddFavorite);
    }

    private String hubSpoken(List<GameActionHub.Entry> entries, int selected) {
        if (entries.isEmpty() || selected < 0 || selected >= entries.size()) {
            return "";
        }
        return entries.get(selected).label + ". "
                + activity.getString(R.string.a11yControlButton) + ", "
                + activity.getString(R.string.a11yPosition, selected + 1, entries.size());
    }

    private String gestureChoiceLabel(int index) {
        if (index == ReaderGestureBindings.CUSTOM_IDS.length) {
            return activity.getString(R.string.a11yBindDefault);
        }
        if (index == ReaderGestureBindings.CUSTOM_IDS.length + 1
                || index < 0 || index >= ReaderGestureBindings.CUSTOM_IDS.length) {
            return activity.getString(R.string.a11yBindCancel);
        }
        return activity.getString(ReaderGestureBindings.CUSTOM_LABELS[index]);
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
                : binding ? ReaderGestureBindings.CUSTOM_IDS.length + 2 : entries.size();
        int selected = choosingHubAction ? hubActionChoice : binding ? gestureChoice : hubIndex;
        if (count == 0) {
            TextView empty = new TextView(activity);
            empty.setText(hubEmptyLabel());
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
        // The hub lives only in Java, so opening it would not otherwise
        // invalidate the native DEFAULTMODE token. Prevent older swipes
        // from moving the character after this explicit mode change.
        try {
            final int discarded = CataclysmDDA.nativeCancelPendingWorldSteps();
            if (discarded != 0) {
                SessionDebugLog.event("GESTURE_ACTION",
                        "hub_cancelled_pending_world_steps=" + discarded);
            }
        } catch (UnsatisfiedLinkError unavailable) {
            SessionDebugLog.event("ERROR", "JNI nativeCancelPendingWorldSteps " + unavailable);
        }
        clearPendingWorldDirection();
        comprehensiveMenu = true;
        sonar.setWorldActive(false);
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
        gestureChoice = existing == null ? 0 : ReaderGestureBindings.customIndexOf(existing);
        if (gestureChoice < 0) { gestureChoice = 0; }
        root.cancelGesture();
        render();
        feedback(activity.getString(R.string.a11yBindTitle, selected.label) + ". "
                + gestureChoiceLabel(gestureChoice) + ", "
                + activity.getString(R.string.a11yPosition,
                        gestureChoice + 1, ReaderGestureBindings.CUSTOM_IDS.length + 2));
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
        if ("hub:settings".equals(target)) {
            return activity.getString(R.string.a11yGameSettingsTitle);
        }
        if ("hub:tts".equals(target)) { return activity.getString(R.string.a11yHubTts); }
        if ("hub:reader_setup".equals(target)) {
            return activity.getString(R.string.a11yGameSettingsTitle);
        }
        if ("hub:debug_log".equals(target)) {
            return activity.getString(R.string.a11yDebugLogTitle);
        }
        return target;
    }

    private void applyGestureChoice() {
        if (!choosingGesture || bindingItem == null) { return; }
        GameActionHub.Entry entry = bindingItem;
        String target = entry.shortcutTarget();
        int chosen = gestureChoice;
        if (chosen == ReaderGestureBindings.CUSTOM_IDS.length + 1) {
            choosingGesture = false;
            bindingItem = null;
            root.cancelGesture();
            render();
            readCurrent();
            return;
        }
        String message;
        if (chosen == ReaderGestureBindings.CUSTOM_IDS.length) {
            gestureBindings.restoreDefault(target);
            message = activity.getString(R.string.a11yBindRestored, entry.label);
        } else if (chosen >= 0 && chosen < ReaderGestureBindings.CUSTOM_IDS.length) {
            ReaderGestureBindings.Assignment result = gestureBindings.assign(
                    ReaderGestureBindings.CUSTOM_IDS[chosen], target);
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
            case GameActionHub.OPEN_DEBUG_LOG:
                activity.onAccessibilityOpenDebugLog();
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
        if (snapshot != null && "surroundings".equals(snapshot.category)
                && !comprehensiveMenu && !commandPaletteOpen) {
            nearbyFirstItemPending = false;
        }
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
                feedback(activity.getString(label) + ". "
                        + activity.getString(R.string.a11yControlButton) + ", "
                        + activity.getString(R.string.a11yPosition, debugFocus, 3));
            }
            return;
        }
        if (editor != null) { editor.readCurrent(); return; }
        if (comprehensiveMenu) {
            if (choosingHubAction) {
                feedback(hubActionLabel(hubActionChoice) + ". "
                        + activity.getString(R.string.a11yControlButton) + ", "
                        + activity.getString(R.string.a11yPosition, hubActionChoice + 1, 2));
            } else if (choosingGesture) {
                feedback(gestureChoiceLabel(gestureChoice) + ". "
                        + activity.getString(R.string.a11yControlButton) + ", "
                        + activity.getString(R.string.a11yPosition, gestureChoice + 1,
                                ReaderGestureBindings.CUSTOM_IDS.length + 2));
            } else {
                List<GameActionHub.Entry> items = hubEntries();
                if (items.isEmpty()) {
                    feedback(activity.getString(hubEmptyLabel()));
                }
                else {
                    hubIndex = Math.max(0, Math.min(hubIndex, items.size() - 1));
                    feedback(hubSpoken(items, hubIndex));
                }
            }
            return;
        }
        if (pocketStage != 0) {
            feedback(pocketChoiceSpeech());
            return;
        }
        if (travelState() != 0) {
            feedback(travelSpoken(travelState()));
            return;
        }
        ReaderSnapshot.Node node = snapshot == null ? null : snapshot.find(focus);
        if (node == null) {
            if (snapshot == null) {
                feedback(lastLoadingStatus.isEmpty()
                        ? activity.getString(R.string.a11yReaderWaiting)
                        : activity.getString(R.string.a11yLoadingStatus, lastLoadingStatus));
            }
            return;
        }
        // This row's visible text is the actual full recipe detail.
        // Read it as long-form speech on ordinary focus, with no tap or
        // special two-finger action required.
        if (snapshot != null && "crafting".equals(snapshot.category)
                && "craft:detail:intro".equals(node.id)) {
            speakInlineDetail(node.detail.isEmpty() ? node.label : node.detail);
            return;
        }
        String spoken = nodeText(node, false);
        if (usesTouchExploration()) {
            announceAccessibility(spoken);
        } else {
            speech.focus(node.id, spoken);
        }
    }

    /**
     * A selected recipe, construction, mission or faction row often has its
     * original long description in a separate read-only document node.
     * Two-finger Detail reads those documents directly, without activating
     * the row or opening another modal. The native screen already updates
     * them for the current selection, so no second gameplay implementation
     * or speculative data is needed here.
     */
    private boolean readLinkedDetails(ReaderSnapshot.Node selected) {
        if (snapshot == null || selected == null) { return false; }
        String category = snapshot.category;
        String id = selected.id;
        String prefix = "";
        if ("crafting".equals(category) && id.startsWith("recipe:")) {
            prefix = "craft:";
        } else if ("construction".equals(category) && id.startsWith("construction:group:")) {
            prefix = "construction:";
        } else if ("missions".equals(category) && id.startsWith("missions:mission:")) {
            prefix = "missions:mission:details";
        } else if ("missions".equals(category) && id.startsWith("missions:poi:")) {
            prefix = "missions:poi:details";
        } else if ("faction".equals(category) && "listitem".equals(selected.role)
                && id.startsWith("faction:")) {
            prefix = "faction:details";
        } else if ("companion_missions".equals(category)
                && id.startsWith("companion:entry:")) {
            prefix = "companion:details";
        } else if ("proficiencies".equals(category)
                && id.startsWith("proficiency:entry:")) {
            prefix = "proficiency:details";
        } else if ("diary".equals(category) && id.startsWith("diary:page:")) {
            prefix = "diary:entry";
        }
        if (prefix.isEmpty()) { return false; }
        // These document nodes describe only the engine's CURRENT selection,
        // not all visible rows. A local Android swipe can arrive ahead of
        // native focus synchronization; defer until its own row is selected.
        if (!selected.selected || !id.equals(snapshot.focus)) {
            pendingLinkedDetail = id;
            pendingLinkedDetailContext = snapshot.contextKey();
            if (send("focus", "") == 0) {
                pendingLinkedDetail = "";
                pendingLinkedDetailContext = "";
                feedback(activity.getString(R.string.a11yReaderWaiting));
            }
            return true;
        }
        StringBuilder full = new StringBuilder();
        for (ReaderSnapshot.Node document : snapshot.nodes) {
            if (!"document".equals(document.role)
                    || !document.id.startsWith(prefix) || document.detail.isEmpty()) {
                continue;
            }
            if (full.length() > 0) { full.append("\n\n"); }
            full.append(document.label).append(".\n").append(document.detail);
        }
        if (full.length() == 0) { return false; }
        // Begin with the focused row identity and its concise state, then
        // speak all accompanying native detail documents in order.
        full.insert(0, nodeText(selected, true) + "\n\n");
        // Keep the selected row active. These facts belong to the current
        // row, not to a nested document screen that hijacks left/right swipes.
        speakInlineDetail(full.toString());
        return true;
    }

    @Override
    public void detail() {
        pendingLinkedDetail = "";
        pendingLinkedDetailContext = "";
        if (snapshot != null && "surroundings".equals(snapshot.category)
                && !comprehensiveMenu && !commandPaletteOpen) {
            nearbyFirstItemPending = false;
        }
        if (debugReport != null) {
            if (usesTouchExploration()) { announceAccessibility(debugReport); }
            else { speech.say("debug-detail", debugReport, ReaderSpeechQueue.LONG_TEXT); }
            return;
        }
        if (editor != null) { feedback(editor.valueDescription()); return; }
        if (comprehensiveMenu) {
            if (choosingGesture || choosingHubAction) {
                readCurrent();
            } else {
                // A two-finger Detail request reads the selected hub entry
                // in place. Its gesture/favorite action menu belongs only
                // to long press, not to the information gesture.
                List<GameActionHub.Entry> entries = hubEntries();
                if (entries.isEmpty()) {
                    feedback(activity.getString(hubEmptyLabel()));
                } else {
                    hubIndex = Math.max(0, Math.min(hubIndex, entries.size() - 1));
                    GameActionHub.Entry entry = entries.get(hubIndex);
                    StringBuilder details = new StringBuilder(hubSpoken(entries, hubIndex));
                    ReaderSnapshot.Node original = snapshot == null || entry.node.isEmpty()
                            ? null : snapshot.find(entry.node);
                    if (original != null && !original.detail.isEmpty()) {
                        details.append(". ").append(original.detail);
                    }
                    speakInlineDetail(details.toString());
                }
            }
            return;
        }
        if (snapshot == null && !lastLoadingStatus.isEmpty()) {
            feedback(activity.getString(R.string.a11yLoadingStatus, lastLoadingStatus));
            return;
        }
        if (travelState() != 0) {
            feedback(travelDetailSpeech(travelState()));
            return;
        }
        // The pocket chooser has synthetic controls that are not native
        // nodes. Speak the actual focused control, not the underlying
        // container item, while still allowing real pocket/item rows to
        // use their full native detail request.
        if (pocketStage == 1 || (pocketStage == 3 &&
                (POCKET_CONFIRM_NODE.equals(focus) || POCKET_CANCEL_NODE.equals(focus)))) {
            feedback(pocketChoiceSpeech());
            return;
        }
        ReaderSnapshot.Node node = snapshot == null ? null : snapshot.find(focus);
        if (node != null) {
            // Native item information is fetched only on explicit request.
            // This also works for read-only/disabled items and pocket rows.
            if (node.actions.contains("read_detail")) {
                pendingItemDetailNode = node.id;
                pendingItemDetailContext = snapshot.contextKey();
                pendingItemDetailParts = null;
                if (send("read_detail", "") == 0) {
                    pendingItemDetailNode = "";
                    pendingItemDetailContext = "";
                }
                return;
            }
            if (readLinkedDetails(node)) { return; }
            // Detail on a document node speaks its original content without
            // entering document-navigation mode. Explicit document-reader
            // commands elsewhere still provide paragraph navigation.
            if ("document".equals(node.role) && !node.detail.isEmpty()) {
                speakInlineDetail(node.detail);
                return;
            }
            String information = nodeText(node, true);
            // Double-tap on an inventory item is reserved for selecting it.
            // Keep its informational pocket nodes accessible through Detail,
            // rather than requiring a visit to a read-only child list.
            if (node.id.startsWith("item:") && node.hasChildren) {
                for (ReaderSnapshot.Node child : snapshot.nodes) {
                    if (node.id.equals(child.parentId) && child.id.startsWith("pocket:")) {
                        information += ". " + nodeText(child, true);
                    }
                }
            }
            // Yes/No choices carry the original question in node.detail,
            // so do not prepend snapshot.summary a second time.
            if (!node.detail.isEmpty() || information.length() > 320) {
                speakInlineDetail(information);
            } else {
                feedback(information);
            }
        }
    }

    private void announceAccessibility(String text) {
        SessionDebugLog.event("ACCESSIBILITY_ANNOUNCE", "requested=" + text);
        root.announceForAccessibility(text);
    }

    private void feedback(String text) {
        if (!foreground || closed) { return; }
        text = ReaderSpokenText.clean(text);
        if (text.isEmpty()) { return; }
        if (usesTouchExploration()) {
            status.setText(text);
            announceAccessibility(text);
        } else {
            speech.say("feedback", text, ReaderSpeechQueue.FOCUS);
        }
    }

    /**
     * Spoken, on-demand facts are not a separate screen. A single long
     * request is split into safe TTS chunks by ReaderSpeechQueue; the next
     * focus swipe interrupts it and navigates the original list normally.
     * Only explicit document-reading actions open ReaderDocumentReader.
     */
    private void speakInlineDetail(String text) {
        text = ReaderSpokenText.clean(text);
        if (text.isEmpty()) { return; }
        if (usesTouchExploration()) {
            feedback(text);
        } else {
            speech.say("inline-detail:" + ANNOUNCEMENT_SEQUENCE.incrementAndGet(),
                    text, ReaderSpeechQueue.LONG_TEXT, 600000L);
        }
    }

    /** Document speech stays at LONG_TEXT priority: warnings may cut it
     *  off, and the document reader then resumes at the same paragraph. */
    private void speakDocument(String text) {
        text = ReaderSpokenText.clean(text);
        if (text.isEmpty()) { return; }
        if (usesTouchExploration()) {
            feedback(text);
        } else {
            speech.say(documentReader.speechKey(), text, ReaderSpeechQueue.LONG_TEXT);
        }
    }

    @Override
    public void stopSpeech() {
        // A single tap is the reader's speech control. While
        // something is speaking (or still queued) it stops it, as before.
        // When the queue is idle, the same tap resumes the last utterance
        // from the chunk where it was cut off (ReaderSpeechQueue.resume(),
        // which falls back to repeating from the start when nothing was
        // cut off mid-utterance). The speaking/pending check comes from
        // the queue's own stats so a resume attempt can never discard
        // speech that is still waiting to be spoken.
        ReaderSpeechQueue.Stats stats = speech.snapshotStats();
        if (stats.speaking || stats.pending > 0) {
            speech.stop();
        } else {
            speech.resume();
        }
    }

    public void onBackPressed() {
        SessionDebugLog.event("ACTION", "back focus=" + focus
                + " hub=" + comprehensiveMenu);
        if (closed) { return; }
        if (debugReport != null) { submitDebugResponse(1); return; }
        if (editor != null) { editor.cancelInput(); return; }
        if (pocketStage != 0) {
            cancelPocketPicker(); // Back has the same rollback as Cancel.
            return;
        }
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
        if (travelState() != 0) {
            // Back must not silently abandon a route. Move focus to the
            // explicit Cancel button; only a subsequent activation ends it.
            travelChoice = travelChoiceCount(travelState()) - 1;
            render();
            readCurrent();
            return;
        }
        if (documentReader.isOpen()) {
            documentReader.clear();
            speech.stop();
            readCurrent();
            return;
        }
        if (backReaderLayer()) { return; }
        // On a creator step, two fingers down or Back returns to the
        // previous step. Nested value/variant lists are different contexts:
        // their Back is the ordinary cancel, returning to their parent tab.
        if (creatorStepAction("PREV_TAB")) { return; }
        // Summary is the seventh creator step. Its command snapshot uses
        // plain action IDs rather than action: prefixes; Back must return
        // to skills instead of invoking the global Quit confirmation.
        if (snapshot != null && "CHARACTER_CREATION".equals(snapshot.category)) {
            ReaderSnapshot.Node previousTab = snapshot.find("PREV_TAB");
            if (previousTab != null && previousTab.enabled
                    && previousTab.actions.contains("activate")) {
                String oldFocus = focus;
                focus = previousTab.id;
                send("activate", "");
                focus = oldFocus;
                return;
            }
        }
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
        sonar.setWorldActive(false);
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

    /** A fixed shortcut only outside the world map; TalkBack owns its gestures. */
    public boolean canToggleReaderOutsideWorld() {
        // Check screen identity rather than TTS readiness. A failed engine
        // must not turn the map's character-details gesture into a toggle.
        boolean worldMap = snapshot != null && snapshot.find("world:here") != null
                && snapshot.find("reader:coordinates") != null
                && !comprehensiveMenu && !commandPaletteOpen && editor == null
                && debugReport == null;
        return !closed && foreground && windowFocused && !usesTouchExploration()
                && (!preferences.isEnabled() || !worldMap);
    }

    public void toggleReaderOutsideWorld() {
        if (!canToggleReaderOutsideWorld() || readerOffAfterAnnouncement) { return; }
        if (!preferences.isEnabled()) {
            applyReaderGestureEnabled(true);
            return;
        }
        if (!speech.isReady()) {
            applyReaderGestureEnabled(false);
            return;
        }
        // Let the confirmation finish before shutting down its own TTS engine.
        speech.stop();
        readerOffAfterAnnouncement = true;
        speech.say(GESTURE_READER_OFF_KEY,
                activity.getString(R.string.a11yReaderDisabledNotice),
                ReaderSpeechQueue.FOCUS);
        handler.postDelayed(finishReaderOffGesture, 3000L);
    }

    private void applyReaderGestureEnabled(boolean enabled) {
        if (closed) { return; }
        handler.removeCallbacks(finishReaderOffGesture);
        readerOffAfterAnnouncement = false;
        announceReaderEnabledAfterInit = false;
        preferences.setEnabled(enabled);
        presentationEpoch++;
        root.cancelGesture();
        if (enabled) {
            root.setVisibility(View.VISIBLE);
            announceReaderEnabledAfterInit = true;
            if (!speech.isReady()) { speech.initialize(preferences); }
            render();
        } else {
            speech.clearForLifecycle();
            pendingGameMessages.clear();
            sonar.setWorldActive(false);
            root.setVisibility(View.GONE);
        }
        updateNativeState(true);
        if (enabled && speech.isReady() && foreground && windowFocused) {
            announceReaderEnabledAfterInit = false;
            speech.say("gesture-reader-on", activity.getString(R.string.a11yReaderGestureOn),
                    ReaderSpeechQueue.FOCUS);
        }
        SessionDebugLog.event("READER_GESTURE_TOGGLE", "enabled=" + enabled);
    }

    public boolean isEnabled() { return preferences.isEnabled(); }

    @Override
    public void close() {
        if (closed) { return; }
        handler.removeCallbacks(finishReaderOffGesture);
        readerOffAfterAnnouncement = false;
        combatMode = false;
        vehicleMode = false;
        pendingVehicleRoadReport = false;
        pendingVehicleSpeedReport = false;
        closed = true;
        foreground = false;
        presentationEpoch++;
        root.cancelGesture();
        documentReader.clear();
        pendingGameMessages.clear();
        cues.close();
        sonar.close();
        speech.close();
        if (accessibility != null) {
            accessibility.removeTouchExplorationStateChangeListener(explorationListener);
        }
        updateNativeState(true);
        handler.removeCallbacksAndMessages(null);
        if (root.getParent() instanceof ViewGroup) { ((ViewGroup) root.getParent()).removeView(root); }
    }
}

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


public final class GameReaderController implements AutoCloseable, ReaderGestureSurface.Listener {
    private static final AtomicLong REQUEST_SEQUENCE = new AtomicLong();
    private static final AtomicLong ANNOUNCEMENT_SEQUENCE = new AtomicLong();


    private final AtomicLong speechActionEpoch = new AtomicLong();


    private long lastAutodriveTerminalActionEpoch = -1;
    private static final int HUB_PAGE_SIZE = 30;
    private static final String GESTURE_GUIDE_NODE = "reader:gesture:guide";
    private static final String TARGET_TOGGLE_NODE = "reader:target:toggle-local";
    private static final String TRAVEL_STATUS_NODE = "reader:travel:status-local";
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


    private boolean announceTtsSavedAfterInit;
    private final Runnable finishReaderOffGesture = () -> {
        if (!readerOffAfterAnnouncement) { return; }
        readerOffAfterAnnouncement = false;
        applyReaderGestureEnabled(false);
    };
    private final LinkedHashMap<Long, Long> pendingIntentTimes = new LinkedHashMap<>();





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
    private static final String QUERY_PREFIX = "A11Y_QUERY:";
    private static final String DRIVE_FEEDBACK_PREFIX = "A11Y_DRIVE_FEEDBACK:";
    private static final String AUTODRIVE_END_PREFIX = "A11Y_AUTODRIVE_END:";
    private static final String AREA_PREFIX = "A11Y_AREA:";





    static boolean allowedDuringDriving(String message, int priority) {
        if (message == null) { return false; }
        return priority >= ReaderSpeechQueue.URGENT
                || message.startsWith(QUERY_PREFIX)
                || message.startsWith(DRIVE_FEEDBACK_PREFIX)
                || message.startsWith(AUTODRIVE_END_PREFIX)
                || message.startsWith(AREA_PREFIX)
                || message.startsWith("A11Y_SCAN:")
                || message.startsWith("A11Y_DETAIL:")
                || message.startsWith("A11Y_DETAIL_BEGIN:")
                || message.startsWith("A11Y_DETAIL_PART:")
                || message.startsWith("A11Y_DETAIL_END:")
                || message.startsWith(COMBAT_ALERT_PREFIX);
    }

    static boolean confirmedEngineTransition(String oldVehicle, String oldState,
            String vehicle, String state) {
        return oldVehicle != null && vehicle != null
                && !oldVehicle.isEmpty() && oldVehicle.equals(vehicle)
                && oldState != null && state != null
                && !oldState.isEmpty() && !state.isEmpty()
                && !oldState.equals(state);
    }



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



            BufferedGameMessage first = pendingGameMessages.peekFirst();
            if (first.text.startsWith(COMBAT_ALERT_PREFIX)
                    && (snapshot == null || !"DEFAULTMODE".equals(snapshot.category))) {
                return;
            }
            BufferedGameMessage buffered = pendingGameMessages.removeFirst();


            announceGameMessage(buffered.text, buffered.priority, true);
        }
    }
    private final AccessibilityManager.TouchExplorationStateChangeListener explorationListener;
    private ReaderSnapshot snapshot;


    private String lastLoadingStatus = "";



    private String lastRecentMessages = "";
    private final ReaderDocumentReader documentReader;
    private ReaderTextEditor editor;
    private String editorContext = "";
    private long editorRequest;
    private String formContext = "";
    private long formRequest;
    private String focus = "";


    private boolean nearbyFirstItemPending;
    private boolean craftingNavigationPending;


    private boolean comprehensiveMenu;


    private boolean activatingHubCommand;
    private int hubSection = GameActionHub.ROOT;
    private int hubIndex;
    private int hubRootIndex;
    private String hubQuery = "";
    private boolean choosingGesture;
    private GameActionHub.Entry bindingItem;
    private int gestureChoice;

    private boolean choosingHubAction;
    private GameActionHub.Entry hubActionItem;
    private int hubActionChoice;

    private String debugReport;
    private int debugFocus;
    private boolean debugResponsePending;




    private boolean commandPaletteOpen;

    private boolean gestureGuideOpen;
    private int gestureGuideIndex;
    private List<String> gestureGuideRows = new ArrayList<>();
    private int travelContextDrawerState;

    private boolean targetingOriginalControls;

    private boolean combatMode;
    private boolean vehicleMode;


    private boolean autonomousDriveTrip;
    private boolean autonomousDriveCancelRequested;

    private boolean vehicleReleasePending;
    private boolean pendingVehicleHeadingReport;
    private String vehicleHeadingBeforeCommand = "";
    private int pendingVehicleHeadingSnapshots;


    private String observedEngineVehicle = "";
    private String observedEngineState = "";
    private String commandPaletteReturnFocus = "";
    private String commandPaletteReturnParent = "";
    private String readerLayerParent = "";
    private String pendingReaderLayer = "";
    private int pendingReaderLayerRevisions;


    private String pendingLinkedDetail = "";
    private String pendingLinkedDetailContext = "";



    private String pendingItemDetailNode = "";
    private String pendingItemDetailContext = "";
    private StringBuilder pendingItemDetailParts;



    private int pocketStage;
    private int pocketMenuChoice;
    private String pocketContainerId = "";
    private String pocketCurrentId = "";
    private String pocketReturnParent = "";
    private String pocketContext = "";
    private final LinkedHashMap<String, Integer> pocketDrafts = new LinkedHashMap<>();


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



    private int travelChoice;
    private int announcedTravelState;




    private final Runnable travelStatePoll = new Runnable() {
        @Override
        public void run() {
            if (closed) { return; }
            if (foreground && windowFocused && preferences.isEnabled()
                    && !lastSuspended) {
                int state = travelState();
                if (state != announcedTravelState) {
                    announcedTravelState = state;
                    if (!autonomousDriveTrip && !vehicleMode) {
                        travelChoice = 0;
                        clearPendingWorldDirection();
                        root.cancelGesture();
                        render();
                    }
                    if (autonomousDriveTrip || vehicleMode) {





                    } else if (state != 0) {
                        feedback(travelStatus(state) + ". " + travelSpoken(state));
                    } else if (snapshot != null
                            && ("DEFAULTMODE".equals(snapshot.category)
                                || "activity".equals(snapshot.category))) {



                        readCurrent();
                    }
                }
            }
            handler.postDelayed(this, 200);
        }
    };




    private String pendingWorldDirection = "";
    private String pendingWorldContext = "";
    private long pendingWorldRevision;
    private long pendingWorldSince;
    private boolean retryingWorldDirection;



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



        cues = new ReaderCuePlayer(activity, preferences, () -> {
            if (!closed) {
                feedback(activity.getString(R.string.a11yCueUnavailable));
            }
        });
        sonar = new AlignmentSonarPlayer(activity);
        documentReader = new ReaderDocumentReader(activity);



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

        root.setBackgroundColor(android.graphics.Color.TRANSPARENT);
        root.setPadding(0, 0, 0, 0);
        status = new TextView(activity);

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
            if (announceTtsSavedAfterInit && preferences.isEnabled()) {
                announceTtsSavedAfterInit = false;
                speech.say("tts-settings-saved",
                        activity.getString(R.string.a11yReaderEnabledNotice),
                        ReaderSpeechQueue.FOCUS);
            } else if (announceReaderEnabledAfterInit && preferences.isEnabled()) {
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






    public void applySavedTtsSettings() {
        if (closed) { return; }
        final boolean enabled = preferences.isEnabled();
        announceReaderEnabledAfterInit = false;
        announceTtsSavedAfterInit = enabled && !usesTouchExploration();
        speech.clearForLifecycle();
        pendingGameMessages.clear();
        root.cancelGesture();
        root.setVisibility(enabled ? View.VISIBLE : View.GONE);
        if (enabled) {



            speech.initialize(preferences);
        } else {
            sonar.setWorldActive(false);
        }
        render();
        updateNativeState(true);
    }

    public void onPause() {
        ReaderDiagnostics.get().event("lifecycle_pause");
        announceTtsSavedAfterInit = false;
        foreground = false;
        sonar.setWorldActive(false);
        presentationEpoch++;
        root.cancelGesture();




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




            speech.clearForLifecycle();
            pendingGameMessages.clear();
        }
        updateNativeState(false);
    }

    private void updateNativeState(boolean force) {
        boolean enabled = !closed && preferences.isEnabled();
        sonar.setChannels(enabled && preferences.isCreatureSonarEnabled(),
                enabled && preferences.isItemSonarEnabled());
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


                    ReaderDiagnostics.get().count("receipt_" + code);





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
                    gestureGuideOpen = false;
                    autonomousDriveTrip = false;
                    autonomousDriveCancelRequested = false;
                    travelContextDrawerState = 0;
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



                final boolean nativeAutodrive =
                        ("activity".equals(next.category) || "DEFAULTMODE".equals(next.category))
                        && next.find("activity:autodrive") != null;
                if (nativeAutodrive && !autonomousDriveCancelRequested) {
                    autonomousDriveTrip = true;
                    vehicleMode = true;
                    combatMode = false;
                    vehicleReleasePending = false;
                } else if (!nativeAutodrive && autonomousDriveTrip
                        && !"activity".equals(next.category)
                        && rawNativeTravelState() == 0) {


                    autonomousDriveTrip = false;
                    autonomousDriveCancelRequested = false;
                }


                if ("DEFAULTMODE".equals(next.category)
                        && next.find("reader:vehicle:seat") == null && !autonomousDriveTrip) {
                    vehicleMode = false;
                    pendingVehicleHeadingReport = false;
                }
                if ("DEFAULTMODE".equals(next.category) &&
                        (next.find("reader:vehicle:seat") == null ||
                         next.find("reader:vehicle:driver") == null ||
                         !next.find("reader:vehicle:driver").enabled)) {
                    boolean finishedVehicleRelease = vehicleReleasePending;
                    vehicleReleasePending = false;
                    if (finishedVehicleRelease && !vehicleMode && !combatMode) {
                        handler.post(() -> {
                            if (!vehicleReleasePending && !vehicleMode && !combatMode
                                    && directWorldGesturesEnabled()) {
                                feedback(activity.getString(R.string.a11yMapMode));
                            }
                        });
                    }
                }



                if ((vehicleMode || autonomousDriveTravelActive())
                        && ("DEFAULTMODE".equals(next.category)
                            || "activity".equals(next.category))) {
                    ReaderSnapshot.Node vehicleSeat = next.find("reader:vehicle:seat");
                    ReaderSnapshot.Node engineState = next.find("reader:vehicle:engine-state");
                    if (vehicleSeat != null && engineState != null) {
                        if (confirmedEngineTransition(observedEngineVehicle,
                                observedEngineState, vehicleSeat.value, engineState.value)
                                && !engineState.detail.trim().isEmpty()) {
                            announceGameMessage(DRIVE_FEEDBACK_PREFIX + engineState.detail, 1, true);
                        }
                        observedEngineVehicle = vehicleSeat.value;
                        observedEngineState = engineState.value;
                    }
                } else if ("DEFAULTMODE".equals(next.category)) {
                    observedEngineVehicle = "";
                    observedEngineState = "";
                }
                if (pendingVehicleHeadingReport && "DEFAULTMODE".equals(next.category)) {
                    ReaderSnapshot.Node heading = next.find("reader:vehicle:heading");



                    String actualCompass = heading == null ? ""
                            : vehicleCompass(heading.detail);
                    if (!actualCompass.isEmpty()
                            && !actualCompass.equals(vehicleHeadingBeforeCommand)) {
                        pendingVehicleHeadingReport = false;
                        vehicleHeadingBeforeCommand = "";
                        final String report = heading.detail;
                        handler.post(() -> {
                            if (vehicleMode && directWorldGesturesEnabled()) {
                                feedback(report);
                            }
                        });
                    } else if (++pendingVehicleHeadingSnapshots >= 8) {


                        pendingVehicleHeadingReport = false;
                        vehicleHeadingBeforeCommand = "";
                    }
                }




                if (object.has("recentMessages")) {
                    lastRecentMessages = object.optString("recentMessages");
                } else {

                    ReaderSnapshot.Node recentMessages = next.find("world:messages");
                    if (recentMessages != null) {
                        lastRecentMessages = recentMessages.detail;
                    }
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
                    gestureGuideOpen = false;
                    commandPaletteOpen = false;
                    travelContextDrawerState = 0;
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
                if (commandPaletteOpen && !changedContext &&
                        (next.find(focus) != null || GESTURE_GUIDE_NODE.equals(focus)
                         || TARGET_TOGGLE_NODE.equals(focus)
                         || TRAVEL_STATUS_NODE.equals(focus))) {

                    readerLayerParent = "";
                } else if (nativeLayerChanged) {
                    focus = next.find(next.focus) != null ? next.focus
                            : next.nodes.get(0).id;
                } else if (selectionChanged) {
                    focus = selectedItemFocus;
                } else if (craftingNavigationCompleted) {


                    focus = next.find(next.focus) != null ? next.focus
                            : next.nodes.isEmpty() ? "" : next.nodes.get(0).id;
                    craftingNavigationPending = false;
                } else if (nearbyCategoryChanged) {


                    focus = next.find(next.focus) != null ? next.focus
                            : next.nodes.isEmpty() ? "" : next.nodes.get(0).id;
                } else if (changedContext) {
                    String remembered = savedScreenFocus.get(nativeScreenKey(next));
                    focus = remembered != null && next.find(remembered) != null
                            ? remembered : next.find(next.focus) != null ? next.focus
                            : next.nodes.isEmpty() ? "" : next.nodes.get(0).id;
                } else if (next.find(focus) == null && !gestureGuideOpen
                        && !(commandPaletteOpen && (GESTURE_GUIDE_NODE.equals(focus)
                                || TARGET_TOGGLE_NODE.equals(focus)
                                || TRAVEL_STATUS_NODE.equals(focus)))
                        && !(pocketStage == 3 && (POCKET_CONFIRM_NODE.equals(focus)
                                || POCKET_CANCEL_NODE.equals(focus)))) {



                    focus = "surroundings".equals(next.category)
                            && next.find(next.focus) != null ? next.focus
                            : resolveFocusFallback(previousSnapshot, previous, next);
                }


                ReaderSnapshot.Node focusedNode = next.find(focus);
                if (!commandPaletteOpen && focusedNode != null
                        && isAuxiliaryCommand(focusedNode)




                        && !(isMinigameCategory(next.category)
                                && isDirectionCommand(focusedNode))) {
                    List<ReaderSnapshot.Node> primary = visibleNodes();
                    if (!primary.isEmpty() && !isAuxiliaryCommand(primary.get(0))) {
                        focus = primary.get(0).id;
                    }
                }



                if ("targeting".equals(next.category)
                        && next.find("target:fire-direction:0") != null
                        && !targetingOriginalControls
                        && !focus.startsWith("target:fire-direction:")) {
                    focus = "target:fire-direction:0";
                }


                if ("player_display".equals(next.category) && !commandPaletteOpen) {
                    List<ReaderSnapshot.Node> readable = visibleNodes();
                    if (!readable.isEmpty()) {
                        boolean focusVisible = false;
                        for (ReaderSnapshot.Node candidate : readable) {
                            if (candidate.id.equals(focus)) {
                                focusVisible = true;
                                break;
                            }
                        }
                        if (!focusVisible) { focus = readable.get(0).id; }
                    }
                }
                ReaderSnapshot.Node currentFocus = next.find(focus);
                if (currentFocus != null && !currentFocus.parentId.equals(readerLayerParent)) {





                    if (!isFlattenedPocketEntry(currentFocus)) {
                        readerLayerParent = currentFocus.parentId;
                    }
                }
                if (changedContext) {





                    root.cancelGesture();
                    editor = null;
                    editorRequest = 0;
                    formRequest = 0;


                    documentReader.clear();
                }




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




                ReaderSnapshot.Node previousPosition = previousSnapshot == null ? null
                        : previousSnapshot.find("world:tile_position");
                ReaderSnapshot.Node nextPosition = next.find("world:tile_position");
                boolean steppedToAnotherTile = previousPosition != null && nextPosition != null
                        && !previousPosition.value.equals(nextPosition.value);




                boolean newlyNoteworthy = worldView && !vehicleMode && previousTile != null
                        && "alert".equals(nextTile.role)
                        && (steppedToAnotherTile || !"alert".equals(previousTile.role)
                            || !nextTile.value.equals(previousTile.value));
                int activeTravel = travelState();
                if (activeTravel != 0 && !autonomousDriveTrip && !vehicleMode) {
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



                announcedTravelState = activeTravel;
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


                    ReaderSnapshot.Node document = next.find(focus);
                    speakDocument(next.title + ". " + documentReader.open(
                            next.contextKey(), document.id, document.detail));
                } else if ("crafting".equals(next.category) && craftingNavigationCompleted
                        && next.find(focus) != null) {
                    feedback(next.summary.trim() + ". " + nodeText(next.find(focus), false));
                } else if ("surroundings".equals(next.category)
                        && (enteredMenu || nearbyCategoryChanged)) {



                    feedback((enteredMenu ? next.title + ". " : "") + next.summary);
                } else if (enteredMenu && "player_display".equals(next.category)
                        && next.find(focus) != null) {

                    readCurrent();
                } else if (enteredMenu && next.find(focus) != null) {




                    boolean yesNo = "YESNO".equals(next.category)
                            || "YESNOQUIT".equals(next.category);
                    String introduction = yesNo ? next.summary.trim() : next.title;


                    if (next.category.startsWith("character-creator-step-")
                            || "CHARACTER_CREATION".equals(next.category)) {











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





                    feedback(next.summary.trim());
                } else if (!usesTouchExploration() && (changedContext || !previous.equals(focus))) {
                    readCurrent();
                }



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
        final boolean userQuery = message.startsWith(QUERY_PREFIX);
        final boolean drivingFeedback = message.startsWith(DRIVE_FEEDBACK_PREFIX);
        final boolean autodriveEnd = message.startsWith(AUTODRIVE_END_PREFIX);
        final boolean areaChange = message.startsWith(AREA_PREFIX);
        final String spokenMessage = ReaderSpokenText.clean(
                loading ? message.substring("A11Y_LOADING:".length())
                : extendedScan ? message.substring("A11Y_SCAN:".length())
                : requestedDetail ? message.substring("A11Y_DETAIL:".length())
                : userQuery ? message.substring(QUERY_PREFIX.length())
                : drivingFeedback ? message.substring(DRIVE_FEEDBACK_PREFIX.length())
                : autodriveEnd ? message.substring(AUTODRIVE_END_PREFIX.length())
                : areaChange ? message.substring(AREA_PREFIX.length())
                : combatAlert ? message.substring(COMBAT_ALERT_PREFIX.length()) : message);



        if ("Đang tạo khu vực bản đồ mới. Vui lòng chờ.".equals(spokenMessage)
                || "Generating a new map area. Please wait.".equals(spokenMessage)) {
            SessionDebugLog.event("MAPGEN_SPEECH_SUPPRESSED", "movement_mapgen_notice");
            return;
        }
        final int level = priority >= ReaderSpeechQueue.URGENT
                ? ReaderSpeechQueue.URGENT : ReaderSpeechQueue.INFORMATION;


        final long receivedActionEpoch = speechActionEpoch.get();
        Runnable deliver = () -> {
            if (receivedActionEpoch != speechActionEpoch.get()) {
                SessionDebugLog.event("GAME_MESSAGE_STALE", "superseded_by_user_action");
                return;
            }
            if (closed || !foreground || !windowFocused || !preferences.isEnabled()) {
                return;
            }




            if (snapshot != null &&
                    ((vehicleMode && "DEFAULTMODE".equals(snapshot.category))
                     || (travelState() != 0 && "activity".equals(snapshot.category)))
                    && !allowedDuringDriving(message, priority)) {
                SessionDebugLog.event("DRIVE_SPEECH_SUPPRESSED", "ordinary_game_chatter");
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
            final boolean autodriveStopped = spokenMessage.contains("No safe route found. Vehicle paused.")
                    || spokenMessage.contains("Không tìm được đường đi an toàn. Xe đã tạm dừng.");
            final boolean autodriveArrived = drivingFeedback
                    && (spokenMessage.contains("You have reached your destination.")
                        || spokenMessage.contains("Bạn đã đến đích."));
            if (autodriveEnd || autodriveStopped || autodriveArrived) {
                lastAutodriveTerminalActionEpoch = receivedActionEpoch;
                speech.discardStaleAutodriveProgress();
                pendingGameMessages.removeIf(old -> ReaderSpeechQueue.isAutodriveProgressText(old.text));
                SessionDebugLog.event("AUTODRIVE_SPEECH",
                        "discarded_stale_progress terminal=" + (autodriveEnd ? "native" : "legacy"));
            } else if (ReaderSpeechQueue.isAutodriveProgressText(spokenMessage)
                    && lastAutodriveTerminalActionEpoch == receivedActionEpoch) {
                SessionDebugLog.event("AUTODRIVE_SPEECH", "suppressed_late_progress");
                return;
            }
            if (!loading && (snapshot == null
                    || (combatAlert && !"DEFAULTMODE".equals(snapshot.category)))) {






                pendingGameMessages.addLast(new BufferedGameMessage(message, priority));
                SessionDebugLog.event("GAME_MESSAGE_BUFFERED", "text=" + message);
                return;
            }
            if (loading) {


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
            } else if (userQuery) {


                speech.focus("user-requested-location", spokenMessage);
            } else if (areaChange) {


                speech.say("driving-named-area", spokenMessage,
                        ReaderSpeechQueue.INFORMATION, 6000L);
            } else if (requestedDetail) {




                speakInlineDetail(spokenMessage);
            } else if (extendedScan) {


                speech.say("world-scan:" + ANNOUNCEMENT_SEQUENCE.incrementAndGet(),
                        spokenMessage, ReaderSpeechQueue.LONG_TEXT);
            } else {




                final int messageLevel = level == ReaderSpeechQueue.URGENT
                        ? ReaderSpeechQueue.URGENT : ReaderSpeechQueue.GAME_EVENT;
                SessionDebugLog.event("GAME_MESSAGE_POLICY", "priority=" + messageLevel
                        + " text=" + message);
                speech.say("game-message:" + ANNOUNCEMENT_SEQUENCE.incrementAndGet(), spokenMessage,
                        messageLevel, 15000);
            }
        };
        if (immediate) {



            deliver.run();
        } else {
            handler.post(deliver);
        }
    }


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


    private void playRejectedCue() {
        ReaderDiagnostics.get().count("cue_rejected");
        cues.play(ReaderCuePlayer.REJECTED);
    }







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

        if (!node.value.isEmpty() && !node.value.equals(node.label)) {
            text.append(". ").append(node.value);
        }


        boolean quantityRow = snapshot != null && "inventory".equals(snapshot.category)
                && "checkbox".equals(node.role) && !node.fieldType.isEmpty();
        if (!quantityRow) {
            if (node.checked) { text.append(". ").append(activity.getString(R.string.a11yChecked)); }
            else if ("checkbox".equals(node.role)) {
                text.append(". ").append(activity.getString(R.string.a11yUnchecked));
            }
        }


        if (includeDetail && node.selected) {
            text.append(". ").append(activity.getString(R.string.a11ySelected));
        }



        int spokenPosition = node.position;
        int spokenTotal = node.total;






        boolean minigamePositions = snapshot != null
                && isMinigameCategory(snapshot.category);
        if (minigamePositions) {
            spokenPosition = 0;
            spokenTotal = 0;
        }


        boolean inventoryPositions = flattenedInventoryBrowse();
        boolean characterDetailPositions = snapshot != null
                && "player_display".equals(snapshot.category);
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
        } else if ((spokenPosition <= 0 || spokenTotal <= 0
                || inventoryPositions || characterDetailPositions)
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


        if (node.depth >= 0 && node.hasChildren) {
            text.append(". ").append(activity.getString(node.expanded
                    ? R.string.a11yTreeExpanded : R.string.a11yTreeCollapsed));
        }
        if (includeDetail && node.depth >= 0) {
            text.append(". ").append(activity.getString(R.string.a11yTreeLevel, node.depth + 1));
        }


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



    private static boolean isMinigameCategory(String category) {
        return "robot-finds-kitten".equals(category) || "snake".equals(category)
                || "sokoban".equals(category) || "lightson".equals(category);
    }










    private static boolean readsSummaryOnChange(String category) {
        return isMinigameCategory(category)
                || "advanced-inventory".equals(category)
                || "armor-layers".equals(category);
    }


    private static boolean isAuxiliaryCommand(ReaderSnapshot.Node node) {
        return "command".equals(node.role) || node.id.startsWith("group:")
                || node.id.startsWith("action:") || node.id.startsWith("category:")
                || "filter".equals(node.id) || "filter:clear".equals(node.id);
    }








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
                    : pocketMenuChoice == 1
                    ? activity.getString(R.string.a11yPocketOtherActions)
                    : activity.getString(R.string.a11yGestureGuideTitle);
            return name + ". " + activity.getString(R.string.a11yControlButton) + ", "
                    + activity.getString(R.string.a11yPosition, pocketMenuChoice + 1, 3);
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
            } else if (pocketMenuChoice == 1) {
                resetPocketPicker(false);
                openGlobalContextActions();
            } else {
                openGestureGuide();
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
            pocketMenuChoice = Math.floorMod(pocketMenuChoice + delta, 3);
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
            int[] ids = { R.string.a11yPocketOpen, R.string.a11yPocketOtherActions,
                          R.string.a11yGestureGuideTitle };
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
        if (pocketStage != 1) {
            renderGuideButton();
        }
    }

    private void openGlobalContextActions() {
        if (snapshot == null || gestureGuideOpen) { return; }


        List<ReaderSnapshot.Node> commands = contextualCommands();
        if (directWorldGesturesEnabled()) {
            CataclysmDDA.nativeCancelPendingWorldSteps();
            clearPendingWorldDirection();
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
            if (travelContextDrawerState != 0) {
                result.add(ReaderSnapshot.Node.localCommand(TRAVEL_STATUS_NODE,
                        activity.getString(R.string.a11yGuideTravelStatus)));
            } else {
                for (ReaderSnapshot.Node node : snapshot.nodes) {
                    if (node.parentId.isEmpty() && isAuxiliaryCommand(node)) {
                        result.add(node);
                    }
                }
            }
            if ("targeting".equals(snapshot.category) &&
                    snapshot.find("target:fire-direction:0") != null) {


                result.add(ReaderSnapshot.Node.localCommand(TARGET_TOGGLE_NODE,
                        activity.getString(targetingOriginalControls
                                ? R.string.a11yTargetDirections
                                : R.string.a11yTargetOriginalControls)));
            }


            result.add(ReaderSnapshot.Node.gestureGuide(
                    activity.getString(R.string.a11yGestureGuideTitle)));
        }
        return result;
    }



    private boolean flattenedInventoryBrowse() {
        return snapshot != null && "inventory".equals(snapshot.category)
                && pocketStage == 0 && readerLayerParent.isEmpty() && !commandPaletteOpen;
    }



    private boolean isFlattenedPocketEntry(ReaderSnapshot.Node node) {
        return node != null && node.id.startsWith("pocket-entry:")
                && (!"checkbox".equals(node.role) || pickupPocketParent(node) != null)
                && flattenedInventoryBrowse();
    }






    private ReaderSnapshot.Node pickupPocketParent(ReaderSnapshot.Node node) {
        if (snapshot == null || node == null || !node.id.startsWith("pocket-entry:")) {
            return null;
        }
        ReaderSnapshot.Node parent = snapshot.find(node.parentId);
        return parent != null && parent.id.startsWith("pocket:")
                && parent.actions.contains("set_value") ? parent : null;
    }






    private boolean isFlatPickupPocketEntry(ReaderSnapshot.Node node) {
        return node != null && "checkbox".equals(node.role)
                && pickupPocketParent(node) != null && flattenedInventoryBrowse();
    }








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


    private ReaderSnapshot.Node inventoryRootItem(ReaderSnapshot.Node node) {
        if (snapshot == null) { return null; }
        for (int steps = 0; node != null && !node.parentId.isEmpty()
                && steps < snapshot.nodes.size(); steps++) {
            node = snapshot.find(node.parentId);
        }
        return node != null && node.parentId.isEmpty()
                && node.id.startsWith("item:") ? node : null;
    }




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

            if (node.id.startsWith("reader:nearby:available:")
                    || "activity:autodrive".equals(node.id)) { continue; }
            if (readerLayerParent.equals(node.parentId)) { result.add(node); }
        }
        if ("targeting".equals(snapshot.category)
                && snapshot.find("target:fire-direction:0") != null) {
            if (targetingOriginalControls) { return result; }
            result.removeIf(node -> !node.id.startsWith("target:fire-direction:")
                    && !"target:auto-reload".equals(node.id));
            return result;
        }




        if ("look-around".equals(snapshot.category)) { return result; }


        boolean containsContent = false;
        for (ReaderSnapshot.Node node : result) {
            if (!isAuxiliaryCommand(node)) { containsContent = true; break; }
        }
        if (containsContent) {





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



            result.clear();
        }
        if ("player_display".equals(snapshot.category)) {




            result.removeIf(node -> "tab".equals(node.role)
                    || node.id.startsWith("pd:skill:category:")
                    || ("status".equals(node.role)
                        && node.value.trim().isEmpty()
                        && node.detail.trim().isEmpty()
                        && node.cells.isEmpty()
                        && !node.actions.contains("activate")));
        }
        return result;
    }

    @Override
    public void openContextActions() {
        if (closed) { return; }
        if (gestureGuideOpen) { closeGestureGuide(); return; }
        if (debugReport != null || editor != null || snapshot == null) {


            openGestureGuide();
            return;
        }
        if (pocketStage != 0) {

            openGestureGuide();
            return;
        }
        if (commandPaletteOpen) { closeContextActions(); return; }
        if (comprehensiveMenu) {
            if (choosingGesture || choosingHubAction) {

                openGestureGuide();
            } else {
                showHubActionChoices();
            }
            return;
        }
        if (walkingTravelPanelActive() && travelState() == 4) {
            showComprehensiveMenu();
            return;
        }
        if (autonomousDriveTravelActive()) {


            if (snapshot.find("control_vehicle") != null) {
                requestAutonomousRead("control_vehicle");
            } else {
                openGlobalContextActions();
            }
            return;
        }
        if (vehicleMode && directWorldGesturesEnabled()) {


            if (!vehicleDriverReady()) {


                openGlobalContextActions();
                return;
            }
            ReaderSnapshot.Node command = snapshot.find("control_vehicle");
            if (command == null || !command.enabled
                    || !command.actions.contains("activate")) {
                openGlobalContextActions();
                return;
            }
            requestLiveWorldCommand("control_vehicle");
            return;
        }
        if (combatMode && directWorldGesturesEnabled()) {


            openHubSection(GameActionHub.COMBAT);
            return;
        }
        if (snapshot != null && "targeting".equals(snapshot.category)
                && snapshot.find("target:fire-direction:0") != null) {


            openGlobalContextActions();
            return;
        }
        if (walkingTravelPanelActive()) {
            travelContextDrawerState = travelState();
            openGlobalContextActions();
            return;
        }




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
        travelContextDrawerState = 0;
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

    private void openGestureGuide() {




        boolean guideForCombat = combatMode && comprehensiveMenu
                && hubSection == GameActionHub.COMBAT && !choosingGesture
                && !choosingHubAction && snapshot != null
                && "DEFAULTMODE".equals(snapshot.category);
        gestureGuideRows = ReaderGestureGuide.entries(activity, snapshot,
                vehicleMode, combatMode, comprehensiveMenu && !guideForCombat,
                travelContextDrawerState != 0 || walkingTravelPanelActive(),
                editor != null, debugReport != null, gestureBindings,
                documentReader.isOpen(), pocketStage);
        if (gestureGuideRows.isEmpty()) { return; }
        gestureGuideIndex = 0;
        gestureGuideOpen = true;
        root.cancelGesture();
        render();
        feedback(activity.getString(R.string.a11yGestureGuideTitle) + ". "
                + gestureGuideSpeech());
    }

    private String gestureGuideSpeech() {
        if (gestureGuideRows.isEmpty()) { return ""; }
        int index = Math.max(0, Math.min(gestureGuideIndex, gestureGuideRows.size() - 1));
        return gestureGuideRows.get(index) + ". " + activity.getString(
                R.string.a11yPosition, index + 1, gestureGuideRows.size());
    }

    private void closeGestureGuide() {
        if (!gestureGuideOpen) { return; }
        gestureGuideOpen = false;
        gestureGuideRows.clear();
        root.cancelGesture();
        render();
        readCurrent();
    }

    private void renderGuideButton() {


        Button guide = new Button(activity);
        guide.setText(R.string.a11yGestureGuideTitle);
        guide.setOnClickListener(view -> openGestureGuide());
        rows.addView(guide);
    }

    private void renderGestureGuide() {
        TextView heading = new TextView(activity);
        heading.setText(R.string.a11yGestureGuideTitle);
        heading.setTextSize(20);
        heading.setTextColor(0xffeeeeee);
        heading.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
        rows.addView(heading);
        Button back = new Button(activity);
        back.setText(R.string.a11yContextCommandsClose);
        back.setOnClickListener(view -> closeGestureGuide());
        rows.addView(back);
        if (gestureGuideRows.isEmpty()) { return; }
        final int count = gestureGuideRows.size();
        final int start = (gestureGuideIndex / HUB_PAGE_SIZE) * HUB_PAGE_SIZE;
        final int end = Math.min(start + HUB_PAGE_SIZE, count);
        if (start > 0) {
            Button previous = new Button(activity);
            previous.setText(R.string.a11yPreviousPage);
            previous.setOnClickListener(view -> {
                gestureGuideIndex = start - 1;
                render();
                readCurrent();
            });
            rows.addView(previous);
        }
        for (int i = start; i < end; i++) {
            final int index = i;
            Button row = new Button(activity);
            row.setText(gestureGuideRows.get(i) + ". " +
                    activity.getString(R.string.a11yPosition, i + 1, count));
            row.setSelected(i == gestureGuideIndex);
            row.setOnClickListener(view -> {
                if (!gestureGuideOpen) { return; }
                gestureGuideIndex = index;

                readCurrent();
            });
            rows.addView(row);
        }
        if (end < count) {
            Button next = new Button(activity);
            next.setText(R.string.a11yNextPage);
            next.setOnClickListener(view -> {
                gestureGuideIndex = end;
                render();
                readCurrent();
            });
            rows.addView(next);
        }
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
                if (!commandPaletteOpen || snapshot == null ||
                        (!GESTURE_GUIDE_NODE.equals(command.id)
                         && !TARGET_TOGGLE_NODE.equals(command.id)
                         && !TRAVEL_STATUS_NODE.equals(command.id)
                         && snapshot.find(command.id) == null)) {
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

    private int rawNativeTravelState() {
        try {
            return CataclysmDDA.nativeTravelState();
        } catch (UnsatisfiedLinkError unavailable) {
            return 0;
        }
    }

    private boolean walkingTravelPanelActive() {

        return !autonomousDriveTrip && !vehicleMode && travelState() != 0;
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
        return rawNativeTravelState();
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


        return index == 0 ? (state == 2 || state == 4
                ? R.string.a11yTravelResume : R.string.a11yTravelPause)
                : R.string.a11yTravelCancel;
    }

    private String travelStatus(int state) {
        return activity.getString(state == 3 ? R.string.a11yTravelSafetyStopped
                : state == 4 ? R.string.a11yTravelObstacleStopped
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
                : travelChoice == 0 ? (state == 2 || state == 4 ? 2 : 1) : 3;
        try {
            if (CataclysmDDA.nativeTravelControl(command)) {
                SessionDebugLog.event("TRAVEL_CONTROL",
                        "command=" + command + " from_state=" + state);
                if (command == 3 && autonomousDriveTrip) {



                    autonomousDriveCancelRequested = true;
                    vehicleMode = false;
                    combatMode = false;
                    clearPendingWorldDirection();
                    root.cancelGesture();
                }
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
        renderGuideButton();
    }

    private void render() {
        if (closed) { return; }
        rows.removeAllViews();
        if (gestureGuideOpen) {
            root.setBackgroundColor(0xff161616);
            renderGestureGuide();
            return;
        }
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
            renderGuideButton();
            return;
        }

        if (editor != null) {
            root.setBackgroundColor(0xff161616);
            rows.addView(editor.getView());
            renderGuideButton();
        } else if (comprehensiveMenu) {
            root.setBackgroundColor(0xff161616);
            renderHub();
        } else if (pocketStage != 0) {
            root.setBackgroundColor(0xff161616);
            renderPocketPicker();
        } else if (commandPaletteOpen) {
            root.setBackgroundColor(0xff161616);
            renderContextCommands();
        } else if (walkingTravelPanelActive()
                   && snapshot != null && !"overmap".equals(snapshot.category)
                   && !"look-around".equals(snapshot.category)) {


            root.setBackgroundColor(0xff161616);
            renderTravelPanel(travelState());
        } else {
            root.setBackgroundColor(android.graphics.Color.TRANSPARENT);


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
    public void onUserAction() {
        if (closed || !foreground || !windowFocused || !preferences.isEnabled()) {
            return;
        }
        long epoch = speechActionEpoch.incrementAndGet();
        pendingGameMessages.clear();
        pendingLinkedDetail = "";
        pendingLinkedDetailContext = "";
        pendingItemDetailNode = "";
        pendingItemDetailContext = "";
        pendingItemDetailParts = null;
        speech.interruptForUserAction();
        SessionDebugLog.event("TTS_USER_ACTION_INTERRUPT", "epoch=" + epoch);
    }

    @Override
    public boolean usesTouchExploration() {
        return accessibility != null && accessibility.isEnabled() && accessibility.isTouchExplorationEnabled();
    }

    @Override
    public boolean gestureGuideActive() {
        return gestureGuideOpen;
    }

    @Override
    public boolean autonomousDriveTravelActive() {
        return autonomousDriveTrip && rawNativeTravelState() != 0
                && !autonomousDriveCancelRequested
                && !gestureGuideOpen && !commandPaletteOpen && !comprehensiveMenu
                && editor == null && !closed && foreground && windowFocused
                && snapshot != null
                && ( snapshot.find("activity:autodrive") != null ||
                     "DEFAULTMODE".equals(snapshot.category) );
    }

    private void requestAutonomousRead(String id) {
        if (!autonomousDriveTravelActive() || snapshot.find(id) == null) { return; }
        String previous = focus;
        focus = id;
        send("activate", "");
        focus = previous;
    }

    private void pauseOrResumeAutonomousDrive() {
        if (!autonomousDriveTravelActive()) { return; }
        int state = travelState();



        int command = state == 1 ? 1
                : (state == 2 || state == 3 || state == 4 ? 2 : 0);
        if (command == 0) { return; }
        try {
            if (CataclysmDDA.nativeTravelControl(command)) {
                feedback(activity.getString(R.string.a11yTravelRequestAccepted));
            } else {
                feedback(activity.getString(R.string.a11yTravelRequestUnavailable));
            }
        } catch (UnsatisfiedLinkError unavailable) {
            feedback(activity.getString(R.string.a11yReaderNativeUnavailable));
        }
    }

    @Override
    public String gestureContext() {
        if (gestureGuideOpen) {
            return presentationEpoch + ":gesture-guide:"
                    + (snapshot == null ? "" : snapshot.contextKey());
        }
        if (debugReport != null) { return presentationEpoch + ":debug"; }
        if (walkingTravelPanelActive()) {
            return presentationEpoch + ":travel:" + travelState();
        }
        if (comprehensiveMenu) {
            return presentationEpoch + ":hub:" + hubSection + ":" + choosingGesture
                    + ":" + choosingHubAction
                    + ":" + (bindingItem == null ? "" : bindingItem.shortcutTarget())
                    + ":" + (hubActionItem == null ? "" : hubActionItem.shortcutTarget())
                    + (editor == null ? "" : ":editor:" + editor.contextKey());
        }




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
        if (gestureGuideOpen) {
            gestureGuideIndex = Math.floorMod(gestureGuideIndex + delta,
                    gestureGuideRows.size());
            render();
            readCurrent();
            return;
        }
        if (debugReport != null) {
            debugFocus = Math.floorMod(debugFocus + delta, 4);
            readCurrent();
            return;
        }
        if (editor != null) { editor.navigate(delta); return; }
        if (snapshot == null && !comprehensiveMenu) {



            readCurrent();
            return;
        }
        if (comprehensiveMenu) {
            if (choosingHubAction) {
                hubActionChoice = Math.floorMod(hubActionChoice + delta, 3);
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
        if (walkingTravelPanelActive()) {
            int state = travelState();
            travelChoice = Math.floorMod(travelChoice + delta, travelChoiceCount(state));
            render();
            readCurrent();
            return;
        }



        if (documentReader.isOpen() && !commandPaletteOpen) {


            speech.stop();
            speakDocument(delta > 0 ? documentReader.nextParagraph()
                    : documentReader.previousParagraph());
            return;
        }
        List<ReaderSnapshot.Node> filtered = visibleNodes();
        if (filtered.isEmpty()) { return; }



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
                    readCurrent();
                    return;
                }
                if (nextRecipe < 0) {
                    if (craftingCommand("craft:page:previous")) { return; }
                    readCurrent();
                    return;
                }
                focus = recipeNodes.get(nextRecipe).id;
                render();
                send("focus", "");
                readCurrent();
                return;
            }
        }



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


        int selected = Math.floorMod(index() + delta, filtered.size());
        focus = filtered.get(selected).id;
        render();


        if (!GESTURE_GUIDE_NODE.equals(focus) && !TARGET_TOGGLE_NODE.equals(focus)
                && !TRAVEL_STATUS_NODE.equals(focus)
                && !isAuxiliaryCommand(filtered.get(selected))) {
            send("focus", "");
        }
        readCurrent();
    }













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



        if (isItemQuantityRow(node)) { return false; }


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


            return creatorCalendarAction(horizontal > 0 ? "RIGHT" : "LEFT");
        }
        if (vertical != 0) {

            navigate(vertical > 0 ? 1 : -1);
            return true;
        }
        return false;
    }


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

            return false;
        }
        return craftingCommand(vertical < 0 ? "LEFT" : "RIGHT");
    }





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


            return surroundingsCommand(vertical < 0
                    ? "nearby:filter:next" : "nearby:filter:previous");
        }
        if (horizontal != 0) {



            if (horizontal > 0 && nearbyFirstItemPending) {
                nearbyFirstItemPending = false;
                readCurrent();
                return true;
            }
            nearbyFirstItemPending = false;


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
        if (closed) { return; }
        if (gestureGuideOpen) { readCurrent(); return; }
        if (debugReport != null) { return; }
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
        if (walkingTravelPanelActive()) { activateTravel(); return; }
        if (snapshot == null || directWorldGesturesEnabled()) { return; }




        if (isCreatorMultiChoiceStep() && creatorStepAction("NEXT_TAB")) { return; }
        if (creatorCalendarAction("CALENDAR_DONE")) { return; }



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



        if ("crafting".equals(snapshot.category)) {

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


        return directWorldGesturesEnabled() && snapshot.swipeMovementAllowed
                && !vehicleReleasePending && !vehicleDriverReady()
                && hasCardinalDirections(snapshot);
    }

    @Override
    public boolean worldMovementEnabled() {


        return !combatMode && !vehicleMode && !autonomousDriveTrip
                && worldStepAllowed();
    }

    private static boolean hasCardinalDirections(ReaderSnapshot snapshot) {
        for (String direction : new String[] { "UP", "DOWN", "LEFT", "RIGHT" }) {
            if (snapshot.find(direction) == null) { return false; }
        }
        return true;
    }

    @Override
    public boolean directWorldGesturesEnabled() {
        return debugReport == null && !comprehensiveMenu
                && !commandPaletteOpen && !gestureGuideOpen && !closed && foreground
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



    private static String vehicleCompass(String description) {
        if (description == null) { return ""; }
        int end = description.indexOf('.');
        return (end < 0 ? description : description.substring(0, end)).trim();
    }

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
            ReaderSnapshot.Node previous = snapshot.find("reader:vehicle:heading");
            vehicleHeadingBeforeCommand = vehicleCompass(previous == null ? "" : previous.detail);
            pendingVehicleHeadingSnapshots = 0;
            pendingVehicleHeadingReport = true;
        }


        requestLiveWorldCommand(id);
        if ("reader:vehicle:brake".equals(id)) {
            feedback(activity.getString(R.string.a11yDriveBraking));
        }
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



    @Override
    public boolean handleVehicleTwoFingerSwipe(int horizontal, int vertical) {
        if (!vehicleMode || (!directWorldGesturesEnabled()
                && !autonomousDriveTravelActive())) { return false; }
        scanDirection(horizontal, vertical);
        return true;
    }

    @Override
    public boolean handleCustomGesture(String gestureId) {
        if (autonomousDriveTravelActive()) {



            switch (gestureId) {
                case "swipe_up":
                case "swipe_down":
                case "swipe_left":
                case "swipe_right":
                    return true;
                case "swipe_down_left":
                case "swipe_up_down":
                case "swipe_down_up":
                    speakVehicleDocument("reader:vehicle:speed");
                    return true;
                case "swipe_down_right":
                case "swipe_up_right":
                    speakVehicleDocument("reader:vehicle:detail");
                    return true;
                case "swipe_left_right":
                case "swipe_right_left":
                    speakVehicleDocument("reader:vehicle:heading");
                    return true;
                case "swipe_up_left":
                    pauseOrResumeAutonomousDrive();
                    return true;
                case "swipe_right_down":
                    requestAutonomousRead("reader:vehicle:horn");
                    return true;
                default:




                    if (gestureId.startsWith("swipe_3_")) {
                        String target = gestureBindings.targetFor(gestureId);
                        if (target != null && (target.startsWith("hub:group:")
                                || "hub:directions".equals(target)
                                || "hub:favorites".equals(target)
                                || "hub:search".equals(target)
                                || "hub:all".equals(target)
                                || "hub:tts".equals(target)
                                || "hub:debug_log".equals(target))) {
                            executeShortcutTarget(target);
                        } else if ("action:reader:coordinates".equals(target)) {
                            requestAutonomousRead("reader:coordinates");
                        } else if ("action:reader:vehicle:speed".equals(target)
                                || "action:reader:vehicle:heading".equals(target)
                                || "action:reader:vehicle:summary".equals(target)) {
                            speakVehicleDocument(target.substring("action:".length()));
                        }
                    }
                    return true;
            }
        }


        if (vehicleReleasePending && directWorldGesturesEnabled()) {
            return true;
        }
        if (vehicleMode && directWorldGesturesEnabled()) {


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
                case "swipe_up_right":
                    requestVehicleAction("reader:vehicle:engine");
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
                case "swipe_down_left":
                    speakVehicleDocument("reader:vehicle:speed");
                    return true;
                case "swipe_down_right":
                    speakVehicleDocument("reader:vehicle:detail");
                    return true;
                case "swipe_up_left":
                    requestVehicleAction("reader:vehicle:brake");
                    return true;
                case "swipe_right_down":
                    requestVehicleAction("reader:vehicle:horn");
                    return true;
                default:
                    // Do not fall through to saved map or combat bindings


                    return true;
            }
        }
        if (combatMode && directWorldGesturesEnabled()) {


            switch (gestureId) {
                case "swipe_3_up":
                case "swipe_3_down":
                case "swipe_3_left":
                case "swipe_3_right":
                    return true;
                default: break;
            }




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

            return false;
        }
        String target = gestureBindings.targetFor(gestureId);
        SessionDebugLog.event("GESTURE_BINDING", "gesture=" + gestureId
                + " target=" + target);
        if (target == null) { return false; }

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


            moveInCombat(dx, dy);
            return;
        }
        if (vehicleMode) {
            if (!vehicleDriverReady()) {
                feedback(activity.getString(R.string.a11yDriveNotReady));
                return;
            }


            String relative = dy < 0 ? "front" : dy > 0 ? "back"
                    : dx < 0 ? "left" : "right";
            if (autonomousDriveTravelActive()) {
                requestAutonomousRead("reader:scan:vehicle:" + relative);
            } else {
                requestLiveWorldCommand("reader:scan:vehicle:" + relative);
            }
            return;
        }
        String id = dy < 0 ? "reader:scan:north" : dy > 0 ? "reader:scan:south"
                : dx < 0 ? "reader:scan:west" : "reader:scan:east";
        requestLiveWorldCommand(id);
    }

    @Override
    public void twoFingerTap(int count) {
        if (gestureGuideOpen) {
            readCurrent();
            return;
        }
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



            String spoken = lastRecentMessages;
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
        if (autonomousDriveTravelActive()) {
            requestAutonomousRead("reader:coordinates");
        } else {
            requestLiveWorldCommand("reader:coordinates");
        }
    }

    @Override
    public void toggleCombatMode() {
        if (autonomousDriveTravelActive()) {



            try {
                if (!CataclysmDDA.nativeTravelControl(3)) {
                    feedback(activity.getString(R.string.a11yTravelRequestUnavailable));
                    return;
                }
            } catch (UnsatisfiedLinkError unavailable) {
                feedback(activity.getString(R.string.a11yReaderNativeUnavailable));
                return;
            }
            autonomousDriveCancelRequested = true;
            vehicleMode = false;
            combatMode = false;
            clearPendingWorldDirection();
            root.cancelGesture();
            feedback(activity.getString(R.string.a11yAutoDriveCancelToMap));
            return;
        }
        if (!directWorldGesturesEnabled()) { return; }
        if (snapshot.find("reader:vehicle:seat") != null) {



            int nextMode = combatMode ? 2 : vehicleMode ? 0 : 1;



            CataclysmDDA.nativeCancelPendingWorldSteps();
            clearPendingWorldDirection();
            root.cancelGesture();
            if (vehicleMode && nextMode != 2) {


                vehicleReleasePending = vehicleDriverReady();
                if (vehicleReleasePending) {
                    requestLiveWorldCommand("reader:vehicle:release");
                }
            } else if (nextMode == 2) {



                vehicleReleasePending = false;
                requestLiveWorldCommand("reader:vehicle:begin");
            }
            combatMode = nextMode == 1;
            vehicleMode = nextMode == 2;
            if (!vehicleMode) {
                observedEngineVehicle = "";
                observedEngineState = "";
            }
            SessionDebugLog.event("VEHICLE_MODE", "selected=" + nextMode
                    + " releasing=" + vehicleReleasePending);


            feedback(activity.getString(vehicleReleasePending && nextMode == 0
                    ? R.string.a11yDriveBraking
                    : nextMode == 0 ? R.string.a11yMapMode
                    : nextMode == 1 ? R.string.a11yCombatOn
                    : R.string.a11yDrivingMode));
            return;
        }
        vehicleMode = false;
        observedEngineVehicle = "";
        observedEngineState = "";
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

        feedback(node.detail);
    }

    private void requestCombatCommand(String id) {
        if (!combatMode || !directWorldGesturesEnabled()) { return; }
        if (vehicleReleasePending || vehicleDriverReady()) {
            feedback(activity.getString(R.string.a11yDriveNotReady));
            return;
        }
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
        if (!directWorldGesturesEnabled() && !autonomousDriveTravelActive()) { return; }
        ReaderSnapshot.Node document = snapshot.find("world:character_detail");
        String spoken = document == null ? "" : document.detail;
        if (spoken.isEmpty()) {
            feedback(activity.getString(R.string.a11yReaderWaiting));
        } else if (usesTouchExploration()) {
            feedback(spoken);
        } else {

            speech.say("world-character-detail", spoken, ReaderSpeechQueue.LONG_TEXT);
        }
    }

    @Override
    public void openActionMenu() {

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

        focus = previous;
    }

    @Override
    public void activate() {
        SessionDebugLog.event("ACTION", "activate focus=" + focus
                + " hub=" + comprehensiveMenu);
        if (gestureGuideOpen) {

            readCurrent();
            return;
        }
        if (commandPaletteOpen && GESTURE_GUIDE_NODE.equals(focus)) {
            openGestureGuide();
            return;
        }
        if (commandPaletteOpen && TRAVEL_STATUS_NODE.equals(focus)) {
            feedback(travelDetailSpeech(travelContextDrawerState));
            return;
        }
        if (commandPaletteOpen && TARGET_TOGGLE_NODE.equals(focus)) {
            commandPaletteOpen = false;
            targetingOriginalControls = !targetingOriginalControls;
            focus = targetingOriginalControls ? "target:cursor" : "target:fire-direction:0";
            readerLayerParent = "";
            root.cancelGesture();
            render();
            feedback(activity.getString(targetingOriginalControls
                    ? R.string.a11yTargetOriginalControls : R.string.a11yTargetDirections));
            return;
        }


        if (comprehensiveMenu) {
            if (choosingHubAction) { applyHubActionChoice(); }
            else if (choosingGesture) { applyGestureChoice(); }
            else { activateHub(); }
            return;
        }


        if (!activatingHubCommand && autonomousDriveTravelActive()) {
            pauseOrResumeAutonomousDrive();
            return;
        }

        if (!activatingHubCommand && vehicleMode && directWorldGesturesEnabled()) {
            requestVehicleAction("reader:vehicle:pause");
            return;
        }
        if (debugReport != null) {
            if (debugFocus == 0) { readCurrent(); }
            else { submitDebugResponse(debugFocus); }
            return;
        }
        if (editor != null) { editor.activate(); return; }
        if (pocketStage != 0) {
            activatePocketPicker();
            return;
        }
        if (walkingTravelPanelActive()) { activateTravel(); return; }
        ReaderSnapshot.Node node = snapshot == null ? null : snapshot.find(focus);
        if (node == null) { return; }
        if (!node.enabled) { feedback(nodeText(node, true)); return; }


        if (node.id.startsWith("group:")) {
            feedback(activity.getString(R.string.a11yCommandGroup) + ". " + node.label);
            return;
        }






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




        if ("tab".equals(node.role) && node.actions.contains("activate")) {
            pendingReaderLayer = node.id;
            pendingReaderLayerRevisions = 0;
        }






        if (isFlatPickupPocketEntry(node)) {
            activateFlatPickupPocketEntry(node);
            return;
        }


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


            speakDocument(documentReader.open(snapshot.contextKey(), node.id, node.detail));
            return;
        }






        if (!node.actions.contains("activate")) {
            feedback(nodeText(node, true));
            return;
        }
        send("activate", "");
    }

    private void openEditor(ReaderSnapshot.Node node) {
        editorContext = snapshot.contextKey();
        editorRequest = 0;


        editor = new ReaderTextEditor(activity, node.value, "number".equals(node.role),
                "int".equals(node.fieldType), null, null,
                node.fieldStep > 0 ? node.fieldStep : null,
                new ReaderTextEditor.Listener() {
                    @Override
                    public void speak(String text) { feedback(text); }
                    @Override
                    public void submit(String text) {



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








    private void openFormEditor(ReaderSnapshot.Node node) {
        final boolean quickQuantity = "checkbox".equals(node.role)
                && "int".equals(node.fieldType) && node.hasFieldRange
                && node.fieldMin == 0.0 && node.fieldMax > 1.0 && node.fieldStep == 1.0
                && node.actions.contains("set_value") && !node.actions.contains("apply");
        formContext = snapshot.contextKey();
        formRequest = 0;
        final String nodeId = node.id;




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



        if (closed || owner == null || !foreground || !windowFocused || lastSuspended) { return 0; }





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



                final boolean walking = "activate".equals(action)
                        && "DEFAULTMODE".equals(owner.category)
                        && !comprehensiveMenu && ("UP".equals(focus) || "DOWN".equals(focus)
                        || "LEFT".equals(focus) || "RIGHT".equals(focus)
                        || "LEFTUP".equals(focus) || "LEFTDOWN".equals(focus)
                        || "RIGHTUP".equals(focus) || "RIGHTDOWN".equals(focus));
                if (walking) {



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




        if (hubSection == GameActionHub.SEARCH && !hubQuery.trim().isEmpty()) {
            return R.string.a11yHubNoMatches;
        }
        return R.string.a11yHubEmpty;
    }

    private List<GameActionHub.Entry> hubEntries() {
        List<GameActionHub.Entry> result =
                GameActionHub.entries(activity, snapshot, hubSection, hubQuery, favorites);

        if (!combatMode) {
            result.removeIf(entry -> "reader:combat:aim-toggle".equals(entry.node));
        }


        result.add(new GameActionHub.Entry(
                activity.getString(R.string.a11yGestureGuideTitle), "",
                GameActionHub.OPEN_GESTURE_GUIDE, 0));
        return result;
    }

    private String hubActionLabel(int choice) {
        if (choice == 0) { return activity.getString(R.string.a11yHubGestureOption); }
        if (choice == 2) { return activity.getString(R.string.a11yGestureGuideTitle); }
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
        int count = choosingHubAction ? 3
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
        if (binding || choosingHubAction) {


            renderGuideButton();
        }
    }


    public void showComprehensiveMenu() {
        if (comprehensiveMenu || debugReport != null
                || (!directWorldGesturesEnabled() && !autonomousDriveTravelActive()
                    && travelState() != 4)) {
            return;
        }
        root.cancelGesture();



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
                + activity.getString(R.string.a11yPosition, 1, 3));
    }

    private void applyHubActionChoice() {
        if (!choosingHubAction || hubActionItem == null) { return; }
        GameActionHub.Entry entry = hubActionItem;
        String target = entry.shortcutTarget();
        int choice = hubActionChoice;
        if (choice == 2) {


            openGestureGuide();
            return;
        }
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
            case GameActionHub.OPEN_GESTURE_GUIDE:
                openGestureGuide();
                return;
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
                activatingHubCommand = true;
                try {
                    activate();
                } finally {
                    activatingHubCommand = false;
                }
                return;
            default:
                return;
        }
    }

    @Override
    public void readCurrent() {
        if (gestureGuideOpen) {
            feedback(gestureGuideSpeech());
            return;
        }
        if (commandPaletteOpen && TRAVEL_STATUS_NODE.equals(focus)) {
            feedback(travelDetailSpeech(travelContextDrawerState));
            return;
        }
        if (commandPaletteOpen && (GESTURE_GUIDE_NODE.equals(focus)
                || TARGET_TOGGLE_NODE.equals(focus))) {
            feedback(GESTURE_GUIDE_NODE.equals(focus)
                    ? activity.getString(R.string.a11yGestureGuideTitle)
                    : activity.getString(targetingOriginalControls
                            ? R.string.a11yTargetDirections : R.string.a11yTargetOriginalControls));
            return;
        }
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
                        + activity.getString(R.string.a11yPosition, hubActionChoice + 1, 3));
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
        if (walkingTravelPanelActive()) {
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
        if (snapshot != null && "player_display".equals(snapshot.category)
                && !commandPaletteOpen) {


            String full = nodeText(node, true);
            if (usesTouchExploration()) {
                announceAccessibility(full);
            } else {
                speech.focus(node.id, full);
            }
            return;
        }



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


        full.insert(0, nodeText(selected, true) + "\n\n");


        speakInlineDetail(full.toString());
        return true;
    }

    @Override
    public void detail() {
        if (gestureGuideOpen) { readCurrent(); return; }
        if (commandPaletteOpen && (GESTURE_GUIDE_NODE.equals(focus)
                || TARGET_TOGGLE_NODE.equals(focus)
                || TRAVEL_STATUS_NODE.equals(focus))) {
            readCurrent();
            return;
        }
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
        if (walkingTravelPanelActive()) {
            feedback(travelDetailSpeech(travelState()));
            return;
        }




        if (pocketStage == 1 || (pocketStage == 3 &&
                (POCKET_CONFIRM_NODE.equals(focus) || POCKET_CANCEL_NODE.equals(focus)))) {
            feedback(pocketChoiceSpeech());
            return;
        }
        ReaderSnapshot.Node node = snapshot == null ? null : snapshot.find(focus);
        if (node != null) {


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



            if ("document".equals(node.role) && !node.detail.isEmpty()) {
                speakInlineDetail(node.detail);
                return;
            }
            String information = nodeText(node, true);



            if (node.id.startsWith("item:") && node.hasChildren) {
                for (ReaderSnapshot.Node child : snapshot.nodes) {
                    if (node.id.equals(child.parentId) && child.id.startsWith("pocket:")) {
                        information += ". " + nodeText(child, true);
                    }
                }
            }


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
        if (gestureGuideOpen) { closeGestureGuide(); return; }
        if (debugReport != null) { submitDebugResponse(1); return; }
        if (editor != null) { editor.cancelInput(); return; }
        if (pocketStage != 0) {
            cancelPocketPicker();
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
        if (autonomousDriveTrip && rawNativeTravelState() != 0) {


            travelChoice = 1;
            activateTravel();
            return;
        }
        if (walkingTravelPanelActive()) {
            if (travelState() == 4) {



                travelChoice = travelChoiceCount(travelState()) - 1;
                activateTravel();
            } else {

                travelChoice = travelChoiceCount(travelState()) - 1;
                render();
                readCurrent();
            }
            return;
        }
        if (documentReader.isOpen()) {
            documentReader.clear();
            speech.stop();
            readCurrent();
            return;
        }
        if (backReaderLayer()) { return; }



        if (creatorStepAction("PREV_TAB")) { return; }



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


    public boolean canToggleReaderOutsideWorld() {


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
        observedEngineVehicle = "";
        observedEngineState = "";
        vehicleReleasePending = false;
        autonomousDriveTrip = false;
        autonomousDriveCancelRequested = false;
        gestureGuideOpen = false;
        gestureGuideRows.clear();
        travelContextDrawerState = 0;
        pendingVehicleHeadingReport = false;
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

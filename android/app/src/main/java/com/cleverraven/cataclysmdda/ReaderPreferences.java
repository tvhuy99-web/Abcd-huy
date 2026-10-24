package com.cleverraven.cataclysmdda;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.SystemClock;

/** Reader configuration is separate from game saves and the existing SDL input settings. */
public final class ReaderPreferences {
    public static final int GESTURE_ACTION_NAVIGATE = 0;
    public static final int GESTURE_ACTION_DETAIL = 1;
    public static final int GESTURE_ACTION_READ_CURRENT = 2;
    public static final int GESTURE_ACTION_NONE = 3;
    public static final long GESTURE_MAP_TRIAL_MS = 45_000L;

    // Legacy persisted trial keys (pre-R00). A trial used to store its pending
    // flag and a wall-clock deadline in SharedPreferences. A monotonic
    // deadline must never be persisted: SystemClock.elapsedRealtime() restarts
    // at reboot, so a stored deadline would be meaningless in a later boot.
    // Trial state therefore lives only in process memory (below); any of
    // these keys found on disk are leftovers of an earlier process or of the
    // old format and are discarded, which returns the last confirmed map.
    private static final String LEGACY_GESTURE_TRIAL_PENDING = "gesture_map_trial_pending";
    private static final String LEGACY_GESTURE_TRIAL_DEADLINE = "gesture_map_trial_deadline";
    private static final String TRIAL_HORIZONTAL_ACTION = "trial_horizontal_swipe_action";
    private static final String TRIAL_VERTICAL_ACTION = "trial_vertical_swipe_action";

    // In-memory trial session, shared by every ReaderPreferences instance in
    // this process. Process death, an app restart or a reboot clears it, so a
    // trial can never survive into a session that did not start it; the
    // confirmed map in SharedPreferences is used again instead.
    private static boolean trialPendingInMemory;
    private static long trialStartedElapsedMs;

    private final SharedPreferences values;

    public ReaderPreferences(Context context) {
        values = context.getApplicationContext().getSharedPreferences("internal_reader", Context.MODE_PRIVATE);
        if (!trialPendingInMemory && (values.contains(LEGACY_GESTURE_TRIAL_PENDING)
                || values.contains(LEGACY_GESTURE_TRIAL_DEADLINE)
                || values.contains(TRIAL_HORIZONTAL_ACTION)
                || values.contains(TRIAL_VERTICAL_ACTION))) {
            // No live trial in this process: persisted trial entries are
            // stale (previous process or legacy format). Drop them so the
            // confirmed gesture map applies.
            values.edit().remove(TRIAL_HORIZONTAL_ACTION).remove(TRIAL_VERTICAL_ACTION)
            .remove(LEGACY_GESTURE_TRIAL_PENDING).remove(LEGACY_GESTURE_TRIAL_DEADLINE).apply();
        }
    }

    public boolean isEnabled() {
        return values.getBoolean("enabled", false) && values.getBoolean("heard_confirmation", false);
    }

    public boolean permitsNetworkVoice() {
        return values.getBoolean("network_voice_consent", false);
    }

    // Confirmed speech selection (R06.1). Only an explicit confirmation in
    // the setup screen writes these keys; trying a voice works on draft
    // values held by the setup Activity and never reaches this storage.
    // Engine/voice are stored as stable names and re-resolved against the
    // live engine on every initialize, so a removed voice is detected
    // (MISSING_VOICE) instead of silently replaced.
    private static final String SPEECH_ENGINE = "speech_engine";
    private static final String SPEECH_VOICE = "speech_voice";
    private static final String SPEECH_LANGUAGE = "speech_language";
    public static final String DEFAULT_SPEECH_LANGUAGE = "vi-VN";

    /** Null when the user has never confirmed an engine: use the system default. */
    public String getSpeechEngineName() {
        String engine = values.getString(SPEECH_ENGINE, null);
        return engine == null || engine.isEmpty() ? null : engine;
    }

    /** Null when no specific voice was confirmed: use the engine default voice. */
    public String getSpeechVoiceName() {
        String voice = values.getString(SPEECH_VOICE, null);
        return voice == null || voice.isEmpty() ? null : voice;
    }

    /** BCP-47 tag of the confirmed reading language; Vietnamese by default. */
    public String getSpeechLanguageTag() {
        String tag = values.getString(SPEECH_LANGUAGE, DEFAULT_SPEECH_LANGUAGE);
        return tag == null || tag.isEmpty() ? DEFAULT_SPEECH_LANGUAGE : tag;
    }

    public void saveSpeechSelection(String engineName, String voiceName, String languageTag) {
        SharedPreferences.Editor editor = values.edit();
        if (engineName == null || engineName.isEmpty()) { editor.remove(SPEECH_ENGINE); }
        else { editor.putString(SPEECH_ENGINE, engineName); }
        if (voiceName == null || voiceName.isEmpty()) { editor.remove(SPEECH_VOICE); }
        else { editor.putString(SPEECH_VOICE, voiceName); }
        if (languageTag == null || languageTag.isEmpty()) { editor.remove(SPEECH_LANGUAGE); }
        else { editor.putString(SPEECH_LANGUAGE, languageTag); }
        editor.apply();
    }

    /** Drop a confirmed selection that no longer exists on this device. */
    public void clearSpeechSelection() {
        values.edit().remove(SPEECH_ENGINE).remove(SPEECH_VOICE).remove(SPEECH_LANGUAGE).apply();
    }

    public float getRate() {
        return Math.max(0.2f, Math.min(3.0f, values.getFloat("rate", 1.0f)));
    }

    // Optional sound/haptic cues (R21.2). Both default OFF: speech is
    // the primary channel and every cue duplicates a spoken signal, so
    // cues are an explicit opt-in in reader setup, never a surprise.
    public boolean areSoundCuesEnabled() {
        return values.getBoolean("sound_cues", false);
    }

    public void setSoundCuesEnabled(boolean enabled) {
        values.edit().putBoolean("sound_cues", enabled).apply();
    }

    public boolean areHapticCuesEnabled() {
        return values.getBoolean("haptic_cues", false);
    }

    public void setHapticCuesEnabled(boolean enabled) {
        values.edit().putBoolean("haptic_cues", enabled).apply();
    }

    // Only an explicit user confirmation, never an onDone callback, enables the reader.
    public void confirmHeardAndEnable(boolean permitNetworkVoice, float rate) {
        values.edit().putBoolean("heard_confirmation", true).putBoolean("enabled", true)
                .putBoolean("network_voice_consent", permitNetworkVoice)
                .putFloat("rate", Math.max(0.2f, Math.min(3.0f, rate))).apply();
    }

    public void disable() {
        values.edit().putBoolean("enabled", false).apply();
    }

    public boolean isFastMovementEnabled() {
        return values.getBoolean("fast_movement", false);
    }

    public void setFastMovementEnabled(boolean enabled) {
        values.edit().putBoolean("fast_movement", enabled).apply();
    }

    public boolean hasCompletedMovementPractice() {
        return values.getBoolean("movement_practice_complete", false);
    }

    public void setMovementPracticeComplete() {
        values.edit().putBoolean("movement_practice_complete", true).apply();
    }

    public boolean isLeftHandedNavigation() {
        return values.getBoolean("left_handed_navigation", false);
    }

    public void setLeftHandedNavigation(boolean enabled) {
        values.edit().putBoolean("left_handed_navigation", enabled).apply();
    }

    /** Minimum swipe distance in dp: short, standard, long. */
    public int getSwipeDistanceDp() {
        switch (values.getInt("swipe_distance_level", 1)) {
            case 0: return 32;
            case 2: return 72;
            default: return 48;
        }
    }

    public int getSwipeDistanceLevel() {
        return Math.max(0, Math.min(2, values.getInt("swipe_distance_level", 1)));
    }

    public void setSwipeDistanceLevel(int level) {
        values.edit().putInt("swipe_distance_level", Math.max(0, Math.min(2, level))).apply();
    }

    /** Maximum swipe duration in ms: quick, standard, deliberate. */
    public int getSwipeDurationLimitMs() {
        switch (values.getInt("swipe_duration_level", 1)) {
            case 0: return 450;
            case 2: return 1200;
            default: return 750;
        }
    }

    public int getSwipeDurationLevel() {
        return Math.max(0, Math.min(2, values.getInt("swipe_duration_level", 1)));
    }

    public void setSwipeDurationLevel(int level) {
        values.edit().putInt("swipe_duration_level", Math.max(0, Math.min(2, level))).apply();
    }

    public int getHorizontalSwipeAction() {
        return getGestureAction("horizontal_swipe_action", TRIAL_HORIZONTAL_ACTION);
    }

    public int getVerticalSwipeAction() {
        return getGestureAction("vertical_swipe_action", TRIAL_VERTICAL_ACTION);
    }

    private int getGestureAction(String confirmedKey, String trialKey) {
        if (isGestureMapTrialActive()) {
            return normalizeGestureAction(values.getInt(trialKey, GESTURE_ACTION_NAVIGATE));
        }
        return normalizeGestureAction(values.getInt(confirmedKey, GESTURE_ACTION_NAVIGATE));
    }

    /**
     * Store a candidate map without changing the last confirmed map. The
     * candidate actions are kept in SharedPreferences only as inert data; the
     * pending flag and the start time live solely in process memory, so a
     * process death, restart or reboot abandons the trial and the confirmed
     * map applies again (see the constructor cleanup).
     */
    public void beginGestureMapTrial(int horizontalAction, int verticalAction) {
        horizontalAction = normalizeGestureAction(horizontalAction);
        verticalAction = normalizeGestureAction(verticalAction);
        // Keep at least one one-finger swipe axis available for menu browsing.
        if (horizontalAction != GESTURE_ACTION_NAVIGATE && verticalAction != GESTURE_ACTION_NAVIGATE) {
            horizontalAction = GESTURE_ACTION_NAVIGATE;
        }
        values.edit().putInt(TRIAL_HORIZONTAL_ACTION, horizontalAction)
        .putInt(TRIAL_VERTICAL_ACTION, verticalAction)
        .remove(LEGACY_GESTURE_TRIAL_PENDING).remove(LEGACY_GESTURE_TRIAL_DEADLINE).apply();
        trialPendingInMemory = true;
        trialStartedElapsedMs = 0L;
    }

    /** Start the timeout on the first game-reader resume, after setup has returned. */
    public boolean startGestureMapTrialTimer() {
        if (!isGestureMapTrialPending()) {
            return false;
        }
        if (trialStartedElapsedMs == 0L) {
            trialStartedElapsedMs = SystemClock.elapsedRealtime();
        }
        return isGestureMapTrialActive();
    }

    public boolean isGestureMapTrialPending() {
        return trialPendingInMemory;
    }

    public boolean isGestureMapTrialActive() {
        if (!isGestureMapTrialPending()) {
            return false;
        }
        if (trialStartedElapsedMs > 0L
                && SystemClock.elapsedRealtime() - trialStartedElapsedMs >= GESTURE_MAP_TRIAL_MS) {
            revertGestureMapTrial();
            return false;
        }
        return true;
    }

    public long gestureMapTrialRemainingMs() {
        if (!isGestureMapTrialActive()) {
            return 0L;
        }
        return trialStartedElapsedMs == 0L ? GESTURE_MAP_TRIAL_MS
               : Math.max(0L, GESTURE_MAP_TRIAL_MS
                          - (SystemClock.elapsedRealtime() - trialStartedElapsedMs));
    }

    public boolean confirmGestureMapTrial() {
        if (!isGestureMapTrialActive()) {
            return false;
        }
        int horizontal = values.getInt(TRIAL_HORIZONTAL_ACTION, GESTURE_ACTION_NAVIGATE);
        int vertical = values.getInt(TRIAL_VERTICAL_ACTION, GESTURE_ACTION_NAVIGATE);
        values.edit().putInt("horizontal_swipe_action", horizontal)
        .putInt("vertical_swipe_action", vertical)
        .remove(TRIAL_HORIZONTAL_ACTION).remove(TRIAL_VERTICAL_ACTION)
        .remove(LEGACY_GESTURE_TRIAL_PENDING).remove(LEGACY_GESTURE_TRIAL_DEADLINE).apply();
        trialPendingInMemory = false;
        trialStartedElapsedMs = 0L;
        return true;
    }

    public void revertGestureMapTrial() {
        trialPendingInMemory = false;
        trialStartedElapsedMs = 0L;
        values.edit().remove(TRIAL_HORIZONTAL_ACTION).remove(TRIAL_VERTICAL_ACTION)
        .remove(LEGACY_GESTURE_TRIAL_PENDING).remove(LEGACY_GESTURE_TRIAL_DEADLINE).apply();
    }

    private static int normalizeGestureAction(int action) {
        return action >= GESTURE_ACTION_NAVIGATE && action <= GESTURE_ACTION_NONE
               ? action : GESTURE_ACTION_NAVIGATE;
    }

    public void restoreGestureDefaults() {
        trialPendingInMemory = false;
        trialStartedElapsedMs = 0L;
        values.edit().remove("left_handed_navigation").remove("swipe_distance_level")
        .remove("swipe_duration_level").remove("horizontal_swipe_action")
        .remove("vertical_swipe_action").remove(TRIAL_HORIZONTAL_ACTION)
        .remove(TRIAL_VERTICAL_ACTION).remove(LEGACY_GESTURE_TRIAL_PENDING)
        .remove(LEGACY_GESTURE_TRIAL_DEADLINE).apply();
    }
}

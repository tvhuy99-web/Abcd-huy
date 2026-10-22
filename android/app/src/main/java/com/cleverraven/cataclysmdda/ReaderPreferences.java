package com.cleverraven.cataclysmdda;

import android.content.Context;
import android.content.SharedPreferences;

/** Reader configuration is separate from game saves and the existing SDL input settings. */
public final class ReaderPreferences {
    public static final int GESTURE_ACTION_NAVIGATE = 0;
    public static final int GESTURE_ACTION_DETAIL = 1;
    public static final int GESTURE_ACTION_READ_CURRENT = 2;
    public static final int GESTURE_ACTION_NONE = 3;
    public static final long GESTURE_MAP_TRIAL_MS = 45_000L;

    private static final String GESTURE_TRIAL_PENDING = "gesture_map_trial_pending";
    private static final String GESTURE_TRIAL_DEADLINE = "gesture_map_trial_deadline";
    private final SharedPreferences values;

    public ReaderPreferences(Context context) {
        values = context.getApplicationContext().getSharedPreferences("internal_reader", Context.MODE_PRIVATE);
    }

    public boolean isEnabled() {
        return values.getBoolean("enabled", false) && values.getBoolean("heard_confirmation", false);
    }

    public boolean permitsNetworkVoice() {
        return values.getBoolean("network_voice_consent", false);
    }

    public float getRate() {
        return Math.max(0.2f, Math.min(3.0f, values.getFloat("rate", 1.0f)));
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
        return getGestureAction("horizontal_swipe_action", "trial_horizontal_swipe_action");
    }

    public int getVerticalSwipeAction() {
        return getGestureAction("vertical_swipe_action", "trial_vertical_swipe_action");
    }

    private int getGestureAction(String confirmedKey, String trialKey) {
        if (isGestureMapTrialActive()) {
            return normalizeGestureAction(values.getInt(trialKey, GESTURE_ACTION_NAVIGATE));
        }
        return normalizeGestureAction(values.getInt(confirmedKey, GESTURE_ACTION_NAVIGATE));
    }

    /** Store a candidate map without changing the last confirmed map. */
    public void beginGestureMapTrial(int horizontalAction, int verticalAction) {
        horizontalAction = normalizeGestureAction(horizontalAction);
        verticalAction = normalizeGestureAction(verticalAction);
        // Keep at least one one-finger swipe axis available for menu browsing.
        if (horizontalAction != GESTURE_ACTION_NAVIGATE && verticalAction != GESTURE_ACTION_NAVIGATE) {
            horizontalAction = GESTURE_ACTION_NAVIGATE;
        }
        values.edit().putInt("trial_horizontal_swipe_action", horizontalAction)
        .putInt("trial_vertical_swipe_action", verticalAction)
        .putBoolean(GESTURE_TRIAL_PENDING, true)
        .putLong(GESTURE_TRIAL_DEADLINE, 0L).apply();
    }

    /** Start the timeout on the first game-reader resume, after setup has returned. */
    public boolean startGestureMapTrialTimer() {
        if (!isGestureMapTrialPending()) {
            return false;
        }
        if (values.getLong(GESTURE_TRIAL_DEADLINE, 0L) == 0L) {
            values.edit().putLong(GESTURE_TRIAL_DEADLINE,
                                  System.currentTimeMillis() + GESTURE_MAP_TRIAL_MS).apply();
        }
        return isGestureMapTrialActive();
    }

    public boolean isGestureMapTrialPending() {
        return values.getBoolean(GESTURE_TRIAL_PENDING, false);
    }

    public boolean isGestureMapTrialActive() {
        if (!isGestureMapTrialPending()) {
            return false;
        }
        long deadline = values.getLong(GESTURE_TRIAL_DEADLINE, 0L);
        if (deadline > 0L && System.currentTimeMillis() >= deadline) {
            revertGestureMapTrial();
            return false;
        }
        return true;
    }

    public long gestureMapTrialRemainingMs() {
        if (!isGestureMapTrialActive()) {
            return 0L;
        }
        long deadline = values.getLong(GESTURE_TRIAL_DEADLINE, 0L);
        return deadline == 0L ? GESTURE_MAP_TRIAL_MS
               : Math.max(0L, deadline - System.currentTimeMillis());
    }

    public boolean confirmGestureMapTrial() {
        if (!isGestureMapTrialActive()) {
            return false;
        }
        int horizontal = values.getInt("trial_horizontal_swipe_action", GESTURE_ACTION_NAVIGATE);
        int vertical = values.getInt("trial_vertical_swipe_action", GESTURE_ACTION_NAVIGATE);
        values.edit().putInt("horizontal_swipe_action", horizontal)
        .putInt("vertical_swipe_action", vertical)
        .remove("trial_horizontal_swipe_action").remove("trial_vertical_swipe_action")
        .remove(GESTURE_TRIAL_PENDING).remove(GESTURE_TRIAL_DEADLINE).apply();
        return true;
    }

    public void revertGestureMapTrial() {
        values.edit().remove("trial_horizontal_swipe_action").remove("trial_vertical_swipe_action")
        .remove(GESTURE_TRIAL_PENDING).remove(GESTURE_TRIAL_DEADLINE).apply();
    }

    private static int normalizeGestureAction(int action) {
        return action >= GESTURE_ACTION_NAVIGATE && action <= GESTURE_ACTION_NONE
               ? action : GESTURE_ACTION_NAVIGATE;
    }

    public void restoreGestureDefaults() {
        values.edit().remove("left_handed_navigation").remove("swipe_distance_level")
        .remove("swipe_duration_level").remove("horizontal_swipe_action")
        .remove("vertical_swipe_action").remove("trial_horizontal_swipe_action")
        .remove("trial_vertical_swipe_action").remove(GESTURE_TRIAL_PENDING)
        .remove(GESTURE_TRIAL_DEADLINE).apply();
    }
}

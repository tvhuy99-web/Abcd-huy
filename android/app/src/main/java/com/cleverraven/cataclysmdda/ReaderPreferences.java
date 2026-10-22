package com.cleverraven.cataclysmdda;

import android.content.Context;
import android.content.SharedPreferences;

/** Reader configuration is separate from game saves and the existing SDL input settings. */
public final class ReaderPreferences {
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

    public void restoreGestureDefaults() {
        values.edit().remove("left_handed_navigation").remove("swipe_distance_level")
                .remove("swipe_duration_level").apply();
    }
}

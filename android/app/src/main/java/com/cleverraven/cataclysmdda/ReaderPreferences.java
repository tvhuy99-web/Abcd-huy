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
}

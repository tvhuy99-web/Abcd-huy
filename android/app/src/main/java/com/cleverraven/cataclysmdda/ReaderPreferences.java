package com.cleverraven.cataclysmdda;

import android.content.Context;
import android.content.SharedPreferences;

/** Reader configuration is separate from game saves and the existing SDL input settings. */
public final class ReaderPreferences {
    public static final int GESTURE_ACTION_NAVIGATE = 0;

    private final SharedPreferences values;

    public ReaderPreferences(Context context) {
        values = context.getApplicationContext().getSharedPreferences("internal_reader", Context.MODE_PRIVATE);
        // New accessibility builds start the reader automatically. Migrate old
        // trial preferences exactly once, without overriding a later opt-out.
        if (BuildConfig.FLAVOR.equals("accessibility")
                && !values.getBoolean("auto_reader_default_20261107", false)) {
            values.edit().putBoolean("auto_reader_default_20261107", true)
                    .putBoolean("enabled", true).remove("heard_confirmation").apply();
        }
    }

    public boolean isEnabled() {
        return values.getBoolean("enabled", BuildConfig.FLAVOR.equals("accessibility"));
    }

    public void setEnabled(boolean enabled) {
        values.edit().putBoolean("auto_reader_default_20261107", true)
                .putBoolean("enabled", enabled).apply();
    }

    public boolean permitsNetworkVoice() {
        return values.getBoolean("network_voice_consent", false);
    }

    // Confirmed speech selection (R06.1). Only an explicit confirmation in
    // the TTS settings screen writes these keys; trying a voice works on draft
    // values held by the TTS settings Activity and never reaches this storage.
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

    /** Drop a confirmed selection that no longer exists on this device. */
    public void clearSpeechSelection() {
        values.edit().remove(SPEECH_ENGINE).remove(SPEECH_VOICE).remove(SPEECH_LANGUAGE).apply();
    }

    public float getRate() {
        return Math.max(0.2f, Math.min(10.0f, values.getFloat("rate", 1.0f)));
    }

    public float getPitch() {
        return Math.max(0.5f, Math.min(2.0f, values.getFloat("tts_pitch", 1.0f)));
    }

    public float getVolume() {
        return Math.max(0.0f, Math.min(1.0f, values.getFloat("tts_volume", 1.0f)));
    }

    /** Commit every TTS setting together. A preview never changes gameplay. */
    public void saveTtsSettings(boolean enabled, String engineName, String voiceName,
                                String languageTag, boolean allowNetwork, float rate,
                                float pitch, float volume) {
        SharedPreferences.Editor editor = values.edit();
        editor.putBoolean("auto_reader_default_20261107", true);
        editor.putBoolean("enabled", enabled);
        editor.putBoolean("network_voice_consent", allowNetwork);
        editor.putFloat("rate", Math.max(0.2f, Math.min(10.0f, rate)));
        editor.putFloat("tts_pitch", Math.max(0.5f, Math.min(2.0f, pitch)));
        editor.putFloat("tts_volume", Math.max(0.0f, Math.min(1.0f, volume)));
        if (engineName == null || engineName.isEmpty()) { editor.remove(SPEECH_ENGINE); }
        else { editor.putString(SPEECH_ENGINE, engineName); }
        if (voiceName == null || voiceName.isEmpty()) { editor.remove(SPEECH_VOICE); }
        else { editor.putString(SPEECH_VOICE, voiceName); }
        if (languageTag == null || languageTag.isEmpty()) { editor.remove(SPEECH_LANGUAGE); }
        else { editor.putString(SPEECH_LANGUAGE, languageTag); }
        editor.apply();
    }

    // The packaged accessibility cues remain optional. The downloaded
    // gameplay soundpack is a separate native-game audio system.
    public boolean areSoundCuesEnabled() {
        return values.getBoolean("sound_cues", false);
    }

    public boolean areHapticCuesEnabled() {
        return values.getBoolean("haptic_cues", false);
    }

    public void setSoundCuesEnabled(boolean enabled) {
        values.edit().putBoolean("sound_cues", enabled).apply();
    }

    public void setHapticCuesEnabled(boolean enabled) {
        values.edit().putBoolean("haptic_cues", enabled).apply();
    }

    /** Independent directional sonar is opt-in, unlike regular spoken output. */
    public boolean isDirectionalSonarEnabled() {
        return values.getBoolean("directional_sonar", false);
    }

    public void setDirectionalSonarEnabled(boolean enabled) {
        values.edit().putBoolean("directional_sonar", enabled).apply();
    }

    public void disable() {
        values.edit().putBoolean("enabled", false).apply();
    }

    /** Spoken-only mode is the default in the dedicated accessibility APK. */
    public boolean isSpokenOnlyWorldEnabled() {
        return values.getBoolean("spoken_only_world", BuildConfig.FLAVOR.equals("accessibility"));
    }

    public void setSpokenOnlyWorldEnabled(boolean enabled) {
        values.edit().putBoolean("spoken_only_world", enabled).apply();
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
        return GESTURE_ACTION_NAVIGATE;
    }

    public int getVerticalSwipeAction() {
        return GESTURE_ACTION_NAVIGATE;
    }
}

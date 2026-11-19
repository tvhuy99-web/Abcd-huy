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

    // The two sounds already exist independently. Preserve the old single
    // sonar setting as the default for both channels when upgrading an APK,
    // until the user explicitly chooses each new switch.
    private static final String SONAR_CREATURES = "sonar_creatures";
    private static final String SONAR_ITEMS = "sonar_items";

    public boolean isCreatureSonarEnabled() {
        return values.getBoolean(SONAR_CREATURES,
                values.getBoolean("directional_sonar", false));
    }

    public boolean isItemSonarEnabled() {
        return values.getBoolean(SONAR_ITEMS,
                values.getBoolean("directional_sonar", false));
    }

    public void setCreatureSonarEnabled(boolean enabled) {
        values.edit().putBoolean(SONAR_CREATURES, enabled).apply();
    }

    public void setItemSonarEnabled(boolean enabled) {
        values.edit().putBoolean(SONAR_ITEMS, enabled).apply();
    }

    /** Native sensing is needed when either category is enabled. */
    public boolean isDirectionalSonarEnabled() {
        return isCreatureSonarEnabled() || isItemSonarEnabled();
    }

    /** Compatibility for older code and settings: change both channels. */
    public void setDirectionalSonarEnabled(boolean enabled) {
        values.edit().putBoolean("directional_sonar", enabled)
                .putBoolean(SONAR_CREATURES, enabled)
                .putBoolean(SONAR_ITEMS, enabled).apply();
    }

    // Independent spoken radar, not the existing repeating 16-tile audio
    // sonar. Eight Chebyshev tiles, one announcement per boundary entry.
    static final int RADAR_CHANNEL_ITEMS = 1 << 0;
    static final int RADAR_CHANNEL_MONSTERS = 1 << 1;
    static final int RADAR_CHANNEL_PLACES = 1 << 2;
    static final int RADAR_CHANNEL_NPCS = 1 << 3;
    static final int RADAR_CHANNEL_DOORS = 1 << 4;
    static final int RADAR_CHANNEL_HAZARDS = 1 << 5;
    static final int RADAR_CHANNEL_VEHICLES = 1 << 6;
    static final int RADAR_CHANNEL_LANDMARKS = 1 << 7;
    static final int RADAR_CHANNEL_ALL = (1 << 8) - 1;
    static final int RADAR_DEFAULT_CHANNELS = RADAR_CHANNEL_MONSTERS
            | RADAR_CHANNEL_NPCS | RADAR_CHANNEL_DOORS | RADAR_CHANNEL_HAZARDS;
    private static final String RADAR_MASTER = "automatic_radar_enabled";
    private static final String RADAR_CHANNELS = "automatic_radar_channels";

    public boolean isRadarEnabled() {
        return values.getBoolean(RADAR_MASTER, false);
    }

    public void setRadarEnabled(boolean enabled) {
        values.edit().putBoolean(RADAR_MASTER, enabled).apply();
    }

    public int radarChannels() {
        return values.getInt(RADAR_CHANNELS, RADAR_DEFAULT_CHANNELS) & RADAR_CHANNEL_ALL;
    }

    public boolean radarChannelEnabled(int bit) {
        return (radarChannels() & bit) != 0;
    }

    public void setRadarChannelEnabled(int bit, boolean enabled) {
        if (bit == 0 || (bit & RADAR_CHANNEL_ALL) != bit) { return; }
        int channels = radarChannels();
        values.edit().putInt(RADAR_CHANNELS,
                enabled ? channels | bit : channels & ~bit).apply();
    }

    public int activeRadarMask() {
        return isRadarEnabled() ? radarChannels() : 0;
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

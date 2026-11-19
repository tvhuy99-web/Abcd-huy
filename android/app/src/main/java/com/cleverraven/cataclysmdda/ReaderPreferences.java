package com.cleverraven.cataclysmdda;

import android.content.Context;
import android.content.SharedPreferences;


public final class ReaderPreferences {
    public static final int GESTURE_ACTION_NAVIGATE = 0;

    private final SharedPreferences values;

    public ReaderPreferences(Context context) {
        values = context.getApplicationContext().getSharedPreferences("internal_reader", Context.MODE_PRIVATE);


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







    private static final String SPEECH_ENGINE = "speech_engine";
    private static final String SPEECH_VOICE = "speech_voice";
    private static final String SPEECH_LANGUAGE = "speech_language";
    public static final String DEFAULT_SPEECH_LANGUAGE = "vi-VN";


    public String getSpeechEngineName() {
        String engine = values.getString(SPEECH_ENGINE, null);
        return engine == null || engine.isEmpty() ? null : engine;
    }


    public String getSpeechVoiceName() {
        String voice = values.getString(SPEECH_VOICE, null);
        return voice == null || voice.isEmpty() ? null : voice;
    }


    public String getSpeechLanguageTag() {
        String tag = values.getString(SPEECH_LANGUAGE, DEFAULT_SPEECH_LANGUAGE);
        return tag == null || tag.isEmpty() ? DEFAULT_SPEECH_LANGUAGE : tag;
    }


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


    public boolean isDirectionalSonarEnabled() {
        return isCreatureSonarEnabled() || isItemSonarEnabled();
    }


    public void setDirectionalSonarEnabled(boolean enabled) {
        values.edit().putBoolean("directional_sonar", enabled)
                .putBoolean(SONAR_CREATURES, enabled)
                .putBoolean(SONAR_ITEMS, enabled).apply();
    }

    public void disable() {
        values.edit().putBoolean("enabled", false).apply();
    }


    public boolean isSpokenOnlyWorldEnabled() {
        return values.getBoolean("spoken_only_world", BuildConfig.FLAVOR.equals("accessibility"));
    }

    public void setSpokenOnlyWorldEnabled(boolean enabled) {
        values.edit().putBoolean("spoken_only_world", enabled).apply();
    }


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

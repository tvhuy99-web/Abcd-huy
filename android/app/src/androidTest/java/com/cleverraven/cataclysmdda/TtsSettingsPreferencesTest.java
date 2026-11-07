package com.cleverraven.cataclysmdda;

import android.content.Context;
import android.content.SharedPreferences;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.HashMap;
import java.util.Map;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

/** Verify that the dedicated TTS settings persist as one configuration. */
@RunWith(AndroidJUnit4.class)
public class TtsSettingsPreferencesTest {
    @Test
    public void savesMuteVoiceRatePitchVolumeAndKeepsExplicitMute() {
        Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        SharedPreferences store = context.getSharedPreferences(
                "internal_reader", Context.MODE_PRIVATE);
        String[] keys = {
            "auto_reader_default_20261107", "enabled", "network_voice_consent",
            "speech_engine", "speech_voice", "speech_language", "rate",
            "tts_pitch", "tts_volume"
        };
        Map<String, Object> previous = new HashMap<>();
        for (String key : keys) {
            if (store.contains(key)) { previous.put(key, store.getAll().get(key)); }
        }
        try {
            ReaderPreferences options = new ReaderPreferences(context);
            options.saveTtsSettings(false, "engine.example", "voice.example",
                    "vi-VN", true, 1.5f, 1.3f, 0.4f);
            ReaderPreferences reloaded = new ReaderPreferences(context);
            assertFalse("Saved TTS off must not be silently reenabled", reloaded.isEnabled());
            assertEquals("engine.example", reloaded.getSpeechEngineName());
            assertEquals("voice.example", reloaded.getSpeechVoiceName());
            assertEquals("vi-VN", reloaded.getSpeechLanguageTag());
            assertTrue(reloaded.permitsNetworkVoice());
            assertEquals(1.5f, reloaded.getRate(), 0.001f);
            assertEquals(1.3f, reloaded.getPitch(), 0.001f);
            assertEquals(0.4f, reloaded.getVolume(), 0.001f);
            reloaded.saveTtsSettings(true, null, null, "vi-VN",
                    false, 1.0f, 1.0f, 1.0f);
            assertTrue(new ReaderPreferences(context).isEnabled());
            assertFalse(new ReaderPreferences(context).permitsNetworkVoice());
        } finally {
            SharedPreferences.Editor restore = store.edit();
            for (String key : keys) {
                restore.remove(key);
                if (!previous.containsKey(key)) { continue; }
                Object value = previous.get(key);
                if (value instanceof String) {
                    restore.putString(key, (String) value);
                } else if (value instanceof Boolean) {
                    restore.putBoolean(key, (Boolean) value);
                } else if (value instanceof Float) {
                    restore.putFloat(key, (Float) value);
                } else if (value instanceof Integer) {
                    restore.putInt(key, (Integer) value);
                } else if (value instanceof Long) {
                    restore.putLong(key, (Long) value);
                }
            }
            restore.commit();
        }
    }
}

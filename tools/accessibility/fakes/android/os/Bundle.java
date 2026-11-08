package android.os;

import java.util.HashMap;
import java.util.Map;

/** Narrow Android Bundle fake for TTS speak parameter tests. */
public final class Bundle {
    private final Map<String, Float> floats = new HashMap<>();

    public void putFloat(String key, float value) {
        floats.put(key, value);
    }

    public float getFloat(String key) {
        Float value = floats.get(key);
        return value == null ? 0.0f : value;
    }
}

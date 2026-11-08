package com.cleverraven.cataclysmdda;

import android.content.Context;
import android.content.SharedPreferences;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.Map;

/**
 * Persistent, one-to-one gameplay shortcut bindings. The latest assignment
 * takes precedence: the displaced target loses its custom binding and
 * automatically falls back to the built-in behavior of the game.
 */
final class ReaderGestureBindings {
    private static final String STORE = "internal_reader";
    private static final String KEY = "world_gesture_bindings_v1";

    static final String[] IDS = {
            "swipe_up",
            "swipe_down",
            "swipe_left",
            "swipe_right",
            "swipe_left_right",
            "swipe_right_left",
            "swipe_up_down",
            "swipe_down_up",
            "swipe_left_up",
            "swipe_left_down",
            "swipe_right_up",
            "swipe_right_down",
            "swipe_up_left",
            "swipe_up_right",
            "swipe_down_left",
            "swipe_down_right",
            "tap_2_1",
            "tap_2_2",
            "tap_2_3",
            "tap_3_1",
            "tap_3_2",
            "tap_3_3",
            "swipe_3_up",
            "swipe_3_down",
            "swipe_3_left",
            "swipe_3_right",
            "swipe_4_up",
            "swipe_4_down",
            "swipe_4_left",
            "swipe_4_right",
            "tap_4_1",
            "tap_4_2",
            "tap_4_3",
            "hold_1",
            "hold_2",
            "hold_3"
    };

    static final int[] LABELS = {
            R.string.a11yBindSwipeUp,
            R.string.a11yBindSwipeDown,
            R.string.a11yBindSwipeLeft,
            R.string.a11yBindSwipeRight,
            R.string.a11yBindSwipeLeftRight,
            R.string.a11yBindSwipeRightLeft,
            R.string.a11yBindSwipeUpDown,
            R.string.a11yBindSwipeDownUp,
            R.string.a11yBindSwipeLeftUp,
            R.string.a11yBindSwipeLeftDown,
            R.string.a11yBindSwipeRightUp,
            R.string.a11yBindSwipeRightDown,
            R.string.a11yBindSwipeUpLeft,
            R.string.a11yBindSwipeUpRight,
            R.string.a11yBindSwipeDownLeft,
            R.string.a11yBindSwipeDownRight,
            R.string.a11yBindTap21,
            R.string.a11yBindTap22,
            R.string.a11yBindTap23,
            R.string.a11yBindTap31,
            R.string.a11yBindTap32,
            R.string.a11yBindTap33,
            R.string.a11yBindSwipe3Up,
            R.string.a11yBindSwipe3Down,
            R.string.a11yBindSwipe3Left,
            R.string.a11yBindSwipe3Right,
            R.string.a11yBindSwipe4Up,
            R.string.a11yBindSwipe4Down,
            R.string.a11yBindSwipe4Left,
            R.string.a11yBindSwipe4Right,
            R.string.a11yBindTap41,
            R.string.a11yBindTap42,
            R.string.a11yBindTap43,
            R.string.a11yBindHold1,
            R.string.a11yBindHold2,
            R.string.a11yBindHold3
    };

    static final class Assignment {
        final String displacedTarget;
        final String displacedGesture;

        Assignment(String displacedTarget, String displacedGesture) {
            this.displacedTarget = displacedTarget;
            this.displacedGesture = displacedGesture;
        }
    }

    private final SharedPreferences store;

    ReaderGestureBindings(Context context) {
        store = context.getApplicationContext().getSharedPreferences(STORE, Context.MODE_PRIVATE);
    }

    static int indexOf(String gesture) {
        for (int i = 0; i < IDS.length; i++) {
            if (IDS[i].equals(gesture)) { return i; }
        }
        return -1;
    }

    static String idForSwipe(int fingers, String directions) {
        if (fingers == 1) {
            String result = "swipe_" + directions;
            return indexOf(result) >= 0 ? result : null;
        }
        String result = "swipe_" + fingers + "_" + directions;
        return indexOf(result) >= 0 ? result : null;
    }

    static String idForTap(int fingers, int count) {
        String result = "tap_" + fingers + "_" + count;
        return indexOf(result) >= 0 ? result : null;
    }

    static String idForHold(int fingers) {
        return fingers >= 1 && fingers <= 3 ? "hold_" + fingers : null;
    }

    private LinkedHashMap<String, String> readBindings() {
        LinkedHashMap<String, String> result = new LinkedHashMap<>();
        String saved = store.getString(KEY, "{}");
        JSONObject data;
        try { data = new JSONObject(saved); }
        catch (JSONException bad) { return result; }
        for (String gesture : IDS) {
            String target = data.optString(gesture, "");
            if (!target.isEmpty() && target.length() <= 256
                    && (target.startsWith("action:") || target.startsWith("hub:"))
                    && !result.containsValue(target)) {
                result.put(gesture, target);
            }
        }
        return result;
    }

    synchronized String targetFor(String gesture) {
        if (indexOf(gesture) < 0) { return null; }
        return readBindings().get(gesture);
    }

    synchronized String gestureFor(String target) {
        if (target == null) { return null; }
        for (Map.Entry<String, String> entry : readBindings().entrySet()) {
            if (target.equals(entry.getValue())) { return entry.getKey(); }
        }
        return null;
    }

    /**
     * Assign atomically. Existing binding on this gesture is displaced;
     * any old custom binding for the target is cleared. No duplicate wins.
     */
    synchronized Assignment assign(String gesture, String target) {
        if (indexOf(gesture) < 0 || target == null || target.length() > 256 ||
                !(target.startsWith("action:") || target.startsWith("hub:"))) {
            throw new IllegalArgumentException("Unknown gesture or target");
        }
        LinkedHashMap<String, String> mapping = readBindings();
        String displacedTarget = mapping.get(gesture);
        String displacedGesture = null;
        for (Map.Entry<String, String> entry : mapping.entrySet()) {
            if (target.equals(entry.getValue()) && !gesture.equals(entry.getKey())) {
                displacedGesture = entry.getKey();
                break;
            }
        }
        if (displacedGesture != null) { mapping.remove(displacedGesture); }
        mapping.put(gesture, target);
        writeBindings(mapping);
        return new Assignment(displacedTarget != null && !displacedTarget.equals(target)
                ? displacedTarget : null, displacedGesture);
    }

    synchronized void restoreDefault(String target) {
        if (target == null) { return; }
        LinkedHashMap<String, String> mapping = readBindings();
        mapping.values().removeAll(Collections.singleton(target));
        writeBindings(mapping);
    }

    private void writeBindings(Map<String, String> mapping) {
        JSONObject json = new JSONObject();
        for (String gesture : IDS) {
            String target = mapping.get(gesture);
            if (target != null) {
                try { json.put(gesture, target); }
                catch (JSONException impossible) { throw new IllegalStateException(impossible); }
            }
        }
        store.edit().putString(KEY, json.toString()).apply();
    }
}

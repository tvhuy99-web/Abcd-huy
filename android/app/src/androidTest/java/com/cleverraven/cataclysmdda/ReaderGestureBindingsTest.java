package com.cleverraven.cataclysmdda;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.fail;

import android.content.Context;
import android.content.SharedPreferences;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.Arrays;
import java.util.List;

/** Device-level contracts for shortcuts and swipe classifier (compilation in CI). */
@RunWith(AndroidJUnit4.class)
public final class ReaderGestureBindingsTest {
    private static final String STORE = "internal_reader";
    private static final String KEY = "world_gesture_bindings_v1";

    @Test
    public void newestAssignmentWinsAndOldActionRevertsToDefault() {
        Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        SharedPreferences store = context.getSharedPreferences(STORE, Context.MODE_PRIVATE);
        String old = store.getString(KEY, null);
        try {
            store.edit().remove(KEY).commit();
            ReaderGestureBindings bindings = new ReaderGestureBindings(context);
            assertEquals(36, ReaderGestureBindings.IDS.length);
            assertEquals(36, ReaderGestureBindings.LABELS.length);
            assertEquals(26, ReaderGestureBindings.CUSTOM_IDS.length);
            assertEquals(26, ReaderGestureBindings.CUSTOM_LABELS.length);
            assertTrue(ReaderGestureBindings.isReserved("tap_3_2"));
            assertTrue(ReaderGestureBindings.isReserved("swipe_up"));
            assertTrue(ReaderGestureBindings.isReserved("hold_1"));
            assertFalse(ReaderGestureBindings.isReserved("tap_3_3"));

            bindings.assign("tap_3_3", "action:open");
            assertEquals("action:open", bindings.targetFor("tap_3_3"));

            // Newest wins the gesture; the former action has no custom mapping.
            ReaderGestureBindings.Assignment conflict = bindings.assign(
                    "tap_3_3", "action:fire");
            assertEquals("action:open", conflict.displacedTarget);
            assertNull(bindings.gestureFor("action:open"));
            assertEquals("action:fire", bindings.targetFor("tap_3_3"));

            // Reassigning an action also frees its previous gesture.
            ReaderGestureBindings.Assignment move = bindings.assign(
                    "swipe_3_up", "action:fire");
            assertEquals("tap_3_3", move.displacedGesture);
            assertNull(bindings.targetFor("tap_3_3"));
            assertEquals("action:fire", bindings.targetFor("swipe_3_up"));

            // Persist across instances and restore original behavior.
            assertEquals("swipe_3_up",
                    new ReaderGestureBindings(context).gestureFor("action:fire"));
            bindings.restoreDefault("action:fire");
            assertNull(bindings.gestureFor("action:fire"));
            // Holds share the same newest-assignment-wins persistence.
            bindings.assign("hold_2", "action:open");
            assertEquals("action:open", bindings.targetFor("hold_2"));
            ReaderGestureBindings.Assignment replaced = bindings.assign(
                    "hold_3", "action:open");
            assertEquals("hold_2", replaced.displacedGesture);
            assertNull(bindings.targetFor("hold_2"));
            assertEquals("hold_3", bindings.gestureFor("action:open"));

            // Fixed gameplay gestures cannot be remapped by this storage API.
            for (String locked : new String[]{"swipe_up", "swipe_down",
                    "swipe_left", "swipe_right", "tap_2_1", "tap_2_2",
                    "tap_2_3", "tap_3_1", "tap_3_2", "hold_1"}) {
                assertNull(bindings.targetFor(locked));
                assertEquals(-1, ReaderGestureBindings.customIndexOf(locked));
                try {
                    bindings.assign(locked, "action:open");
                    fail("Should reject reserved gesture: " + locked);
                } catch (IllegalArgumentException expected) {
                    // The original built-in action remains available.
                }
            }
            // A pre-upgrade binding is discarded, never activated.
            store.edit().putString(KEY, "{\"tap_3_2\":\"action:fire\","
                    + "\"swipe_3_up\":\"action:open\"}").commit();
            ReaderGestureBindings migrated = new ReaderGestureBindings(context);
            assertNull(migrated.targetFor("tap_3_2"));
            assertEquals("action:open", migrated.targetFor("swipe_3_up"));
        } finally {
            SharedPreferences.Editor edit = store.edit();
            if (old == null) { edit.remove(KEY); }
            else { edit.putString(KEY, old); }
            edit.commit();
        }
    }

    @Test
    public void classifiesAllTwoStrokeDirectionsWithoutDiagonalMovement() {
        List<float[]> leftRight = Arrays.asList(
                new float[] { 0, 0 }, new float[] { -90, 0 }, new float[] { 0, 0 });
        assertEquals("swipe_left_right",
                ReaderGestureSurface.identifyOneFingerSwipe(leftRight, 48));

        List<float[]> leftUp = Arrays.asList(
                new float[] { 0, 0 }, new float[] { -90, 0 },
                new float[] { -90, -90 });
        assertEquals("swipe_left_up",
                ReaderGestureSurface.identifyOneFingerSwipe(leftUp, 48));

        List<float[]> simple = Arrays.asList(
                new float[] { 0, 0 }, new float[] { 0, -90 });
        assertEquals("swipe_up",
                ReaderGestureSurface.identifyOneFingerSwipe(simple, 48));

        List<float[]> diagonal = Arrays.asList(
                new float[] { 0, 0 }, new float[] { 40, 40 },
                new float[] { 90, 90 });
        assertNull(ReaderGestureSurface.identifyOneFingerSwipe(diagonal, 48));

        assertEquals("tap_4_3", ReaderGestureBindings.idForTap(4, 3));
        assertEquals("hold_1", ReaderGestureBindings.idForHold(1));
        assertEquals("hold_2", ReaderGestureBindings.idForHold(2));
        assertEquals("hold_3", ReaderGestureBindings.idForHold(3));
        assertNull(ReaderGestureBindings.idForHold(4));
        assertEquals("swipe_4_left", ReaderGestureBindings.idForSwipe(4, "left"));
        assertNull(ReaderGestureBindings.idForTap(5, 1));
        assertTrue(ReaderGestureBindings.indexOf("tap_2_1") >= 0);
    }
}

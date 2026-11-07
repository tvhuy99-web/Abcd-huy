package com.cleverraven.cataclysmdda;

import android.content.Context;
import android.content.Intent;
import android.os.SystemClock;
import android.view.MotionEvent;

import androidx.test.core.app.ActivityScenario;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import org.junit.Test;
import org.junit.runner.RunWith;

import static org.junit.Assert.assertEquals;

/** Exercise the real gesture dispatcher with synthetic two-finger touch events. */
@RunWith(AndroidJUnit4.class)
public final class GameMenuGestureDispatchTest {
    private static final class Actions implements ReaderGestureSurface.Listener {
        boolean playing;
        int backs;
        int scans;
        int sliderChanges;
        int actionMenus;
        int activations;

        @Override public boolean usesTouchExploration() { return false; }
        @Override public String gestureContext() { return playing ? "world" : "menu"; }
        @Override public boolean leftHandedNavigation() { return false; }
        @Override public int horizontalSwipeAction() {
            return ReaderPreferences.GESTURE_ACTION_NAVIGATE;
        }
        @Override public int verticalSwipeAction() {
            return ReaderPreferences.GESTURE_ACTION_NAVIGATE;
        }
        @Override public float swipeDistancePx() { return 48.0f; }
        @Override public long swipeDurationLimitMs() { return 750L; }
        @Override public void navigate(int delta) { }
        @Override public boolean adjustFocusedSetting(int delta) {
            sliderChanges += delta;
            return true;
        }
        @Override public void goBack() { backs++; }
        @Override public boolean movementPracticeEnabled() { return false; }
        @Override public void practiceMovementDirection(int dx, int dy) { }
        @Override public boolean worldMovementEnabled() { return false; }
        @Override public void moveDirection(int dx, int dy) { }
        @Override public boolean directWorldGesturesEnabled() { return playing; }
        @Override public void scanDirection(int dx, int dy) {
            if (dx == 0 && dy == 1) { scans++; }
        }
        @Override public void twoFingerTap(int count) { }
        @Override public void threeFingerTap() { }
        @Override public void openActionMenu() { actionMenus++; }
        @Override public boolean handleCustomGesture(String id) { return false; }
        @Override public void activate() { activations++; }
        @Override public void detail() { }
        @Override public void readCurrent() { }
        @Override public void stopSpeech() { }
    }

    private static void touch(ReaderGestureSurface surface, long downTime,
                              long eventTime, int action, float y0, float y1,
                              boolean twoPointers) {
        int count = twoPointers ? 2 : 1;
        MotionEvent.PointerProperties[] properties =
                new MotionEvent.PointerProperties[count];
        MotionEvent.PointerCoords[] coords = new MotionEvent.PointerCoords[count];
        for (int i = 0; i < count; i++) {
            properties[i] = new MotionEvent.PointerProperties();
            properties[i].id = i;
            properties[i].toolType = MotionEvent.TOOL_TYPE_FINGER;
            coords[i] = new MotionEvent.PointerCoords();
            coords[i].x = i == 0 ? 50 : 110;
            coords[i].y = i == 0 ? y0 : y1;
            coords[i].pressure = 1.0f;
            coords[i].size = 1.0f;
        }
        MotionEvent event = MotionEvent.obtain(downTime, eventTime,
                action, count, properties, coords, 0, 0, 1.0f, 1.0f,
                0, 0, 0, 0);
        surface.dispatchTouchEvent(event);
        event.recycle();
    }

    private static void swipeTwoDown(ReaderGestureSurface surface) {
        long start = SystemClock.uptimeMillis();
        touch(surface, start, start, MotionEvent.ACTION_DOWN, 100, 0, false);
        touch(surface, start, start + 30,
                MotionEvent.ACTION_POINTER_DOWN | (1 << MotionEvent.ACTION_POINTER_INDEX_SHIFT),
                100, 100, true);
        touch(surface, start, start + 75, MotionEvent.ACTION_MOVE, 200, 210, true);
        touch(surface, start, start + 100,
                MotionEvent.ACTION_POINTER_UP | (1 << MotionEvent.ACTION_POINTER_INDEX_SHIFT),
                200, 210, true);
        touch(surface, start, start + 130, MotionEvent.ACTION_UP, 200, 0, false);
    }

    @Test
    public void tripleTapOpensActionsWithoutDoubleTapActivation() {
        Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        Intent intent = new Intent(context, ReaderSetupActivity.class)
                .putExtra(ReaderSetupActivity.EXTRA_DISABLE_SELF_VOICE, true)
                .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        try (ActivityScenario<ReaderSetupActivity> scenario = ActivityScenario.launch(intent)) {
            scenario.onActivity(activity -> {
                Actions actions = new Actions();
                actions.playing = true;
                ReaderGestureSurface surface = new ReaderGestureSurface(activity, actions);
                activity.setContentView(surface);
                long start = SystemClock.uptimeMillis();
                for (int i = 0; i < 3; i++) {
                    long down = start + i * 95L;
                    touch(surface, down, down, MotionEvent.ACTION_DOWN, 60, 0, false);
                    touch(surface, down, down + 35,
                            MotionEvent.ACTION_UP, 60, 0, false);
                }
                assertEquals("Triple tap opens the native action menu", 1, actions.actionMenus);
                assertEquals("A third tap never activates the second-tap command",
                        0, actions.activations);
            });
        }
    }

    @Test
    public void downSwipeGoesBackOnMenusAndScansOnlyWhilePlaying() {
        Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        Intent intent = new Intent(context, ReaderSetupActivity.class)
                .putExtra(ReaderSetupActivity.EXTRA_DISABLE_SELF_VOICE, true)
                .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        try (ActivityScenario<ReaderSetupActivity> scenario = ActivityScenario.launch(intent)) {
            scenario.onActivity(activity -> {
                Actions actions = new Actions();
                ReaderGestureSurface surface = new ReaderGestureSurface(activity, actions);
                activity.setContentView(surface);

                swipeTwoDown(surface);
                assertEquals(1, actions.backs);
                assertEquals(0, actions.scans);

                actions.playing = true;
                swipeTwoDown(surface);
                assertEquals(1, actions.backs);
                assertEquals(1, actions.scans);

                actions.playing = false;
                long start = SystemClock.uptimeMillis();
                touch(surface, start, start, MotionEvent.ACTION_DOWN, 100, 0, false);
                touch(surface, start, start + 80, MotionEvent.ACTION_UP, 0, 0, false);
                assertEquals(1, actions.sliderChanges);
            });
        }
    }
}

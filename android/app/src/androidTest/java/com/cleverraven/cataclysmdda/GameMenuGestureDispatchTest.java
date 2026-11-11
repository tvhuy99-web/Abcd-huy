package com.cleverraven.cataclysmdda;

import android.content.Context;
import android.content.Intent;
import android.os.SystemClock;
import android.view.MotionEvent;
import android.view.ViewConfiguration;

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
        boolean combatMode;
        boolean customThreeFingerSwipe;
        int combatMoves;
        String lastCombatDirection = "";
        int savedThreeFingerSwipes;
        int backs;
        int confirms;
        int contextMenus;
        int scans;
        int northScans;
        int sliderChanges;
        int actionMenus;
        int activations;
        int tripleFingerTaps;
        int characterDetailReads;
        int combatModeToggles;
        int fourFingerTaps;
        int fourFingerSwipes;
        int unassignedGestures;
        int oneFingerHolds;
        int twoFingerHolds;
        int threeFingerHolds;
        int details;

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
        @Override public void confirm() { confirms++; }
        @Override public void openContextActions() { contextMenus++; }
        @Override public boolean movementPracticeEnabled() { return false; }
        @Override public void practiceMovementDirection(int dx, int dy) { }
        @Override public boolean worldMovementEnabled() { return false; }
        @Override public void moveDirection(int dx, int dy) { }
        @Override public boolean directWorldGesturesEnabled() { return playing; }
        @Override public void scanDirection(int dx, int dy) {
            if (dx == 0 && dy == 1) { scans++; }
            if (dx == 0 && dy == -1) { northScans++; }
        }
        @Override public void twoFingerTap(int count) {
            if (count == 1) { details++; }
        }
        @Override public void threeFingerTap() { tripleFingerTaps++; }
        @Override public void characterDetails() { characterDetailReads++; }
        @Override public void toggleCombatMode() { combatModeToggles++; }
        @Override public void openActionMenu() { actionMenus++; }
        @Override public boolean handleCustomGesture(String id) {
            if ("swipe_3_right".equals(id) || "swipe_3_left".equals(id)
                    || "swipe_3_up".equals(id) || "swipe_3_down".equals(id)) {
                if (combatMode) {
                    combatMoves++;
                    lastCombatDirection = id;
                    return true;
                }
                if (customThreeFingerSwipe) { savedThreeFingerSwipes++; return true; }
            }
            if ("hold_1".equals(id)) { oneFingerHolds++; return true; }
            if ("hold_2".equals(id)) { twoFingerHolds++; return true; }
            if ("hold_3".equals(id)) { threeFingerHolds++; return true; }
            if ("tap_4_1".equals(id)) { fourFingerTaps++; return true; }
            if ("swipe_4_right".equals(id)) { fourFingerSwipes++; return true; }
            return false;
        }
        @Override public void unassignedMultiGesture(String id) {
            unassignedGestures++;
        }
        @Override public void activate() { activations++; }
        @Override public void detail() { details++; }
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

    private static void multiTouch(ReaderGestureSurface surface, long downTime,
                                   long eventTime, int action, int pointers,
                                   float offsetX) {
        multiTouch(surface, downTime, eventTime, action, pointers, offsetX, 0);
    }

    private static void multiTouch(ReaderGestureSurface surface, long downTime,
                                   long eventTime, int action, int pointers,
                                   float offsetX, float offsetY) {
        MotionEvent.PointerProperties[] properties =
                new MotionEvent.PointerProperties[pointers];
        MotionEvent.PointerCoords[] coords = new MotionEvent.PointerCoords[pointers];
        for (int i = 0; i < pointers; i++) {
            properties[i] = new MotionEvent.PointerProperties();
            // Deliberately non-contiguous IDs: pointer index is not identity.
            properties[i].id = 10 + i * 2;
            properties[i].toolType = MotionEvent.TOOL_TYPE_FINGER;
            coords[i] = new MotionEvent.PointerCoords();
            coords[i].x = 50 + i * 45 + offsetX;
            coords[i].y = 140 + offsetY;
            coords[i].pressure = 1.0f;
            coords[i].size = 1.0f;
        }
        MotionEvent event = MotionEvent.obtain(downTime, eventTime, action,
                pointers, properties, coords, 0, 0, 1.0f, 1.0f,
                0, 0, 0, 0);
        surface.dispatchTouchEvent(event);
        event.recycle();
    }

    private static void startMulti(ReaderGestureSurface surface, int count, long time) {
        multiTouch(surface, time, time, MotionEvent.ACTION_DOWN, 1, 0);
        for (int fingers = 2; fingers <= count; fingers++) {
            multiTouch(surface, time, time + fingers * 35L,
                    MotionEvent.ACTION_POINTER_DOWN
                            | ((fingers - 1) << MotionEvent.ACTION_POINTER_INDEX_SHIFT),
                    fingers, 0);
        }
    }

    private static void endMulti(ReaderGestureSurface surface, int count,
                                 long time, float dx) {
        endMulti(surface, count, time, dx, 0);
    }

    private static void endMulti(ReaderGestureSurface surface, int count,
                                 long time, float dx, float dy) {
        for (int fingers = count; fingers >= 2; fingers--) {
            multiTouch(surface, time, time + 180 + (count - fingers) * 20L,
                    MotionEvent.ACTION_POINTER_UP
                            | ((fingers - 1) << MotionEvent.ACTION_POINTER_INDEX_SHIFT),
                    fingers, dx, dy);
        }
        multiTouch(surface, time, time + 280, MotionEvent.ACTION_UP, 1, dx, dy);
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

    private static void swipeTwoUp(ReaderGestureSurface surface) {
        long start = SystemClock.uptimeMillis();
        touch(surface, start, start, MotionEvent.ACTION_DOWN, 210, 0, false);
        touch(surface, start, start + 30,
                MotionEvent.ACTION_POINTER_DOWN | (1 << MotionEvent.ACTION_POINTER_INDEX_SHIFT),
                210, 210, true);
        touch(surface, start, start + 75, MotionEvent.ACTION_MOVE, 110, 100, true);
        touch(surface, start, start + 100,
                MotionEvent.ACTION_POINTER_UP | (1 << MotionEvent.ACTION_POINTER_INDEX_SHIFT),
                110, 100, true);
        touch(surface, start, start + 130, MotionEvent.ACTION_UP, 110, 0, false);
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
    public void threeFingerSystemCancelRecoversOneTapOnly() throws Exception {
        Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        Intent intent = new Intent(context, ReaderSetupActivity.class)
                .putExtra(ReaderSetupActivity.EXTRA_DISABLE_SELF_VOICE, true)
                .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        final Actions actions = new Actions();
        final ReaderGestureSurface[] surface = new ReaderGestureSurface[1];
        actions.playing = true;
        try (ActivityScenario<ReaderSetupActivity> scenario = ActivityScenario.launch(intent)) {
            scenario.onActivity(activity -> {
                surface[0] = new ReaderGestureSurface(activity, actions);
                activity.setContentView(surface[0]);
                long start = SystemClock.uptimeMillis();
                startMulti(surface[0], 3, start);
                // Android delivered ACTION_CANCEL with the same three stable
                // pointer IDs: recover a tap instead of silently discarding it.
                multiTouch(surface[0], start, start + 145,
                        MotionEvent.ACTION_CANCEL, 3, 0);
            });
            Thread.sleep(ViewConfiguration.getDoubleTapTimeout() + 150L);
            scenario.onActivity(activity -> {
                assertEquals(1, actions.tripleFingerTaps);
                // A dragged system cancellation must never become a tap.
                long start = SystemClock.uptimeMillis();
                startMulti(surface[0], 3, start);
                multiTouch(surface[0], start, start + 115,
                        MotionEvent.ACTION_MOVE, 3, 100);
                multiTouch(surface[0], start, start + 145,
                        MotionEvent.ACTION_CANCEL, 3, 100);
            });
            Thread.sleep(ViewConfiguration.getDoubleTapTimeout() + 150L);
            scenario.onActivity(activity -> {
                assertEquals("Only the valid cancelled tap is recovered",
                        1, actions.tripleFingerTaps);
            });
        }
    }

    @Test
    public void doubleThreeFingerTapReadsDetailsWithoutOverridingOtherTaps() throws Exception {
        Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        Intent intent = new Intent(context, ReaderSetupActivity.class)
                .putExtra(ReaderSetupActivity.EXTRA_DISABLE_SELF_VOICE, true)
                .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        final Actions actions = new Actions();
        final ReaderGestureSurface[] surface = new ReaderGestureSurface[1];
        actions.playing = true;
        try (ActivityScenario<ReaderSetupActivity> scenario = ActivityScenario.launch(intent)) {
            scenario.onActivity(activity -> {
                surface[0] = new ReaderGestureSurface(activity, actions);
                activity.setContentView(surface[0]);
                long now = SystemClock.uptimeMillis();
                startMulti(surface[0], 3, now);
                endMulti(surface[0], 3, now, 0);
                startMulti(surface[0], 3, now + 290);
                endMulti(surface[0], 3, now + 290, 0);
            });
            Thread.sleep(ViewConfiguration.getDoubleTapTimeout() + 150L);
            scenario.onActivity(activity -> {
                assertEquals(1, actions.characterDetailReads);
                assertEquals(0, actions.tripleFingerTaps);
                assertEquals(0, actions.unassignedGestures);
            });
        }
    }

    @Test
    public void threeFingerTripleTapTogglesCombatNotUnassigned() throws Exception {
        Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        Intent intent = new Intent(context, ReaderSetupActivity.class)
                .putExtra(ReaderSetupActivity.EXTRA_DISABLE_SELF_VOICE, true)
                .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        final Actions actions = new Actions();
        final ReaderGestureSurface[] surface = new ReaderGestureSurface[1];
        actions.playing = true;
        try (ActivityScenario<ReaderSetupActivity> scenario = ActivityScenario.launch(intent)) {
            scenario.onActivity(activity -> {
                surface[0] = new ReaderGestureSurface(activity, actions);
                activity.setContentView(surface[0]);
                long now = SystemClock.uptimeMillis();
                for (int i = 0; i < 3; ++i) {
                    startMulti(surface[0], 3, now + i * 180);
                    endMulti(surface[0], 3, now + i * 180, 0);
                }
            });
            Thread.sleep(ViewConfiguration.getDoubleTapTimeout() + 150L);
            scenario.onActivity(activity -> {
                assertEquals(1, actions.combatModeToggles);
                assertEquals(0, actions.unassignedGestures);
                assertEquals(0, actions.characterDetailReads);
            });
        }
    }

    @Test
    public void fourFingerTapAndSwipeReachBindings() throws Exception {
        Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        Intent intent = new Intent(context, ReaderSetupActivity.class)
                .putExtra(ReaderSetupActivity.EXTRA_DISABLE_SELF_VOICE, true)
                .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        final Actions actions = new Actions();
        final ReaderGestureSurface[] surface = new ReaderGestureSurface[1];
        actions.playing = true;
        try (ActivityScenario<ReaderSetupActivity> scenario = ActivityScenario.launch(intent)) {
            scenario.onActivity(activity -> {
                surface[0] = new ReaderGestureSurface(activity, actions);
                activity.setContentView(surface[0]);
                long start = SystemClock.uptimeMillis();
                startMulti(surface[0], 4, start);
                endMulti(surface[0], 4, start, 0);
            });
            Thread.sleep(ViewConfiguration.getDoubleTapTimeout() + 150L);
            scenario.onActivity(activity -> {
                assertEquals(1, actions.fourFingerTaps);
                long start = SystemClock.uptimeMillis();
                startMulti(surface[0], 4, start);
                multiTouch(surface[0], start, start + 155,
                        MotionEvent.ACTION_MOVE, 4, 110);
                endMulti(surface[0], 4, start, 110);
                assertEquals(1, actions.fourFingerSwipes);
            });
        }
    }

    @Test
    public void unassignedThreeFingerSwipeIsNotSilentlyDropped() {
        Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        Intent intent = new Intent(context, ReaderSetupActivity.class)
                .putExtra(ReaderSetupActivity.EXTRA_DISABLE_SELF_VOICE, true)
                .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        final Actions actions = new Actions();
        actions.playing = true;
        try (ActivityScenario<ReaderSetupActivity> scenario = ActivityScenario.launch(intent)) {
            scenario.onActivity(activity -> {
                ReaderGestureSurface surface = new ReaderGestureSurface(activity, actions);
                activity.setContentView(surface);
                long start = SystemClock.uptimeMillis();
                startMulti(surface, 3, start);
                multiTouch(surface, start, start + 145,
                        MotionEvent.ACTION_MOVE, 3, 110);
                endMulti(surface, 3, start, 110);
                assertEquals(1, actions.unassignedGestures);
            });
        }
    }

    @Test
    public void threeFingerSwipeMovesOnlyInCombatAndRestoresCustomBindingOnMap() {
        Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        Intent intent = new Intent(context, ReaderSetupActivity.class)
                .putExtra(ReaderSetupActivity.EXTRA_DISABLE_SELF_VOICE, true)
                .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        final Actions actions = new Actions();
        actions.playing = true;
        actions.combatMode = true;
        try (ActivityScenario<ReaderSetupActivity> scenario = ActivityScenario.launch(intent)) {
            scenario.onActivity(activity -> {
                ReaderGestureSurface surface = new ReaderGestureSurface(activity, actions);
                activity.setContentView(surface);
                long first = SystemClock.uptimeMillis();
                startMulti(surface, 3, first);
                multiTouch(surface, first, first + 145,
                        MotionEvent.ACTION_MOVE, 3, 110);
                endMulti(surface, 3, first, 110);
                assertEquals("Combat swipe dispatches one normal map movement request",
                        1, actions.combatMoves);
                assertEquals(0, actions.unassignedGestures);
                actions.combatMode = false;
                actions.customThreeFingerSwipe = true;
                long second = SystemClock.uptimeMillis() + 400;
                startMulti(surface, 3, second);
                multiTouch(surface, second, second + 145,
                        MotionEvent.ACTION_MOVE, 3, 110);
                endMulti(surface, 3, second, 110);
                assertEquals("Map swipe uses its saved custom binding",
                        1, actions.savedThreeFingerSwipes);
                assertEquals("No combat movement after returning to map",
                        1, actions.combatMoves);
                assertEquals(0, actions.unassignedGestures);
            });
        }
    }

    @Test
    public void allFourThreeFingerCombatDirectionsAreDetectedWithoutWalkingOnMap() {
        Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        Intent intent = new Intent(context, ReaderSetupActivity.class)
                .putExtra(ReaderSetupActivity.EXTRA_DISABLE_SELF_VOICE, true)
                .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        final Actions actions = new Actions();
        actions.playing = true;
        actions.combatMode = true;
        try (ActivityScenario<ReaderSetupActivity> scenario = ActivityScenario.launch(intent)) {
            scenario.onActivity(activity -> {
                ReaderGestureSurface surface = new ReaderGestureSurface(activity, actions);
                activity.setContentView(surface);
                String[] ids = {
                    "swipe_3_up", "swipe_3_right", "swipe_3_down", "swipe_3_left"
                };
                float[] offsetsX = { 0, 120, 0, -120 };
                float[] offsetsY = { -120, 0, 120, 0 };
                for (int i = 0; i < ids.length; i++) {
                    long start = SystemClock.uptimeMillis() + i * 400L;
                    startMulti(surface, 3, start);
                    multiTouch(surface, start, start + 145,
                            MotionEvent.ACTION_MOVE, 3, offsetsX[i], offsetsY[i]);
                    endMulti(surface, 3, start, offsetsX[i], offsetsY[i]);
                    assertEquals("Every combat direction must reach its own native handler",
                            ids[i], actions.lastCombatDirection);
                    assertEquals(i + 1, actions.combatMoves);
                    assertEquals(0, actions.unassignedGestures);
                }
                actions.combatMode = false;
                actions.customThreeFingerSwipe = true;
                long mapStart = SystemClock.uptimeMillis() + 2200L;
                startMulti(surface, 3, mapStart);
                multiTouch(surface, mapStart, mapStart + 145,
                        MotionEvent.ACTION_MOVE, 3, 0, -120);
                endMulti(surface, 3, mapStart, 0, -120);
                assertEquals("Saved map gesture must resume outside combat",
                        1, actions.savedThreeFingerSwipes);
                assertEquals("No combat move after leaving combat mode", 4, actions.combatMoves);
            });
        }
    }

    @Test
    public void singleFingerHoldAlwaysOpensContextActions() throws Exception {
        Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        Intent intent = new Intent(context, ReaderSetupActivity.class)
                .putExtra(ReaderSetupActivity.EXTRA_DISABLE_SELF_VOICE, true)
                .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        final Actions actions = new Actions();
        final ReaderGestureSurface[] surface = new ReaderGestureSurface[1];
        final long[] down = new long[1];
        actions.playing = true;
        try (ActivityScenario<ReaderSetupActivity> scenario = ActivityScenario.launch(intent)) {
            scenario.onActivity(activity -> {
                surface[0] = new ReaderGestureSurface(activity, actions);
                activity.setContentView(surface[0]);
                down[0] = SystemClock.uptimeMillis();
                touch(surface[0], down[0], down[0], MotionEvent.ACTION_DOWN,
                        80, 0, false);
            });
            Thread.sleep(ViewConfiguration.getLongPressTimeout() + 180L);
            scenario.onActivity(activity -> {
                assertEquals(0, actions.oneFingerHolds);
                assertEquals(1, actions.contextMenus);
                assertEquals(0, actions.details);
                touch(surface[0], down[0], SystemClock.uptimeMillis(),
                        MotionEvent.ACTION_UP, 80, 0, false);
                actions.playing = false;
                down[0] = SystemClock.uptimeMillis();
                touch(surface[0], down[0], down[0], MotionEvent.ACTION_DOWN,
                        80, 0, false);
            });
            Thread.sleep(ViewConfiguration.getLongPressTimeout() + 180L);
            scenario.onActivity(activity -> {
                assertEquals(0, actions.oneFingerHolds);
                assertEquals(0, actions.details);
                assertEquals(2, actions.contextMenus);
                touch(surface[0], down[0], SystemClock.uptimeMillis(),
                        MotionEvent.ACTION_UP, 80, 0, false);
            });
        }
    }

    @Test
    public void twoAndThreeFingerHoldsFireOnceWithoutBecomingTaps()
            throws Exception {
        Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        Intent intent = new Intent(context, ReaderSetupActivity.class)
                .putExtra(ReaderSetupActivity.EXTRA_DISABLE_SELF_VOICE, true)
                .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        final Actions actions = new Actions();
        final ReaderGestureSurface[] surface = new ReaderGestureSurface[1];
        final long[] down = new long[1];
        actions.playing = true;
        try (ActivityScenario<ReaderSetupActivity> scenario = ActivityScenario.launch(intent)) {
            scenario.onActivity(activity -> {
                surface[0] = new ReaderGestureSurface(activity, actions);
                activity.setContentView(surface[0]);
                down[0] = SystemClock.uptimeMillis();
                startMulti(surface[0], 2, down[0]);
            });
            Thread.sleep(ViewConfiguration.getLongPressTimeout() + 180L);
            scenario.onActivity(activity -> {
                assertEquals(1, actions.twoFingerHolds);
                assertEquals(0, actions.threeFingerHolds);
                endMulti(surface[0], 2, down[0], 0);
                down[0] = SystemClock.uptimeMillis();
                startMulti(surface[0], 3, down[0]);
            });
            Thread.sleep(ViewConfiguration.getLongPressTimeout() + 180L);
            scenario.onActivity(activity -> {
                assertEquals(1, actions.twoFingerHolds);
                assertEquals(1, actions.threeFingerHolds);
                endMulti(surface[0], 3, down[0], 0);
                assertEquals("Holding is not a tap", 0, actions.tripleFingerTaps);
                assertEquals(0, actions.activations);
            });
        }
    }

    @Test
    public void movingFingersCancelsPendingHold() throws Exception {
        Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        Intent intent = new Intent(context, ReaderSetupActivity.class)
                .putExtra(ReaderSetupActivity.EXTRA_DISABLE_SELF_VOICE, true)
                .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        final Actions actions = new Actions();
        final ReaderGestureSurface[] surface = new ReaderGestureSurface[1];
        final long[] down = new long[1];
        actions.playing = true;
        try (ActivityScenario<ReaderSetupActivity> scenario = ActivityScenario.launch(intent)) {
            scenario.onActivity(activity -> {
                surface[0] = new ReaderGestureSurface(activity, actions);
                activity.setContentView(surface[0]);
                down[0] = SystemClock.uptimeMillis();
                startMulti(surface[0], 3, down[0]);
                multiTouch(surface[0], down[0], down[0] + 135,
                        MotionEvent.ACTION_MOVE, 3, 110);
            });
            Thread.sleep(ViewConfiguration.getLongPressTimeout() + 180L);
            scenario.onActivity(activity -> {
                assertEquals(0, actions.threeFingerHolds);
                endMulti(surface[0], 3, down[0], 110);
            });
        }
    }

    @Test
    public void upSwipeConfirmsOnlyInMenusAndScansInWorld() {
        Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        Intent intent = new Intent(context, ReaderSetupActivity.class)
                .putExtra(ReaderSetupActivity.EXTRA_DISABLE_SELF_VOICE, true)
                .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        try (ActivityScenario<ReaderSetupActivity> scenario = ActivityScenario.launch(intent)) {
            scenario.onActivity(activity -> {
                Actions actions = new Actions();
                ReaderGestureSurface surface = new ReaderGestureSurface(activity, actions);
                activity.setContentView(surface);
                swipeTwoUp(surface);
                assertEquals(1, actions.confirms);
                assertEquals(0, actions.northScans);
                actions.playing = true;
                swipeTwoUp(surface);
                assertEquals(1, actions.confirms);
                assertEquals(1, actions.northScans);
            });
        }
    }

    @Test
    public void twoFingerSingleTapReadsDetailInMenus() throws Exception {
        Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        Intent intent = new Intent(context, ReaderSetupActivity.class)
                .putExtra(ReaderSetupActivity.EXTRA_DISABLE_SELF_VOICE, true)
                .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        final Actions actions = new Actions();
        try (ActivityScenario<ReaderSetupActivity> scenario = ActivityScenario.launch(intent)) {
            scenario.onActivity(activity -> {
                ReaderGestureSurface surface = new ReaderGestureSurface(activity, actions);
                activity.setContentView(surface);
                long start = SystemClock.uptimeMillis();
                startMulti(surface, 2, start);
                endMulti(surface, 2, start, 0);
            });
            Thread.sleep(ViewConfiguration.getDoubleTapTimeout() + 150L);
            scenario.onActivity(activity -> {
                assertEquals(1, actions.details);
                assertEquals(0, actions.confirms);
                assertEquals(0, actions.backs);
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

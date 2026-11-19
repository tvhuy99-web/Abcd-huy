package com.cleverraven.cataclysmdda;

import android.content.Context;
import android.os.SystemClock;
import android.view.MotionEvent;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.lang.reflect.Field;
import java.lang.reflect.Proxy;
import java.util.concurrent.atomic.AtomicInteger;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;


@RunWith(AndroidJUnit4.class)
public final class UserActionSpeechInterruptionTest {
    @Test public void passiveNotificationsQueueUntilTheNextExplicitUserAction() {
        Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
            ReaderSpeechQueue queue = new ReaderSpeechQueue(context, state -> { });
            try {


                Field healthy = ReaderSpeechQueue.class.getDeclaredField("healthy");
                healthy.setAccessible(true);
                healthy.setBoolean(queue, true);

                queue.say("game-message:first", "Item A, first alert.", ReaderSpeechQueue.GAME_EVENT);
                queue.say("game-message:second", "Item A, second alert.", ReaderSpeechQueue.GAME_EVENT);
                queue.say("game-message:urgent", "Item A, danger.", ReaderSpeechQueue.URGENT);
                queue.say("old-focus", "Old focus label.", ReaderSpeechQueue.FOCUS);
                ReaderSpeechQueue.Stats before = queue.snapshotStats();
                assertTrue(before.speaking);
                assertEquals("Passive events and focus remain queued until the user acts",
                        3, before.pending);

                queue.interruptForUserAction();
                ReaderSpeechQueue.Stats after = queue.snapshotStats();
                assertFalse(after.speaking);
                assertEquals(0, after.pending);

                queue.say("game-message:next", "Item B only.", ReaderSpeechQueue.GAME_EVENT);
                ReaderSpeechQueue.Stats newAction = queue.snapshotStats();
                assertTrue(newAction.speaking);
                assertEquals(0, newAction.pending);
            } catch (ReflectiveOperationException error) {
                throw new AssertionError(error);
            } finally {
                queue.close();
            }
        });
    }

    @Test public void swipesInterruptOnRecognitionNotOnFingerDown() {
        Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        AtomicInteger actions = new AtomicInteger();
        AtomicInteger moves = new AtomicInteger();
        ReaderGestureSurface.Listener listener = (ReaderGestureSurface.Listener)
                Proxy.newProxyInstance(ReaderGestureSurface.Listener.class.getClassLoader(),
                        new Class<?>[] { ReaderGestureSurface.Listener.class },
                        (proxy, method, args) -> {
                            switch (method.getName()) {
                                case "gestureContext": return "test-menu";
                                case "swipeDistancePx": return 32.0f;
                                case "swipeDurationLimitMs": return 750L;
                                case "horizontalSwipeAction":
                                case "verticalSwipeAction": return ReaderPreferences.GESTURE_ACTION_NAVIGATE;
                                case "onUserAction": actions.incrementAndGet(); return null;
                                case "navigate": moves.incrementAndGet(); return null;
                                default:
                                    Class<?> type = method.getReturnType();
                                    if (type == boolean.class) { return false; }
                                    if (type == int.class) { return 0; }
                                    if (type == long.class) { return 0L; }
                                    if (type == float.class) { return 0.0f; }
                                    return null;
                            }
                        });
        InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
            ReaderGestureSurface surface = new ReaderGestureSurface(context, listener);
            long when = SystemClock.uptimeMillis();
            MotionEvent down = MotionEvent.obtain(when, when,
                    MotionEvent.ACTION_DOWN, 20.0f, 20.0f, 0);
            MotionEvent move = MotionEvent.obtain(when, when + 60,
                    MotionEvent.ACTION_MOVE, 150.0f, 20.0f, 0);
            MotionEvent up = MotionEvent.obtain(when, when + 120,
                    MotionEvent.ACTION_UP, 150.0f, 20.0f, 0);
            try {
                surface.dispatchTouchEvent(down);
                assertEquals("A touch start is not a user command", 0, actions.get());
                surface.dispatchTouchEvent(move);
                surface.dispatchTouchEvent(up);
                assertEquals("One recognized swipe interrupts once", 1, actions.get());
                assertEquals("The new focus action is still delivered", 1, moves.get());
            } finally {
                down.recycle();
                move.recycle();
                up.recycle();
                surface.cancelGesture();
            }
        });
    }
}

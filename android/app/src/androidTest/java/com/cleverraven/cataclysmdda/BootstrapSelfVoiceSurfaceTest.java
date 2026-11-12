package com.cleverraven.cataclysmdda;

import android.content.Context;
import android.os.SystemClock;
import android.view.MotionEvent;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.Spinner;
import android.widget.LinearLayout;

import androidx.test.core.app.ActivityScenario;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicReference;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

/** Source test for startup gesture ownership; not executed during the code-writing phase. */
@RunWith(AndroidJUnit4.class)
public class BootstrapSelfVoiceSurfaceTest {
    @Test
    public void doubleTapRunsTheFocusedStartupActionExactlyOnce() {
        AtomicInteger clicks = new AtomicInteger();
        try (ActivityScenario<GameSettingsActivity> scenario = launchSettings()) {
            scenario.onActivity(activity -> {
                Context context = activity;
                BootstrapSelfVoiceSurface surface = new BootstrapSelfVoiceSurface(context,
                        new BootstrapSelfVoiceSurface.Listener() {
                            @Override
                            public boolean isActive() { return true; }
                            @Override
                            public void speak(String id, String text) { }
                            @Override
                            public void stopSpeech() { }
                            @Override
                            public void goBack() { }
                            @Override
                            public boolean leftHandedNavigation() { return false; }
                            @Override
                            public float swipeDistancePx() { return 48; }
                            @Override
                            public long swipeDurationLimitMs() { return 750; }
                        });
                LinearLayout content = new LinearLayout(context);
                Button action = new Button(context);
                action.setText("Start");
                action.setOnClickListener(view -> clicks.incrementAndGet());
                content.addView(action);
                activity.setContentView(surface);
                surface.setContent(content);

                long firstDown = SystemClock.uptimeMillis();
                dispatch(surface, MotionEvent.ACTION_DOWN, firstDown, firstDown);
                dispatch(surface, MotionEvent.ACTION_UP, firstDown, firstDown + 40);
                long secondDown = firstDown + 100;
                dispatch(surface, MotionEvent.ACTION_DOWN, secondDown, secondDown);
                dispatch(surface, MotionEvent.ACTION_UP, secondDown, secondDown + 40);
                assertEquals(1, clicks.get());
            });
        }
    }

    @Test
    public void swipingRightReadsNextItemAndInterruptsPreviousFocus() {
        AtomicReference<String> spoken = new AtomicReference<>("");
        AtomicInteger interrupted = new AtomicInteger();
        try (ActivityScenario<GameSettingsActivity> scenario = launchSettings()) {
            scenario.onActivity(activity -> {
                BootstrapSelfVoiceSurface surface = new BootstrapSelfVoiceSurface(activity,
                        new BootstrapSelfVoiceSurface.Listener() {
                            @Override public boolean isActive() { return true; }
                            @Override public void speak(String id, String text) {
                                spoken.set(text);
                            }
                            @Override public void stopSpeech() { interrupted.incrementAndGet(); }
                            @Override public void goBack() { }
                            @Override public boolean leftHandedNavigation() { return true; }
                            @Override public float swipeDistancePx() { return 48; }
                            @Override public long swipeDurationLimitMs() { return 750; }
                        });
                LinearLayout content = new LinearLayout(activity);
                content.setOrientation(LinearLayout.VERTICAL);
                for (String label : new String[] { "First", "Second", "Third" }) {
                    Button button = new Button(activity);
                    button.setText(label);
                    content.addView(button);
                }
                activity.setContentView(surface);
                surface.setContent(content);
                surface.speakFocused();
                assertTrue(spoken.get().contains("First"));
                assertTrue(spoken.get().contains("1"));
                swipe(surface, 15, 235);
                assertTrue(spoken.get().contains("Second"));
                assertTrue(spoken.get().contains("2"));
                swipe(surface, 235, 15);
                assertTrue(spoken.get().contains("First"));
                assertTrue(interrupted.get() >= 3);
            });
        }
    }

    @Test
    public void doubleTapOnVoiceSelectorOpensNavigableChoicesInsteadOfCycling() {
        try (ActivityScenario<GameSettingsActivity> scenario = launchSettings()) {
            scenario.onActivity(activity -> {
                BootstrapSelfVoiceSurface surface = new BootstrapSelfVoiceSurface(activity,
                        new BootstrapSelfVoiceSurface.Listener() {
                            @Override public boolean isActive() { return true; }
                            @Override public void speak(String id, String text) { }
                            @Override public void stopSpeech() { }
                            @Override public void goBack() { }
                            @Override public boolean leftHandedNavigation() { return false; }
                            @Override public float swipeDistancePx() { return 48; }
                            @Override public long swipeDurationLimitMs() { return 750; }
                        });
                LinearLayout content = new LinearLayout(activity);
                Spinner spinner = new Spinner(activity);
                spinner.setPrompt("Voice engine");
                spinner.setAdapter(new ArrayAdapter<>(activity,
                        android.R.layout.simple_spinner_item,
                        new String[] { "Default voice", "Vietnamese voice" }));
                content.addView(spinner);
                activity.setContentView(surface);
                surface.setContent(content);
                doubleTap(surface);
                // Focus in the chooser starts on the currently selected voice.
                swipe(surface, 15, 235);
                doubleTap(surface);
                assertEquals(1, spinner.getSelectedItemPosition());
                assertTrue(!surface.dismissChoiceList());
            });
        }
    }

    private void doubleTap(BootstrapSelfVoiceSurface surface) {
        long time = SystemClock.uptimeMillis();
        dispatch(surface, MotionEvent.ACTION_DOWN, time, time);
        dispatch(surface, MotionEvent.ACTION_UP, time, time + 40);
        dispatch(surface, MotionEvent.ACTION_DOWN, time + 100, time + 100);
        dispatch(surface, MotionEvent.ACTION_UP, time + 100, time + 140);
    }

    private void swipe(BootstrapSelfVoiceSurface surface, float start, float end) {
        long time = SystemClock.uptimeMillis();
        float mid = (start + end) / 2;
        touch(surface, MotionEvent.ACTION_DOWN, time, time, start);
        touch(surface, MotionEvent.ACTION_MOVE, time, time + 70, mid);
        touch(surface, MotionEvent.ACTION_UP, time, time + 140, end);
    }

    private void touch(BootstrapSelfVoiceSurface surface, int action,
                       long startTime, long eventTime, float x) {
        MotionEvent event = MotionEvent.obtain(startTime, eventTime, action, x, 20, 0);
        surface.dispatchTouchEvent(event);
        event.recycle();
    }

    private ActivityScenario<GameSettingsActivity> launchSettings() {
        android.content.Intent intent = new android.content.Intent(
                InstrumentationRegistry.getInstrumentation().getTargetContext(), GameSettingsActivity.class);
        return ActivityScenario.launch(intent);
    }

    private void dispatch(BootstrapSelfVoiceSurface surface, int action, long downTime, long eventTime) {
        MotionEvent event = MotionEvent.obtain(downTime, eventTime, action, 10, 10, 0);
        surface.dispatchTouchEvent(event);
        event.recycle();
    }
}

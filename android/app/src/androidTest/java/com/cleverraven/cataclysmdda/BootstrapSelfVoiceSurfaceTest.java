package com.cleverraven.cataclysmdda;

import android.content.Context;
import android.os.SystemClock;
import android.view.MotionEvent;
import android.widget.Button;
import android.widget.LinearLayout;

import androidx.test.core.app.ActivityScenario;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.concurrent.atomic.AtomicInteger;

import static org.junit.Assert.assertEquals;

/** Source test for startup gesture ownership; not executed during the code-writing phase. */
@RunWith(AndroidJUnit4.class)
public class BootstrapSelfVoiceSurfaceTest {
    @Test
    public void doubleTapRunsTheFocusedStartupActionExactlyOnce() {
        AtomicInteger clicks = new AtomicInteger();
        try (ActivityScenario<ReaderSetupActivity> scenario = launchSetup()) {
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

    private ActivityScenario<ReaderSetupActivity> launchSetup() {
        android.content.Intent intent = new android.content.Intent(
                InstrumentationRegistry.getInstrumentation().getTargetContext(), ReaderSetupActivity.class);
        intent.putExtra(ReaderSetupActivity.EXTRA_DISABLE_SELF_VOICE, true);
        return ActivityScenario.launch(intent);
    }

    private void dispatch(BootstrapSelfVoiceSurface surface, int action, long downTime, long eventTime) {
        MotionEvent event = MotionEvent.obtain(downTime, eventTime, action, 10, 10, 0);
        surface.dispatchTouchEvent(event);
        event.recycle();
    }
}

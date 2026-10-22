package com.cleverraven.cataclysmdda;

import android.os.SystemClock;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.TextView;

import androidx.lifecycle.Lifecycle;
import androidx.test.core.app.ActivityScenario;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.concurrent.atomic.AtomicBoolean;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

/** Native UI/lifecycle smoke checks. This does not certify audible Vietnamese,
 *  TalkBack gesture behavior, the JNI bridge, or gameplay accessibility.
 */
@RunWith(AndroidJUnit4.class)
public class ReaderSetupActivityTest {
    @Test
    public void controlsHaveLabelsAndOnlineVoiceIsOptIn() {
        try (ActivityScenario<ReaderSetupActivity> scenario = ActivityScenario.launch(ReaderSetupActivity.class)) {
            scenario.onActivity(activity -> {
                Button play = activity.findViewById(R.id.a11y_test_voice);
                Button stop = activity.findViewById(R.id.a11y_stop_voice);
                Button back = activity.findViewById(R.id.a11y_return);
                CheckBox online = activity.findViewById(R.id.a11y_network_voice);
                assertNotNull(play);
                assertTrue(play.getText().length() > 0);
                assertTrue(stop.getText().length() > 0);
                assertTrue(back.getText().length() > 0);
                assertFalse(online.isChecked());
                online.setChecked(true);
            });
            scenario.recreate();
            scenario.onActivity(activity -> {
                CheckBox online = activity.findViewById(R.id.a11y_network_voice);
                TextView status = activity.findViewById(R.id.a11y_speech_status);
                assertFalse("A recreated screen must not silently restore network voice consent", online.isChecked());
                assertEquals(activity.getString(R.string.a11ySpeechNew), status.getText().toString());
            });
        }
    }

    @Test
    public void stopAndRecreateDuringInitializationLeaveAUsableScreen() {
        try (ActivityScenario<ReaderSetupActivity> scenario = ActivityScenario.launch(ReaderSetupActivity.class)) {
            scenario.onActivity(activity -> {
                activity.findViewById(R.id.a11y_test_voice).performClick();
                activity.findViewById(R.id.a11y_stop_voice).performClick();
            });
            // The emulator may have no Vietnamese TTS data at all. Both ready
            // and a reported failure must release the retry button, without ANR.
            AtomicBoolean retryEnabled = new AtomicBoolean(false);
            long deadline = SystemClock.uptimeMillis() + 15000;
            while (!retryEnabled.get() && SystemClock.uptimeMillis() < deadline) {
                scenario.onActivity(activity -> retryEnabled.set(activity.findViewById(R.id.a11y_test_voice).isEnabled()));
                SystemClock.sleep(50);
            }
            assertTrue("Initialization/failure must finish or time out", retryEnabled.get());
            scenario.moveToState(Lifecycle.State.CREATED);
            scenario.moveToState(Lifecycle.State.RESUMED);
            scenario.recreate();
            scenario.onActivity(activity -> {
                assertTrue(activity.findViewById(R.id.a11y_test_voice).isEnabled());
                TextView status = activity.findViewById(R.id.a11y_speech_status);
                assertEquals(activity.getString(R.string.a11ySpeechNew), status.getText().toString());
                activity.findViewById(R.id.a11y_return).performClick();
            });
            InstrumentationRegistry.getInstrumentation().waitForIdleSync();
            long finishDeadline = SystemClock.uptimeMillis() + 5000;
            while (scenario.getState() != Lifecycle.State.DESTROYED
                    && SystemClock.uptimeMillis() < finishDeadline) {
                SystemClock.sleep(50);
            }
            assertEquals(Lifecycle.State.DESTROYED, scenario.getState());
        }
    }
}

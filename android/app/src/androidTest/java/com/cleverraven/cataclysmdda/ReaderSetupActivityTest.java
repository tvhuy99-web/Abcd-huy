package com.cleverraven.cataclysmdda;

import android.content.Intent;
import android.os.SystemClock;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.SeekBar;
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
    private ActivityScenario<ReaderSetupActivity> launchForUiTest() {
        Intent intent = new Intent(InstrumentationRegistry.getInstrumentation().getTargetContext(),
                                   ReaderSetupActivity.class);
        intent.putExtra(ReaderSetupActivity.EXTRA_DISABLE_SELF_VOICE, true);
        return ActivityScenario.launch(intent);
    }

    @Test
    public void controlsHaveLabelsAndOnlineVoiceIsOptIn() {
        try (ActivityScenario<ReaderSetupActivity> scenario = launchForUiTest()) {
            scenario.onActivity(activity -> {
                Button play = activity.findViewById(R.id.a11y_test_voice);
                Button stop = activity.findViewById(R.id.a11y_stop_voice);
                Button back = activity.findViewById(R.id.a11y_return);
                CheckBox online = activity.findViewById(R.id.a11y_network_voice);
                SeekBar rate = activity.findViewById(R.id.a11y_speech_rate);
                assertNotNull(play);
                assertTrue(play.getText().length() > 0);
                assertTrue(stop.getText().length() > 0);
                assertTrue(back.getText().length() > 0);
                assertFalse(online.isChecked());
                assertNotNull(rate);
                String originalRate = rate.getContentDescription().toString();
                rate.setProgress(rate.getProgress() == rate.getMax() ? 0 : rate.getProgress() + 1);
                assertFalse(originalRate.equals(rate.getContentDescription().toString()));
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
        try (ActivityScenario<ReaderSetupActivity> scenario = launchForUiTest()) {
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

    @Test
    public void gestureSettingsAreAdjustableAndCanBeRestored() {
        ReaderPreferences appPreferences = new ReaderPreferences(
                InstrumentationRegistry.getInstrumentation().getTargetContext());
        appPreferences.restoreGestureDefaults();
        try (ActivityScenario<ReaderSetupActivity> scenario = launchForUiTest()) {
            scenario.onActivity(activity -> {
                ReaderPreferences preferences = new ReaderPreferences(activity);
                CheckBox leftHanded = activity.findViewById(R.id.a11y_left_handed_navigation);
                SeekBar distance = activity.findViewById(R.id.a11y_swipe_distance);
                SeekBar duration = activity.findViewById(R.id.a11y_swipe_duration);
                Button horizontal = activity.findViewById(R.id.a11y_horizontal_swipe_action);
                Button vertical = activity.findViewById(R.id.a11y_vertical_swipe_action);

                assertNotNull(leftHanded);
                assertNotNull(distance);
                assertNotNull(duration);
                assertNotNull(horizontal);
                assertNotNull(vertical);
                assertEquals(1, distance.getProgress());
                assertEquals(1, duration.getProgress());
                assertEquals(ReaderPreferences.GESTURE_ACTION_NAVIGATE,
                             preferences.getHorizontalSwipeAction());
                assertEquals(ReaderPreferences.GESTURE_ACTION_NAVIGATE,
                             preferences.getVerticalSwipeAction());

                leftHanded.performClick();
                distance.setProgress(2);
                duration.setProgress(0);
                assertTrue(preferences.isLeftHandedNavigation());
                assertEquals(72, preferences.getSwipeDistanceDp());
                assertEquals(450, preferences.getSwipeDurationLimitMs());

                // The setup edits a draft; the previous mapping remains active
                // until the user explicitly starts the timed game trial.
                horizontal.performClick();
                assertTrue(horizontal.getText().toString().contains(
                                   activity.getString(R.string.a11yGestureActionDetail)));
                assertEquals(ReaderPreferences.GESTURE_ACTION_NAVIGATE,
                             preferences.getHorizontalSwipeAction());
                vertical.performClick();
                assertTrue(vertical.getText().toString().contains(
                                   activity.getString(R.string.a11yGestureActionNavigate)));

                activity.findViewById(R.id.a11y_restore_gesture_defaults).performClick();
                assertFalse(preferences.isLeftHandedNavigation());
                assertEquals(48, preferences.getSwipeDistanceDp());
                assertEquals(750, preferences.getSwipeDurationLimitMs());
                assertEquals(ReaderPreferences.GESTURE_ACTION_NAVIGATE,
                             preferences.getHorizontalSwipeAction());
                assertEquals(ReaderPreferences.GESTURE_ACTION_NAVIGATE,
                             preferences.getVerticalSwipeAction());
            });
        }
        appPreferences.beginGestureMapTrial(ReaderPreferences.GESTURE_ACTION_DETAIL,
                                            ReaderPreferences.GESTURE_ACTION_NAVIGATE);
        assertTrue(appPreferences.isGestureMapTrialActive());
        assertEquals(ReaderPreferences.GESTURE_ACTION_DETAIL, appPreferences.getHorizontalSwipeAction());
        assertTrue(appPreferences.confirmGestureMapTrial());
        assertFalse(appPreferences.isGestureMapTrialPending());
        assertEquals(ReaderPreferences.GESTURE_ACTION_DETAIL, appPreferences.getHorizontalSwipeAction());
        appPreferences.beginGestureMapTrial(ReaderPreferences.GESTURE_ACTION_NAVIGATE,
                                            ReaderPreferences.GESTURE_ACTION_READ_CURRENT);
        appPreferences.revertGestureMapTrial();
        assertEquals(ReaderPreferences.GESTURE_ACTION_DETAIL, appPreferences.getHorizontalSwipeAction());
        assertEquals(ReaderPreferences.GESTURE_ACTION_NAVIGATE, appPreferences.getVerticalSwipeAction());
        appPreferences.restoreGestureDefaults();
    }
}

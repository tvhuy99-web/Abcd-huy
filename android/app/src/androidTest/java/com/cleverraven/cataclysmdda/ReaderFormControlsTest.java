package com.cleverraven.cataclysmdda;

import android.content.Context;
import android.content.Intent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;

import androidx.test.core.app.ActivityScenario;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import org.junit.Test;
import org.junit.runner.RunWith;

import static org.junit.Assert.assertTrue;

/** Exercises the real controller's one-finger command path without loading the SDL engine. */
@RunWith(AndroidJUnit4.class)
public class ReaderFormControlsTest {
    private static final class AttachedGameActivity extends CataclysmDDA {
        AttachedGameActivity(Context context) { attachBaseContext(context); }
    }

    private static Button selectedButton(View view) {
        if (view instanceof Button && view.isSelected()) { return (Button) view; }
        if (view instanceof ViewGroup) {
            ViewGroup group = (ViewGroup) view;
            for (int i = 0; i < group.getChildCount(); i++) {
                Button found = selectedButton(group.getChildAt(i));
                if (found != null) { return found; }
            }
        }
        return null;
    }

    private static void chooseWithNavigation(GameReaderController controller,
                                             LinearLayout parent, String label) {
        controller.showControls();
        for (int i = 0; i < 40; i++) {
            Button selected = selectedButton(parent);
            if (selected != null && label.contentEquals(selected.getText())) {
                controller.activate();
                return;
            }
            controller.navigate(1);
        }
        throw new AssertionError("Cannot reach form command through gesture navigation: " + label);
    }

    @Test
    public void pendingDraftCanReachApplyAndDiscardWithoutTouchingNativeButtons() {
        Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        Intent intent = new Intent(context, ReaderSetupActivity.class)
                .putExtra(ReaderSetupActivity.EXTRA_DISABLE_SELF_VOICE, true)
                .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        GameReaderController[] controller = new GameReaderController[1];
        LinearLayout[] parent = new LinearLayout[1];
        try (ActivityScenario<ReaderSetupActivity> scenario = ActivityScenario.launch(intent)) {
            scenario.onActivity(activity -> {
                new ReaderPreferences(activity).confirmHeardAndEnable(false, 1.0f);
                parent[0] = new LinearLayout(activity);
                activity.setContentView(parent[0]);
                controller[0] = new GameReaderController(new AttachedGameActivity(activity), parent[0]);
                controller[0].onResume();
                controller[0].receive("{\"session\":\"1\",\"frame\":\"1\",\"generation\":\"1\","
                        + "\"revision\":\"1\",\"title\":\"Preferences\",\"focus\":\"field\","
                        + "\"nodes\":[{\"id\":\"field\",\"label\":\"Threshold\",\"fieldType\":\"int\","
                        + "\"appliedValue\":\"10\",\"draftValue\":\"20\",\"value\":\"10\","
                        + "\"fieldMin\":0,\"fieldMax\":100,\"fieldStep\":1,"
                        + "\"actions\":[\"focus\",\"set_value\",\"apply\",\"cancel_draft\"]}]}");
            });
            InstrumentationRegistry.getInstrumentation().waitForIdleSync();
            scenario.onActivity(activity -> {
                chooseWithNavigation(controller[0], parent[0], activity.getString(R.string.a11yReaderApplyValue));
                Button selected = selectedButton(parent[0]);
                assertTrue("Apply returns to the field", selected != null
                        && selected.getText().toString().contains("Threshold"));
                chooseWithNavigation(controller[0], parent[0], activity.getString(R.string.a11yReaderCancelDraft));
                selected = selectedButton(parent[0]);
                assertTrue("Discard returns to the field", selected != null
                        && selected.getText().toString().contains("Threshold"));
            });
        } finally {
            InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
                if (controller[0] != null) { controller[0].close(); }
                new ReaderPreferences(context).disable();
            });
        }
    }
}

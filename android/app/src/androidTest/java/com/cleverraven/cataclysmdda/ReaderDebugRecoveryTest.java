package com.cleverraven.cataclysmdda;

import android.content.Context;
import android.content.Intent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.test.core.app.ActivityScenario;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import org.junit.Test;
import org.junit.runner.RunWith;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

/** Only an actual SDL error may draw the self-voicing recovery controls. */
@RunWith(AndroidJUnit4.class)
public final class ReaderDebugRecoveryTest {
    private static final class AttachedGameActivity extends CataclysmDDA {
        AttachedGameActivity(Context context) { attachBaseContext(context); }
    }

    private static int buttonCount(View node) {
        int count = node instanceof Button ? 1 : 0;
        if (node instanceof ViewGroup) {
            ViewGroup group = (ViewGroup) node;
            for (int i = 0; i < group.getChildCount(); i++) {
                count += buttonCount(group.getChildAt(i));
            }
        }
        return count;
    }

    private static boolean containsReport(View node, String report) {
        if (node instanceof TextView
                && ((TextView) node).getText().toString().contains(report)) {
            return true;
        }
        if (node instanceof ViewGroup) {
            ViewGroup group = (ViewGroup) node;
            for (int i = 0; i < group.getChildCount(); i++) {
                if (containsReport(group.getChildAt(i), report)) {
                    return true;
                }
            }
        }
        return false;
    }

    @Test
    public void debugReportHasThreeActionsButNoPersistentDashboard() {
        Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        Intent intent = new Intent(context, AccessibilitySettingsActivity.class)
                .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        final GameReaderController[] reader = new GameReaderController[1];
        try (ActivityScenario<AccessibilitySettingsActivity> scenario = ActivityScenario.launch(intent)) {
            scenario.onActivity(activity -> {
                new ReaderPreferences(activity).saveTtsSettings(true, null, null,
                        ReaderPreferences.DEFAULT_SPEECH_LANGUAGE, false, 1.0f, 1.0f, 1.0f);
                LinearLayout layout = new LinearLayout(activity);
                activity.setContentView(layout);
                reader[0] = new GameReaderController(new AttachedGameActivity(activity), layout);
                reader[0].onResume();
                assertEquals("Normal game screen has no debug buttons", 0, buttonCount(layout));
                reader[0].showDebugPrompt("Stale game data detected at mods/Backrooms/modinfo.json");
                assertEquals("Three recovery actions for a native error", 3, buttonCount(layout));
                assertTrue("The full report is available as accessible text",
                        containsReport(layout, "mods/Backrooms/modinfo.json"));
                reader[0].navigate(1);
                reader[0].dismissDebugPrompt();
                assertEquals("Error panel disappears after native closes it", 0, buttonCount(layout));
                assertFalse("No original control dashboard remains",
                        containsReport(layout, "Stale game data detected"));
            });
        } finally {
            InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
                if (reader[0] != null) { reader[0].close(); }
                new ReaderPreferences(context).disable();
            });
        }
    }
}

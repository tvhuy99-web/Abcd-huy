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

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

/** The game reader must never paint the removed Android control dashboard. */
@RunWith(AndroidJUnit4.class)
public final class ReaderOverlayRemovalTest {
    private static final class AttachedGameActivity extends CataclysmDDA {
        AttachedGameActivity(Context context) { attachBaseContext(context); }
    }

    private static boolean hasButton(View view) {
        if (view instanceof Button) { return true; }
        if (view instanceof ViewGroup) {
            ViewGroup group = (ViewGroup) view;
            for (int i = 0; i < group.getChildCount(); i++) {
                if (hasButton(group.getChildAt(i))) { return true; }
            }
        }
        return false;
    }

    @Test
    public void startupAndBackHaveNoReaderControlButtons() {
        Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        Intent intent = new Intent(context, ReaderSetupActivity.class)
                .putExtra(ReaderSetupActivity.EXTRA_DISABLE_SELF_VOICE, true)
                .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        final GameReaderController[] controller = new GameReaderController[1];
        try (ActivityScenario<ReaderSetupActivity> scenario = ActivityScenario.launch(intent)) {
            scenario.onActivity(activity -> {
                new ReaderPreferences(activity).confirmHeardAndEnable(false, 1.0f);
                LinearLayout parent = new LinearLayout(activity);
                activity.setContentView(parent);
                controller[0] = new GameReaderController(new AttachedGameActivity(activity), parent);
                controller[0].onResume();
                assertTrue("Transparent gesture layer is retained", parent.getChildCount() > 0);
                assertFalse("There is no reader dashboard or child menu", hasButton(parent));
                controller[0].onBackPressed();
                assertFalse("Back cannot reopen a deleted reader menu", hasButton(parent));
            });
        } finally {
            InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
                if (controller[0] != null) { controller[0].close(); }
                new ReaderPreferences(context).disable();
            });
        }
    }
}

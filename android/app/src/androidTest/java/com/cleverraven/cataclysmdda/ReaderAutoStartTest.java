package com.cleverraven.cataclysmdda;

import android.content.Context;
import android.content.SharedPreferences;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import org.junit.Test;
import org.junit.runner.RunWith;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assume.assumeTrue;

/** Startup regression: prior voice testing must no longer gate the reader. */
@RunWith(AndroidJUnit4.class)
public class ReaderAutoStartTest {
    @Test
    public void upgradeAutoEnablesAndLaterExplicitDisableIsRespected() {
        assumeTrue(BuildConfig.FLAVOR.equals("accessibility"));
        Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        SharedPreferences values = context.getSharedPreferences("internal_reader", Context.MODE_PRIVATE);
        String migration = "auto_reader_default_20261107";
        boolean hadMigration = values.contains(migration);
        boolean oldMigration = values.getBoolean(migration, false);
        boolean hadEnabled = values.contains("enabled");
        boolean oldEnabled = values.getBoolean("enabled", false);
        boolean hadHeard = values.contains("heard_confirmation");
        boolean oldHeard = values.getBoolean("heard_confirmation", false);
        try {
            values.edit().remove(migration).putBoolean("enabled", false)
                    .putBoolean("heard_confirmation", false).commit();
            ReaderPreferences reader = new ReaderPreferences(context);
            assertTrue("No sample or manual confirmation should be required", reader.isEnabled());
            reader.disable();
            assertFalse("Explicit later disable should persist", new ReaderPreferences(context).isEnabled());
        } finally {
            SharedPreferences.Editor editor = values.edit();
            if (hadMigration) { editor.putBoolean(migration, oldMigration); }
            else { editor.remove(migration); }
            if (hadEnabled) { editor.putBoolean("enabled", oldEnabled); }
            else { editor.remove("enabled"); }
            if (hadHeard) { editor.putBoolean("heard_confirmation", oldHeard); }
            else { editor.remove("heard_confirmation"); }
            editor.commit();
        }
    }
}

package com.cleverraven.cataclysmdda;

import android.content.Context;
import android.content.SharedPreferences;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.HashMap;
import java.util.Map;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

/** Independently saved creature and item sonar, with legacy preference migration. */
@RunWith(AndroidJUnit4.class)
public final class DirectionalSonarPreferencesTest {
    @Test public void legacySingleSwitchMigratesWithoutChangingAnExistingChoice() {
        Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        SharedPreferences store = context.getSharedPreferences("internal_reader", Context.MODE_PRIVATE);
        String[] keys = { "directional_sonar", "sonar_creatures", "sonar_items",
                "auto_reader_default_20261107", "enabled" };
        Map<String, Object> previous = new HashMap<>();
        for (String key : keys) {
            if (store.contains(key)) { previous.put(key, store.getAll().get(key)); }
        }
        try {
            store.edit().remove("sonar_creatures").remove("sonar_items")
                    .putBoolean("directional_sonar", true).commit();
            ReaderPreferences prefs = new ReaderPreferences(context);
            assertTrue(prefs.isCreatureSonarEnabled());
            assertTrue(prefs.isItemSonarEnabled());

            prefs.setCreatureSonarEnabled(false);
            ReaderPreferences reloaded = new ReaderPreferences(context);
            assertFalse(reloaded.isCreatureSonarEnabled());
            assertTrue(reloaded.isItemSonarEnabled());
            assertTrue(reloaded.isDirectionalSonarEnabled());

            reloaded.setItemSonarEnabled(false);
            ReaderPreferences silent = new ReaderPreferences(context);
            assertFalse(silent.isCreatureSonarEnabled());
            assertFalse(silent.isItemSonarEnabled());
            assertFalse(silent.isDirectionalSonarEnabled());

            silent.setCreatureSonarEnabled(true);
            assertTrue(new ReaderPreferences(context).isCreatureSonarEnabled());
            assertFalse(new ReaderPreferences(context).isItemSonarEnabled());

            silent.setDirectionalSonarEnabled(false);
            assertFalse(silent.isCreatureSonarEnabled());
            assertFalse(silent.isItemSonarEnabled());
            silent.setDirectionalSonarEnabled(true);
            assertTrue(silent.isCreatureSonarEnabled());
            assertTrue(silent.isItemSonarEnabled());

            store.edit().remove("sonar_creatures").remove("sonar_items")
                    .putBoolean("directional_sonar", false).commit();
            ReaderPreferences oldDisabled = new ReaderPreferences(context);
            assertFalse(oldDisabled.isCreatureSonarEnabled());
            assertFalse(oldDisabled.isItemSonarEnabled());
        } finally {
            SharedPreferences.Editor restore = store.edit();
            for (String key : keys) {
                restore.remove(key);
                if (!previous.containsKey(key)) { continue; }
                Object value = previous.get(key);
                if (value instanceof Boolean) { restore.putBoolean(key, (Boolean) value); }
                else if (value instanceof Integer) { restore.putInt(key, (Integer) value); }
                else if (value instanceof String) { restore.putString(key, (String) value); }
            }
            restore.commit();
        }
    }
}

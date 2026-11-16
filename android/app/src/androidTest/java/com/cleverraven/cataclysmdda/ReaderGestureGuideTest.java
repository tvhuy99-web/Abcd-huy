package com.cleverraven.cataclysmdda;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.assertEquals;

import android.content.Context;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import org.json.JSONArray;
import org.json.JSONObject;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.List;

/** The same guide is reused without changing any gameplay bindings. */
@RunWith(AndroidJUnit4.class)
public final class ReaderGestureGuideTest {
    private static ReaderSnapshot screen(String category) throws Exception {
        JSONObject data = new JSONObject();
        data.put("session", 1);
        data.put("frame", 1);
        data.put("generation", 1);
        data.put("revision", 1);
        data.put("category", category);
        data.put("nodes", new JSONArray());
        return new ReaderSnapshot(data);
    }

    private static boolean mentions(List<String> lines, String needle) {
        for (String line : lines) {
            if (line.contains(needle)) { return true; }
        }
        return false;
    }

    @Test
    public void mapCombatDriveHaveDistinctGestureExplanations() throws Exception {
        Context c = InstrumentationRegistry.getInstrumentation().getTargetContext();
        ReaderSnapshot world = screen("DEFAULTMODE");
        List<String> map = ReaderGestureGuide.entries(c, world, false, false,
                false, false, false, false, null);
        List<String> combat = ReaderGestureGuide.entries(c, world, false, true,
                false, false, false, false, null);
        List<String> drive = ReaderGestureGuide.entries(c, world, true, false,
                false, false, false, false, null);
        assertTrue(mentions(map, c.getString(R.string.a11yGuideScanNorth)));
        assertFalse(mentions(map, c.getString(R.string.a11yGuideScanFront)));
        assertTrue(mentions(drive, c.getString(R.string.a11yGuideScanFront)));
        assertTrue(mentions(drive, c.getString(R.string.a11yGuideBrake)));
        assertFalse(mentions(drive, c.getString(R.string.a11yGuideScanNorth)));
        assertTrue(mentions(combat, c.getString(R.string.a11yGuideReload)));
        assertTrue(mentions(combat, c.getString(R.string.a11yGuideCombatEnemy)));
        assertFalse(mentions(combat, c.getString(R.string.a11yGuideScanNorth)));
    }

    @Test
    public void inventoryCraftingAndFallbackAreFocusableRows() throws Exception {
        Context c = InstrumentationRegistry.getInstrumentation().getTargetContext();
        List<String> inventory = ReaderGestureGuide.entries(c, screen("inventory"),
                false, false, false, false, false, false, null);
        List<String> crafting = ReaderGestureGuide.entries(c, screen("crafting"),
                false, false, false, false, false, false, null);
        List<String> unknown = ReaderGestureGuide.entries(c, screen("future_screen"),
                false, false, false, false, false, false, null);
        assertTrue(mentions(inventory, c.getString(R.string.a11yGuideBrowsePockets)));
        assertTrue(mentions(crafting, c.getString(R.string.a11yGuideBrowseRecipes)));
        assertTrue(mentions(unknown, c.getString(R.string.a11yGuideActivate)));
        assertFalse(unknown.isEmpty());
        for (String row : unknown) {
            assertTrue(row.contains(": "));
        }
    }

    @Test
    public void nativeHelpCommandIsLocalOnly() {
        ReaderSnapshot.Node node = ReaderSnapshot.Node.gestureGuide("Gesture guide");
        assertEquals("reader:gesture:guide", node.id);
        assertFalse(node.actions.contains("activate"));
        assertTrue(node.enabled);
    }
}

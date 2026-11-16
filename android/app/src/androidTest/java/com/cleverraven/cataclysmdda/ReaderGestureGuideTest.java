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
    public void allSixCreatorStepsExposeTheRealTwoFingerConfirmation() throws Exception {
        Context c = InstrumentationRegistry.getInstrumentation().getTargetContext();
        for (int step = 1; step <= 6; step++) {
            List<String> rows = ReaderGestureGuide.entries(c,
                    screen("character-creator-step-" + step), false, false,
                    false, false, false, false, null);
            assertTrue("Creator step " + step + " missing two-finger UP",
                    mentions(rows, c.getString(R.string.a11yGuideTwoUp)));
            assertTrue(mentions(rows, c.getString(step < 3
                    ? R.string.a11yGuideCreatorSelect : R.string.a11yGuideCreatorNextStep)));
            assertTrue(mentions(rows, c.getString(R.string.a11yGuideTwoDown)));
            assertTrue(mentions(rows, c.getString(R.string.a11yGuideRecentMessages)));
            assertFalse(mentions(rows, c.getString(R.string.a11yGuideScanNorth)));
        }
    }

    @Test
    public void creatorSummaryCalendarAndYesNoExplainTheirActualConfirm() throws Exception {
        Context c = InstrumentationRegistry.getInstrumentation().getTargetContext();
        String[][] screens = {
                { "CHARACTER_CREATION", c.getString(R.string.a11yGuideCreatorFinish) },
                { "character-creator-calendar", c.getString(R.string.a11yGuideCalendarConfirm) },
                { "YESNO", c.getString(R.string.a11yGuideAcceptYes) },
                { "YESNOQUIT", c.getString(R.string.a11yGuideAcceptYes) }
        };
        for (String[] pair : screens) {
            List<String> rows = ReaderGestureGuide.entries(c,
                    screen(pair[0]), false, false, false, false, false, false, null);
            assertTrue(pair[0] + " missing two-finger UP",
                    mentions(rows, c.getString(R.string.a11yGuideTwoUp)));
            assertTrue(pair[0] + " missing correct meaning", mentions(rows, pair[1]));
            assertTrue(mentions(rows, c.getString(R.string.a11yGuideGoBack)));
        }
    }

    @Test
    public void universalActionsArePresentOnceAndDoNotOverwriteWorldScans() throws Exception {
        Context c = InstrumentationRegistry.getInstrumentation().getTargetContext();
        String[] categories = { "MAIN_MENU", "inventory", "pickup-here",
                "crafting", "construction", "targeting", "surroundings",
                "overmap", "dialogue", "zones", "future_screen" };
        for (String category : categories) {
            List<String> rows = ReaderGestureGuide.entries(c, screen(category),
                    false, false, false, false, false, false, null);
            assertTrue(category, mentions(rows, c.getString(R.string.a11yGuideTwoUp)));
            assertTrue(category, mentions(rows, c.getString(R.string.a11yGuideGoBack)));
            assertTrue(category, mentions(rows, c.getString(R.string.a11yGuideRecentMessages)));
            assertTrue(category, mentions(rows, c.getString(R.string.a11yGuideStopSpeech)));
            java.util.HashSet<String> gestures = new java.util.HashSet<>();
            for (String row : rows) {
                String name = row.split(": ", 2)[0];
                assertTrue("Repeated gesture " + category + ": " + name, gestures.add(name));
            }
        }
        List<String> world = ReaderGestureGuide.entries(c,
                screen("DEFAULTMODE"), false, false, false, false, false, false, null);
        assertTrue(mentions(world, c.getString(R.string.a11yGuideScanNorth)));
        assertTrue(mentions(world, c.getString(R.string.a11yGuideOpenHub)));
        assertFalse(mentions(world, c.getString(R.string.a11yGuideConfirmSelection)));
        assertFalse(mentions(world, c.getString(R.string.a11yGuideGoBack)));
    }

    @Test
    public void editorAndDebugPromptsRetainDistinctControls() throws Exception {
        Context c = InstrumentationRegistry.getInstrumentation().getTargetContext();
        List<String> editor = ReaderGestureGuide.entries(c, null, false, false,
                false, false, true, false, null);
        List<String> debug = ReaderGestureGuide.entries(c, null, false, false,
                false, false, false, true, null);
        assertTrue(mentions(editor, c.getString(R.string.a11yGuideConfirmInput)));
        assertTrue(mentions(editor, c.getString(R.string.a11yGuideCancelInput)));
        assertFalse(mentions(debug, c.getString(R.string.a11yGuideConfirmSelection)));
        assertTrue(mentions(debug, c.getString(R.string.a11yGuideDebugContinue)));
    }

    @Test
    public void documentReaderOverridesUnderlyingInventoryGestures() throws Exception {
        Context c = InstrumentationRegistry.getInstrumentation().getTargetContext();
        List<String> document = ReaderGestureGuide.entries(c, screen("inventory"),
                false, false, false, false, false, false, null, true, 0);
        assertTrue(mentions(document, c.getString(R.string.a11yGuideBrowseParagraph)));
        assertTrue(mentions(document, c.getString(R.string.a11yGuideReadDocument)));
        assertTrue(mentions(document, c.getString(R.string.a11yGuideGoBack)));
        assertFalse(mentions(document, c.getString(R.string.a11yGuideBrowsePockets)));
        assertFalse(mentions(document, c.getString(R.string.a11yGuideConfirmSelection)));
    }

    @Test
    public void nestedPocketMenusExplainEachSubscreen() throws Exception {
        Context c = InstrumentationRegistry.getInstrumentation().getTargetContext();
        int[] meaning = { R.string.a11yGuideChoosePocketAction,
                R.string.a11yGuidePickPocket, R.string.a11yGuideChoosePocketItem };
        for (int stage = 1; stage <= 3; stage++) {
            List<String> rows = ReaderGestureGuide.entries(c, screen("inventory"),
                    false, false, false, false, false, false, null, false, stage);
            assertTrue(mentions(rows, c.getString(meaning[stage - 1])));
            assertTrue(mentions(rows, c.getString(R.string.a11yGuideTwoDown)));
            if (stage == 3) {
                assertTrue(mentions(rows, c.getString(R.string.a11yGuideConfirmPocket)));
            }
            assertFalse(mentions(rows, c.getString(R.string.a11yGuideBrowsePockets)));
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

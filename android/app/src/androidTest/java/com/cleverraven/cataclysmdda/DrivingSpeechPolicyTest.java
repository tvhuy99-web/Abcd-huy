package com.cleverraven.cataclysmdda;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

/** The car is quiet while moving, but never silent for explicit queries or hazards. */
public final class DrivingSpeechPolicyTest {
    @Test public void ordinaryDrivingRemainsSilent() {
        assertFalse(GameReaderController.allowedDuringDriving("Tiến.", 1));
        assertFalse(GameReaderController.allowedDuringDriving("Road ahead.", 1));
        assertFalse(GameReaderController.allowedDuringDriving("Engine humming.", 1));
    }

    @Test public void explicitLocationAndScansAlwaysRead() {
        assertTrue(GameReaderController.allowedDuringDriving("A11Y_QUERY:Coordinates. X 2.", 1));
        assertTrue(GameReaderController.allowedDuringDriving("A11Y_SCAN:Tile 1: car.", 1));
        assertTrue(GameReaderController.allowedDuringDriving("A11Y_DETAIL:Inspection.", 1));
        assertTrue(GameReaderController.allowedDuringDriving("A11Y_DETAIL_BEGIN:part", 1));
        assertTrue(GameReaderController.allowedDuringDriving("A11Y_DETAIL_PART:part", 1));
        assertTrue(GameReaderController.allowedDuringDriving("A11Y_DETAIL_END:part", 1));
    }

    @Test public void confirmedControlsAndNamedAreaChangesRead() {
        assertTrue(GameReaderController.allowedDuringDriving("A11Y_DRIVE_FEEDBACK:Engine running.", 1));
        assertTrue(GameReaderController.allowedDuringDriving("A11Y_DRIVE_FEEDBACK:Driving paused.", 1));
        assertTrue(GameReaderController.allowedDuringDriving("A11Y_AREA:Entering area: forest.", 1));
        assertFalse(GameReaderController.allowedDuringDriving("Entering area: forest.", 1));
    }

    @Test public void urgentWarningsAreNeverMuted() {
        assertTrue(GameReaderController.allowedDuringDriving("Automatic braking: wall ahead.", 3));
        assertTrue(GameReaderController.allowedDuringDriving("Vehicle skidding.", 3));
        assertTrue(GameReaderController.allowedDuringDriving("A11Y_COMBAT_ALERT:danger", 1));
    }

    @Test public void invalidMessagesCannotBypassFilter() {
        assertFalse(GameReaderController.allowedDuringDriving(null, 1));
        assertFalse(GameReaderController.allowedDuringDriving("", 1));
    }
}

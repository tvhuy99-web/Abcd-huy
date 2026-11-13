package com.cleverraven.cataclysmdda;

import androidx.test.ext.junit.runners.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;

/** Source-level direction mapping checks; device gesture behavior still needs user testing. */
@RunWith(AndroidJUnit4.class)
public class ReaderGestureSurfaceTest {
    @Test
    public void mapsEightSwipeSectorsToEightDirections() {
        assertArrayEquals(new int[] { 0, -1 }, ReaderGestureSurface.directionForSwipe(0, -100));
        assertArrayEquals(new int[] { 1, -1 }, ReaderGestureSurface.directionForSwipe(100, -100));
        assertArrayEquals(new int[] { 1, 0 }, ReaderGestureSurface.directionForSwipe(100, 0));
        assertArrayEquals(new int[] { 1, 1 }, ReaderGestureSurface.directionForSwipe(100, 100));
        assertArrayEquals(new int[] { 0, 1 }, ReaderGestureSurface.directionForSwipe(0, 100));
        assertArrayEquals(new int[] { -1, 1 }, ReaderGestureSurface.directionForSwipe(-100, 100));
        assertArrayEquals(new int[] { -1, 0 }, ReaderGestureSurface.directionForSwipe(-100, 0));
        assertArrayEquals(new int[] { -1, -1 }, ReaderGestureSurface.directionForSwipe(-100, -100));
    }

    @Test
    public void zeroLengthSwipeHasNoMovementDirection() {
        assertNull(ReaderGestureSurface.directionForSwipe(0, 0));
    }

    @Test
    public void practiceRequiresEightDistinctDirectionBits() {
        int[][] directions = new int[][] {
            { 0, -1 }, { 1, -1 }, { 1, 0 }, { 1, 1 },
            { 0, 1 }, { -1, 1 }, { -1, 0 }, { -1, -1 }
        };
        int seen = 0;
        for (int[] direction : directions) {
            int bit = GameReaderController.directionBit(direction[0], direction[1]);
            assertEquals(1, Integer.bitCount(bit));
            assertEquals(0, seen & bit);
            seen |= bit;
        }
        assertEquals(8, Integer.bitCount(seen));
    }
}

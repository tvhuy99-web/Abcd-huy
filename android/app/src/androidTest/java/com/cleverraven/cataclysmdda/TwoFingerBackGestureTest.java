package com.cleverraven.cataclysmdda;

import androidx.test.ext.junit.runners.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

@RunWith(AndroidJUnit4.class)
public final class TwoFingerBackGestureTest {
    @Test
    public void bothFingersMustSwipeDownTogether() {
        assertTrue(TwoFingerBackGesture.isDownSwipe(2, 90, -4, 98, 48));
        assertFalse(TwoFingerBackGesture.isDownSwipe(4, -90, -3, -80, 48));
        assertFalse(TwoFingerBackGesture.isDownSwipe(1, 90, 1, 10, 48));
        assertFalse(TwoFingerBackGesture.isDownSwipe(80, 65, 5, 90, 48));
        assertFalse(TwoFingerBackGesture.isDownSwipe(0, 40, 0, 90, 48));
        assertFalse(TwoFingerBackGesture.isDownSwipe(0, 90, 0, -90, 48));
    }
}

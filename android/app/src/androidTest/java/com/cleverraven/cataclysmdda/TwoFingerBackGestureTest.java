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
    @Test
    public void twoFingerHorizontalCategorySwipesAreDeliberateAndSymmetric() {
        assertTrue(TwoFingerBackGesture.isLeftSwipe(-100, -8, -110, 7, 48));
        assertTrue(TwoFingerBackGesture.isRightSwipe(100, 8, 110, -7, 48));
        assertFalse(TwoFingerBackGesture.isLeftSwipe(-100, -8, 110, -7, 48));
        assertFalse(TwoFingerBackGesture.isRightSwipe(100, 8, -110, 7, 48));
        assertFalse(TwoFingerBackGesture.isLeftSwipe(-40, 0, -110, 0, 48));
        assertFalse(TwoFingerBackGesture.isRightSwipe(40, 0, 110, 0, 48));
        assertFalse(TwoFingerBackGesture.isLeftSwipe(-100, -100, -100, -100, 48));
        assertFalse(TwoFingerBackGesture.isRightSwipe(100, 100, 100, 100, 48));
    }

    @Test
    public void horizontalAndVerticalTwoFingerGesturesNeverOverlap() {
        assertTrue(TwoFingerBackGesture.isUpSwipe(0, -110, 0, -120, 48));
        assertTrue(TwoFingerBackGesture.isDownSwipe(0, 110, 0, 120, 48));
        assertFalse(TwoFingerBackGesture.isLeftSwipe(0, -110, 0, -120, 48));
        assertFalse(TwoFingerBackGesture.isRightSwipe(0, 110, 0, 120, 48));
        assertFalse(TwoFingerBackGesture.isUpSwipe(-110, 0, -120, 0, 48));
        assertFalse(TwoFingerBackGesture.isDownSwipe(110, 0, 120, 0, 48));
    }

    @Test
    public void upwardConfirmNeedsTwoStraightFingers() {
        assertTrue(TwoFingerBackGesture.isUpSwipe(-3, -90, 4, -99, 48));
        assertFalse(TwoFingerBackGesture.isUpSwipe(0, 90, 0, 90, 48));
        assertFalse(TwoFingerBackGesture.isUpSwipe(0, -90, 0, -12, 48));
        assertFalse(TwoFingerBackGesture.isUpSwipe(72, -80, 0, -90, 48));
        assertFalse(TwoFingerBackGesture.isUpSwipe(0, -90, 0, 90, 48));
    }

}

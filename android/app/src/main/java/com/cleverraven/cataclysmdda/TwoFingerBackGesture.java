package com.cleverraven.cataclysmdda;

import android.view.MotionEvent;

/** Recognizes an unambiguous simultaneous two-finger downward Back swipe. */
final class TwoFingerBackGesture {
    private TwoFingerBackGesture() { }

    /** Both fingers must travel downward far enough without drifting sideways. */
    static boolean isDownSwipe(float dx1, float dy1, float dx2, float dy2,
                               float minimumDistance) {
        return dy1 >= minimumDistance && dy2 >= minimumDistance
                && dy1 >= Math.abs(dx1) * 1.35f
                && dy2 >= Math.abs(dx2) * 1.35f;
    }

    /** Inspect both pointers while still present at ACTION_POINTER_UP. */
    static boolean matches(MotionEvent event, int[] ids, float[] startX,
                           float[] startY, int count, float minimumDistance) {
        if (count != 2 || event.getPointerCount() != 2
                || event.getActionMasked() != MotionEvent.ACTION_POINTER_UP) {
            return false;
        }
        int first = event.findPointerIndex(ids[0]);
        int second = event.findPointerIndex(ids[1]);
        if (first < 0 || second < 0 || first == second) { return false; }
        return isDownSwipe(event.getX(first) - startX[0],
                event.getY(first) - startY[0],
                event.getX(second) - startX[1],
                event.getY(second) - startY[1], minimumDistance);
    }

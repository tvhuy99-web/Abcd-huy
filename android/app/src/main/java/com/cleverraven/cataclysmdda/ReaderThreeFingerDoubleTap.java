package com.cleverraven.cataclysmdda;

import android.content.Context;
import android.view.MotionEvent;
import android.view.ViewConfiguration;

/**
 * A narrow three-finger double-tap recognizer at Activity level.
 * Unlike the reader overlay, it still receives input when speech is off.
 * Other gestures and the live world map are left to their existing owners.
 */
final class ReaderThreeFingerDoubleTap {
    static final int PASS = 0;
    static final int CAPTURE = 1;
    static final int CONSUME = 2;
    static final int TOGGLE = 3;
    private static final long JOIN_MS = 400L;
    private static final long MAX_TAP_MS = 700L;
    private final long doubleTapTimeout = ViewConfiguration.getDoubleTapTimeout();
    private final float moveLimitSquared;
    private final int[] ids = new int[3];
    private final float[] startX = new float[3];
    private final float[] startY = new float[3];
    private int tracked;
    private int completedTaps;
    private long downTime;
    private long previousTapTime;
    private boolean captured;
    private boolean moved;
    private boolean secondTapStartedInTime;

    ReaderThreeFingerDoubleTap(Context context) {
        float moveLimit = Math.max(2f * ViewConfiguration.get(context).getScaledTouchSlop(),
                32f * context.getResources().getDisplayMetrics().density);
        moveLimitSquared = moveLimit * moveLimit;
    }

    void reset() {
        completedTaps = 0;
        previousTapTime = 0;
        resetStroke();
    }

    private void resetStroke() {
        tracked = 0;
        captured = false;
        moved = false;
        secondTapStartedInTime = false;
    }

    private void remember(MotionEvent event, int index) {
        if (tracked >= ids.length) {
            moved = true;
            return;
        }
        ids[tracked] = event.getPointerId(index);
        startX[tracked] = event.getX(index);
        startY[tracked] = event.getY(index);
        tracked++;
    }

    private void checkMovement(MotionEvent event) {
        for (int i = 0; i < event.getPointerCount(); i++) {
            int id = event.getPointerId(i);
            for (int j = 0; j < tracked; j++) {
                if (id == ids[j]) {
                    float dx = event.getX(i) - startX[j];
                    float dy = event.getY(i) - startY[j];
                    if (dx * dx + dy * dy > moveLimitSquared) { moved = true; }
                    break;
                }
            }
        }
    }

    private int finishTap(long when, boolean valid) {
        int result = captured ? CONSUME : PASS;
        boolean wasTimelySecondTap = secondTapStartedInTime;
        resetStroke();
        if (!valid) {
            completedTaps = 0;
            previousTapTime = 0;
            return result;
        }
        if (completedTaps == 1 && wasTimelySecondTap) {
            completedTaps = 0;
            previousTapTime = 0;
            return TOGGLE;
        }
        completedTaps = 1;
        previousTapTime = when;
        return result;
    }

    int observe(MotionEvent event) {
        int action = event.getActionMasked();
        long when = event.getEventTime();
        switch (action) {
            case MotionEvent.ACTION_DOWN:
                resetStroke();
                secondTapStartedInTime = completedTaps == 1
                        && when - previousTapTime <= doubleTapTimeout;
                if (!secondTapStartedInTime) {
                    completedTaps = 0;
                }
                downTime = when;
                remember(event, 0);
                return PASS;
            case MotionEvent.ACTION_POINTER_DOWN:
                checkMovement(event);
                if (event.getPointerCount() > 3
                        || tracked != event.getPointerCount() - 1) {
                    moved = true;
                } else {
                    remember(event, event.getActionIndex());
                }
                if (captured) { return CONSUME; }
                if (event.getPointerCount() == 3 && !moved && tracked == 3
                        && when - downTime <= JOIN_MS) {
                    captured = true;
                    return CAPTURE;
                }
                return PASS;
            case MotionEvent.ACTION_MOVE:
            case MotionEvent.ACTION_POINTER_UP:
                checkMovement(event);
                return captured ? CONSUME : PASS;
            case MotionEvent.ACTION_UP:
                checkMovement(event);
                return finishTap(when, captured && !moved && tracked == 3
                        && when - downTime <= MAX_TAP_MS);
            case MotionEvent.ACTION_CANCEL:
                checkMovement(event);
                // Some Android devices cancel a completed 3-finger touch.
                // Recover only a stationary, prompt triple contact.
                return finishTap(when, captured && !moved && tracked == 3
                        && event.getPointerCount() == 3
                        && when - downTime <= MAX_TAP_MS);
            default:
                return captured ? CONSUME : PASS;
        }
    }
}

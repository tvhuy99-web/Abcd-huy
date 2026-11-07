package com.cleverraven.cataclysmdda;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.ViewConfiguration;
import android.widget.LinearLayout;

/** Owns each pointer sequence once. Native widgets retain normal TalkBack gestures. */
public final class ReaderGestureSurface extends LinearLayout {
    public interface Listener {
        boolean usesTouchExploration();
        String gestureContext();
        boolean leftHandedNavigation();
        int horizontalSwipeAction();
        int verticalSwipeAction();
        float swipeDistancePx();
        long swipeDurationLimitMs();
        void navigate(int delta);
        boolean movementPracticeEnabled();
        void practiceMovementDirection(int dx, int dy);
        boolean worldMovementEnabled();
        void moveDirection(int dx, int dy);
        boolean directWorldGesturesEnabled();
        void scanDirection(int dx, int dy);
        void twoFingerTap(int count);
        void threeFingerTap();
        void activate();
        void detail();
        void readCurrent();
        void stopSpeech();
    }

    private final Listener listener;
    private final GestureDetector detector;
    private final Handler handler = new Handler(Looper.getMainLooper());
    private Runnable multiTapTimeout;
    private int twoFingerTapCount;
    private String multiTapOwner;
    private int maxPointers;
    private float multiStartX;
    private float multiStartY;
    private float multiLastX;
    private float multiLastY;
    private String owner;
    private String previousTapOwner;
    private boolean multiple;
    private boolean cancelled;
    private boolean activateOnUp;
    private boolean swipeHandled;
    private boolean longPressHandled;
    private long downTime;
    private float firstX;
    private float firstY;

    public ReaderGestureSurface(Context context, Listener listener) {
        super(context);
        this.listener = listener;
        detector = new GestureDetector(context, new GestureDetector.SimpleOnGestureListener() {
            @Override
            public boolean onDown(MotionEvent event) { return true; }

            @Override
            public boolean onDoubleTap(MotionEvent event) {
                if (previousTapOwner == null || !previousTapOwner.equals(owner)) {
                    cancelled = true;
                    return true;
                }
                activateOnUp = true;
                return true;
            }

            @Override
            public void onLongPress(MotionEvent event) {
                if (valid()) {
                    longPressHandled = true;
                    listener.detail();
                }
            }

            @Override
            public boolean onFling(MotionEvent start, MotionEvent end, float vx, float vy) {
                if (start == null || !valid() || activateOnUp || longPressHandled) {
                    return true;
                }
                if (end.getEventTime() - downTime > listener.swipeDurationLimitMs()) {
                    swipeHandled = true;
                    return true;
                }
                float dx = end.getX() - start.getX();
                float dy = end.getY() - start.getY();
                if (Math.max(Math.abs(dx), Math.abs(dy)) < listener.swipeDistancePx()) {
                    return true;
                }
                handleSwipe(dx, dy);
                return true;
            }
        });
    }

    static int[] directionForSwipe(float dx, float dy) {
        float absX = Math.abs(dx);
        float absY = Math.abs(dy);
        if (absX == 0 && absY == 0) { return null; }
        if (absX < absY * 0.41421356f) { return new int[] { 0, dy > 0 ? 1 : -1 }; }
        if (absY < absX * 0.41421356f) { return new int[] { dx > 0 ? 1 : -1, 0 }; }
        return new int[] { dx > 0 ? 1 : -1, dy > 0 ? 1 : -1 };
    }

    static int navigationDeltaForHorizontalSwipe(float dx, boolean leftHanded) {
        int delta = dx > 0 ? 1 : -1;
        return leftHanded ? -delta : delta;
    }

    private boolean valid() {
        return !cancelled && !multiple && owner != null && owner.equals(listener.gestureContext());
    }

    private static float centroidX(MotionEvent event) {
        float sum = 0;
        for (int i = 0; i < event.getPointerCount(); i++) { sum += event.getX(i); }
        return sum / event.getPointerCount();
    }

    private static float centroidY(MotionEvent event) {
        float sum = 0;
        for (int i = 0; i < event.getPointerCount(); i++) { sum += event.getY(i); }
        return sum / event.getPointerCount();
    }

    private void clearMultiTap() {
        if (multiTapTimeout != null) {
            handler.removeCallbacks(multiTapTimeout);
            multiTapTimeout = null;
        }
        twoFingerTapCount = 0;
        multiTapOwner = null;
    }

    /** Resolve one/two/three two-finger taps as one mutually exclusive sequence. */
    private void queueTwoFingerTap(String tapOwner) {
        if (twoFingerTapCount > 0 && !tapOwner.equals(multiTapOwner)) {
            clearMultiTap();
        }
        if (twoFingerTapCount == 0) { multiTapOwner = tapOwner; }
        twoFingerTapCount++;
        if (multiTapTimeout != null) {
            handler.removeCallbacks(multiTapTimeout);
            multiTapTimeout = null;
        }
        if (twoFingerTapCount == 3) {
            clearMultiTap();
            if (listener.directWorldGesturesEnabled()) { listener.twoFingerTap(3); }
        } else {
            multiTapTimeout = () -> {
                int count = twoFingerTapCount;
                String expected = multiTapOwner;
                clearMultiTap();
                if (expected != null && expected.equals(listener.gestureContext())
                        && listener.directWorldGesturesEnabled()) {
                    listener.twoFingerTap(count);
                }
            };
            handler.postDelayed(multiTapTimeout, ViewConfiguration.getDoubleTapTimeout());
        }
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent event) {
        if (listener.usesTouchExploration()) {
            cancelGesture();
            return super.dispatchTouchEvent(event);
        }
        int action = event.getActionMasked();
        if (action == MotionEvent.ACTION_DOWN) {
            owner = listener.gestureContext();
            multiple = false;
            maxPointers = 1;
            cancelled = false;
            activateOnUp = false;
            swipeHandled = false;
            longPressHandled = false;
            downTime = event.getEventTime();
            firstX = event.getX();
            firstY = event.getY();
        } else if (action == MotionEvent.ACTION_CANCEL) {
            cancelGesture();
            detector.onTouchEvent(event);
            return true;
        } else if (action == MotionEvent.ACTION_POINTER_DOWN) {
            multiple = true;
            maxPointers = Math.max(maxPointers, event.getPointerCount());
            activateOnUp = false;
            MotionEvent cancel = MotionEvent.obtain(event);
            cancel.setAction(MotionEvent.ACTION_CANCEL);
            detector.onTouchEvent(cancel);
            cancel.recycle();
            if (maxPointers > 3) { cancelled = true; }
            multiStartX = centroidX(event);
            multiStartY = centroidY(event);
            multiLastX = multiStartX;
            multiLastY = multiStartY;
        }
        if (multiple) {
            if ((action == MotionEvent.ACTION_MOVE || action == MotionEvent.ACTION_POINTER_UP)
                    && event.getPointerCount() == maxPointers) {
                multiLastX = centroidX(event);
                multiLastY = centroidY(event);
            }
            if (action == MotionEvent.ACTION_UP) {
                boolean permitted = !cancelled && owner != null
                        && owner.equals(listener.gestureContext())
                        && event.getEventTime() - downTime <= listener.swipeDurationLimitMs()
                        && listener.directWorldGesturesEnabled();
                float dx = multiLastX - multiStartX;
                float dy = multiLastY - multiStartY;
                if (permitted && maxPointers == 2) {
                    if (Math.max(Math.abs(dx), Math.abs(dy)) >= listener.swipeDistancePx()) {
                        clearMultiTap();
                        // Only cardinal directions are accepted, never diagonals.
                        if (Math.abs(dx) >= Math.abs(dy) * 1.35f) {
                            listener.scanDirection(dx > 0 ? 1 : -1, 0);
                        } else if (Math.abs(dy) >= Math.abs(dx) * 1.35f) {
                            listener.scanDirection(0, dy > 0 ? 1 : -1);
                        }
                    } else {
                        queueTwoFingerTap(owner);
                    }
                } else if (permitted && maxPointers == 3
                        && Math.max(Math.abs(dx), Math.abs(dy)) < listener.swipeDistancePx()) {
                    clearMultiTap();
                    listener.threeFingerTap();
                } else {
                    clearMultiTap();
                }
                // Keep any pending two-finger tap series across pointer resets.
                multiple = false;
                cancelled = true;
                owner = null;
                previousTapOwner = null;
            }
            return true;
        }
        detector.onTouchEvent(event);
        boolean activatedByDoubleTap = false;
        if (action == MotionEvent.ACTION_UP && !swipeHandled && !activateOnUp
                && !longPressHandled && valid()) {
            float dx = event.getX() - firstX;
            float dy = event.getY() - firstY;
            if (event.getEventTime() - downTime > listener.swipeDurationLimitMs()) {
                cancelGesture();
            } else if (Math.max(Math.abs(dx), Math.abs(dy)) >= listener.swipeDistancePx()) {
                handleSwipe(dx, dy);
            }
        }
        if (action == MotionEvent.ACTION_UP && activateOnUp && valid()) {
            activatedByDoubleTap = true;
            listener.activate();
            activateOnUp = false;
        }
        if (action == MotionEvent.ACTION_UP) {
            float dx = event.getX() - firstX;
            float dy = event.getY() - firstY;
            boolean tap = valid() && !multiple && !cancelled && !swipeHandled && !longPressHandled
                    && !activatedByDoubleTap
                    && Math.max(Math.abs(dx), Math.abs(dy)) < listener.swipeDistancePx()
                    && event.getEventTime() - downTime <= listener.swipeDurationLimitMs();
            previousTapOwner = tap ? owner : null;
        }
        return true;
    }

    private void handleSwipe(float dx, float dy) {
        swipeHandled = true;
        int[] direction = directionForSwipe(dx, dy);
        if (direction != null && listener.movementPracticeEnabled()) {
            listener.practiceMovementDirection(direction[0], direction[1]);
        } else if (listener.worldMovementEnabled()) {
            // Walking is four-way and immediately enabled in the live world.
            if (Math.abs(dx) >= Math.abs(dy) * 1.35f) {
                listener.moveDirection(dx > 0 ? 1 : -1, 0);
            } else if (Math.abs(dy) >= Math.abs(dx) * 1.35f) {
                listener.moveDirection(0, dy > 0 ? 1 : -1);
            }
        } else if (Math.abs(dx) >= Math.abs(dy) * 1.35f) {
            performMappedSwipeAction(listener.horizontalSwipeAction(),
                                     navigationDeltaForHorizontalSwipe(dx, listener.leftHandedNavigation()));
        } else if (Math.abs(dy) >= Math.abs(dx) * 1.35f) {
            performMappedSwipeAction(listener.verticalSwipeAction(), dy > 0 ? 1 : -1);
        }
    }

    private void performMappedSwipeAction(int action, int navigationDelta) {
        switch (action) {
            case ReaderPreferences.GESTURE_ACTION_NAVIGATE:
                listener.navigate(navigationDelta);
                break;
            case ReaderPreferences.GESTURE_ACTION_DETAIL:
                listener.detail();
                break;
            case ReaderPreferences.GESTURE_ACTION_READ_CURRENT:
                listener.readCurrent();
                break;
            default:
                // A disabled mapping is intentionally inert and never becomes
                // an arbitrary game command.
                break;
        }
    }

    public void cancelGesture() {
        cancelled = true;
        activateOnUp = false;
        owner = null;
        previousTapOwner = null;
        clearMultiTap();
    }
}

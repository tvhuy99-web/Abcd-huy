package com.cleverraven.cataclysmdda;

import android.content.Context;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.widget.LinearLayout;

/** Owns each pointer sequence once. Native widgets retain normal TalkBack gestures. */
public final class ReaderGestureSurface extends LinearLayout {
    public interface Listener {
        boolean usesTouchExploration();
        String gestureContext();
        boolean leftHandedNavigation();
        float swipeDistancePx();
        long swipeDurationLimitMs();
        void navigate(int delta);
        boolean movementPracticeEnabled();
        void practiceMovementDirection(int dx, int dy);
        boolean worldMovementEnabled();
        void moveDirection(int dx, int dy);
        void activate();
        void detail();
        void stopSpeech();
    }

    private final Listener listener;
    private final GestureDetector detector;
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
            activateOnUp = false;
            MotionEvent cancel = MotionEvent.obtain(event);
            cancel.setAction(MotionEvent.ACTION_CANCEL);
            detector.onTouchEvent(cancel);
            cancel.recycle();
            if (event.getPointerCount() > 2) {
                cancelled = true;
            }
        }
        if (multiple) {
            if (Math.hypot(event.getX() - firstX, event.getY() - firstY) > listener.swipeDistancePx()) {
                cancelled = true;
            }
            if (action == MotionEvent.ACTION_UP) {
                if (!cancelled && event.getEventTime() - downTime <= listener.swipeDurationLimitMs()
                        && owner != null && owner.equals(listener.gestureContext())) {
                    listener.stopSpeech();
                }
                cancelGesture();
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
            if (direction != null) {
                listener.moveDirection(direction[0], direction[1]);
            }
            // Without explicit world movement mode, menus never interpret a
            // diagonal as movement.
        } else if (Math.abs(dx) >= Math.abs(dy) * 1.35f) {
            listener.navigate(navigationDeltaForHorizontalSwipe(dx, listener.leftHandedNavigation()));
        } else if (Math.abs(dy) >= Math.abs(dx) * 1.35f) {
            listener.navigate(dy > 0 ? 1 : -1);
        }
    }

    public void cancelGesture() {
        cancelled = true;
        activateOnUp = false;
        owner = null;
        previousTapOwner = null;
    }
}

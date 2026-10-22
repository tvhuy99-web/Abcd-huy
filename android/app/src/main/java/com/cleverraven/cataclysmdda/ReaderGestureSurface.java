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
        void navigate(int delta);
        void activate();
        void detail();
        void stopSpeech();
    }

    private final Listener listener;
    private final GestureDetector detector;
    private final float swipeDistance;
    private String owner;
    private String previousTapOwner;
    private boolean multiple;
    private boolean cancelled;
    private boolean activateOnUp;
    private long downTime;
    private float firstX;
    private float firstY;

    public ReaderGestureSurface(Context context, Listener listener) {
        super(context);
        this.listener = listener;
        swipeDistance = 48 * getResources().getDisplayMetrics().density;
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
                    listener.detail();
                }
            }

            @Override
            public boolean onFling(MotionEvent start, MotionEvent end, float vx, float vy) {
                if (start == null || !valid() || activateOnUp) {
                    return true;
                }
                float dx = end.getX() - start.getX();
                float dy = end.getY() - start.getY();
                if (Math.max(Math.abs(dx), Math.abs(dy)) < swipeDistance) {
                    return true;
                }
                // Menus never interpret a diagonal as world movement.
                if (Math.abs(dx) >= Math.abs(dy) * 1.35f) {
                    listener.navigate(dx > 0 ? 1 : -1);
                } else if (Math.abs(dy) >= Math.abs(dx) * 1.35f) {
                    listener.navigate(dy > 0 ? 1 : -1);
                }
                return true;
            }
        });
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
            if (Math.hypot(event.getX() - firstX, event.getY() - firstY) > swipeDistance) {
                cancelled = true;
            }
            if (action == MotionEvent.ACTION_UP) {
                if (!cancelled && event.getEventTime() - downTime <= 700
                        && owner != null && owner.equals(listener.gestureContext())) {
                    listener.stopSpeech();
                }
                cancelGesture();
            }
            return true;
        }
        detector.onTouchEvent(event);
        if (action == MotionEvent.ACTION_UP && activateOnUp && valid()) {
            listener.activate();
            activateOnUp = false;
        }
        if (action == MotionEvent.ACTION_UP) { previousTapOwner = valid() ? owner : null; }
        return true;
    }

    public void cancelGesture() {
        cancelled = true;
        activateOnUp = false;
        owner = null;
        previousTapOwner = null;
    }
}

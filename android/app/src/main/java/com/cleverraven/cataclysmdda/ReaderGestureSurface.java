package com.cleverraven.cataclysmdda;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.ViewConfiguration;
import android.widget.LinearLayout;

import java.util.ArrayList;
import java.util.List;

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
        boolean handleCustomGesture(String gestureId);
        void activate();
        void detail();
        void readCurrent();
        void stopSpeech();
    }

    private final Listener listener;
    private final GestureDetector detector;
    private final Handler handler = new Handler(Looper.getMainLooper());
    private Runnable multiTapTimeout;
    private int multiTapCount;
    private int multiTapFingers;
    private String multiTapOwner;
    private final List<float[]> oneFingerPath = new ArrayList<>();
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
                if (listener.directWorldGesturesEnabled() || start == null || !valid()
                        || activateOnUp || longPressHandled) {
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
        multiTapCount = 0;
        multiTapFingers = 0;
        multiTapOwner = null;
    }

    /** One/two/three taps are mutually exclusive, for 2/3/4 fingers. */
    private void queueMultiTap(String tapOwner, int fingers) {
        if (multiTapCount > 0 && (!tapOwner.equals(multiTapOwner)
                || fingers != multiTapFingers)) {
            clearMultiTap();
        }
        if (multiTapCount == 0) {
            multiTapOwner = tapOwner;
            multiTapFingers = fingers;
        }
        multiTapCount++;
        if (multiTapTimeout != null) {
            handler.removeCallbacks(multiTapTimeout);
            multiTapTimeout = null;
        }
        if (multiTapCount >= 3) {
            clearMultiTap();
            dispatchMultiTap(fingers, 3);
        } else {
            multiTapTimeout = () -> {
                int count = multiTapCount;
                int countFingers = multiTapFingers;
                String expectedOwner = multiTapOwner;
                clearMultiTap();
                if (expectedOwner != null && expectedOwner.equals(listener.gestureContext())) {
                    dispatchMultiTap(countFingers, count);
                }
            };
            handler.postDelayed(multiTapTimeout, ViewConfiguration.getDoubleTapTimeout());
        }
    }

    private void dispatchMultiTap(int fingers, int count) {
        if (!listener.directWorldGesturesEnabled()) { return; }
        String gesture = ReaderGestureBindings.idForTap(fingers, count);
        if (gesture == null || listener.handleCustomGesture(gesture)) { return; }
        if (fingers == 2) {
            listener.twoFingerTap(count);
        } else if (fingers == 3 && count == 1) {
            listener.threeFingerTap();
        }
    }

    private static String cardinal(float dx, float dy, float minimum) {
        if (Math.abs(dx) >= minimum && Math.abs(dx) >= 1.35f * Math.abs(dy)) {
            return dx < 0 ? "left" : "right";
        }
        if (Math.abs(dy) >= minimum && Math.abs(dy) >= 1.35f * Math.abs(dx)) {
            return dy < 0 ? "up" : "down";
        }
        return null;
    }

    /**
     * Finds one clear change in direction, including out-and-back strokes.
     * Both legs must pass the physical swipe threshold; diagonal drags never
     * silently become a command, and straight swipes remain single-stroke.
     */
    static String identifyOneFingerSwipe(List<float[]> points, float distance) {
        if (points.size() < 2) { return null; }
        float[] start = points.get(0);
        float[] end = points.get(points.size() - 1);
        String first = null;
        String second = null;
        float best = 0.0f;
        float legMin = distance * 0.85f;
        for (int i = 1; i + 1 < points.size(); i++) {
            float[] turn = points.get(i);
            String leg1 = cardinal(turn[0] - start[0], turn[1] - start[1], legMin);
            String leg2 = cardinal(end[0] - turn[0], end[1] - turn[1], legMin);
            if (leg1 == null || leg2 == null || leg1.equals(leg2)) { continue; }
            float a = (float) Math.hypot(turn[0] - start[0], turn[1] - start[1]);
            float b = (float) Math.hypot(end[0] - turn[0], end[1] - turn[1]);
            if (Math.min(a, b) > best) {
                best = Math.min(a, b);
                first = leg1;
                second = leg2;
            }
        }
        if (first != null) {
            return ReaderGestureBindings.idForSwipe(1, first + "_" + second);
        }
        String single = cardinal(end[0] - start[0], end[1] - start[1], distance);
        return single == null ? null : ReaderGestureBindings.idForSwipe(1, single);
    }

    private void recordOneFinger(float x, float y) {
        if (oneFingerPath.size() >= 256) { return; }
        oneFingerPath.add(new float[] { x, y });
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
            oneFingerPath.clear();
            recordOneFinger(firstX, firstY);
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
            if (maxPointers > 4) { cancelled = true; }
            // Reset origin when the full finger count is reached.
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
                float threshold = listener.swipeDistancePx();
                if (permitted && maxPointers >= 2 && maxPointers <= 4) {
                    String direction = cardinal(dx, dy, threshold);
                    if (direction != null) {
                        clearMultiTap();
                        if (maxPointers == 2) {
                            // Two-finger swipes are dedicated to the 16-tile scan,
                            // not among the 33 assignable shortcut gestures.
                            listener.scanDirection("right".equals(direction) ? 1
                                    : "left".equals(direction) ? -1 : 0,
                                    "down".equals(direction) ? 1
                                    : "up".equals(direction) ? -1 : 0);
                        } else {
                            String gesture = ReaderGestureBindings.idForSwipe(
                                    maxPointers, direction);
                            if (gesture != null) { listener.handleCustomGesture(gesture); }
                        }
                    } else if (Math.max(Math.abs(dx), Math.abs(dy)) < threshold) {
                        queueMultiTap(owner, maxPointers);
                    } else {
                        clearMultiTap();
                    }
                } else {
                    clearMultiTap();
                }
                multiple = false;
                cancelled = true;
                owner = null;
                previousTapOwner = null;
            }
            return true;
        }

        if (action == MotionEvent.ACTION_MOVE && event.getPointerCount() == 1) {
            for (int i = 0; i < event.getHistorySize(); i++) {
                recordOneFinger(event.getHistoricalX(i), event.getHistoricalY(i));
            }
            recordOneFinger(event.getX(), event.getY());
        }
        detector.onTouchEvent(event);
        boolean activatedByDoubleTap = false;
        if (action == MotionEvent.ACTION_UP && !swipeHandled && !activateOnUp
                && !longPressHandled && valid()) {
            recordOneFinger(event.getX(), event.getY());
            if (event.getEventTime() - downTime > listener.swipeDurationLimitMs()) {
                cancelGesture();
            } else if (listener.directWorldGesturesEnabled()) {
                String gesture = identifyOneFingerSwipe(oneFingerPath, listener.swipeDistancePx());
                if (gesture != null) {
                    clearMultiTap();
                    if (!listener.handleCustomGesture(gesture)
                            && gesture.indexOf('_', "swipe_".length()) < 0) {
                        handleSwipe(event.getX() - firstX, event.getY() - firstY);
                    }
                }
            } else if (Math.max(Math.abs(event.getX() - firstX),
                                 Math.abs(event.getY() - firstY)) >= listener.swipeDistancePx()) {
                handleSwipe(event.getX() - firstX, event.getY() - firstY);
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
        oneFingerPath.clear();
    }
}

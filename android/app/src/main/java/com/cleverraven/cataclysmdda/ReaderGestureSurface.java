package com.cleverraven.cataclysmdda;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.ViewConfiguration;
import android.view.ViewParent;
import android.widget.LinearLayout;

import java.util.ArrayList;
import java.util.List;


public final class ReaderGestureSurface extends LinearLayout {
    public interface Listener {
        boolean usesTouchExploration();
        String gestureContext();
        int horizontalSwipeAction();
        int verticalSwipeAction();
        float swipeDistancePx();
        long swipeDurationLimitMs();
        void navigate(int delta);

        boolean adjustFocusedSetting(int delta);

        default boolean handleCreatorCalendarSwipe(int horizontal, int vertical) { return false; }

        default boolean handleInventoryAxisSwipe(int horizontal, int vertical) { return false; }

        default boolean handleVehicleTwoFingerSwipe(int horizontal, int vertical) { return false; }

        default boolean handleSurroundingsSwipe(int horizontal, int vertical) { return false; }

        default boolean handleCraftingRecipeSwipe(int horizontal) { return false; }

        default boolean handleCraftingSubcategorySwipe(int vertical) { return false; }

        default boolean handleCraftingMainCategorySwipe(int horizontal) { return false; }

        void goBack();

        default void confirm() { }

        default void openContextActions() { }

        default boolean gestureGuideActive() { return false; }

        default boolean autonomousDriveTravelActive() { return false; }
        boolean movementPracticeEnabled();
        void practiceMovementDirection(int dx, int dy);
        boolean worldMovementEnabled();
        void moveDirection(int dx, int dy);
        boolean directWorldGesturesEnabled();
        void scanDirection(int dx, int dy);
        void twoFingerTap(int count);
        void threeFingerTap();

        default void characterDetails() { }

        default void toggleCombatMode() { }

        void openActionMenu();
        boolean handleCustomGesture(String gestureId);

        default void unassignedMultiGesture(String gestureId) { }
        void activate();
        void detail();
        void readCurrent();
        void stopSpeech();

        default void onUserAction() { }
    }

    private final Listener listener;
    private final GestureDetector detector;
    private final Handler handler = new Handler(Looper.getMainLooper());
    private Runnable multiTapTimeout;
    private Runnable multiHoldTimeout;
    private Runnable worldTapTimeout;
    private int worldTapCount;
    private String worldTapOwner;
    private long worldTapLastTime;
    private float worldTapX;
    private float worldTapY;
    private int multiTapCount;
    private int multiTapFingers;
    private String multiTapOwner;
    private final List<float[]> oneFingerPath = new ArrayList<>();
    private int maxPointers;


    private static final int MAX_READER_FINGERS = 4;
    private static final long MULTI_FINGER_JOIN_MS = 400L;
    private static final long MULTI_CANCEL_TAP_MS = 700L;
    private final int[] multiPointerIds = new int[MAX_READER_FINGERS];
    private final float[] multiPointerStartX = new float[MAX_READER_FINGERS];
    private final float[] multiPointerStartY = new float[MAX_READER_FINGERS];
    private int multiTrackedCount;
    private boolean multiFingerMoved;
    private boolean lateMultiFingerJoin;
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
    private boolean userActionSignaled;
    private boolean longPressHandled;
    private boolean ignoreUntilPointersReleased;
    private final int[] backPointerIds = new int[2];
    private final float[] backPointerStartX = new float[2];
    private final float[] backPointerStartY = new float[2];
    private int backPointerCount;
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
            public boolean onSingleTapConfirmed(MotionEvent event) {



                if( !listener.directWorldGesturesEnabled() && previousTapOwner != null
                        && previousTapOwner.equals(listener.gestureContext()) ) {
                    listener.stopSpeech();
                }
                return true;
            }

            @Override
            public boolean onDoubleTap(MotionEvent event) {


                if (listener.directWorldGesturesEnabled()) { return true; }
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
                    SessionDebugLog.event("GESTURE", "one_finger_long_press"
                            + " context=" + owner);
                    longPressHandled = true;
                    signalUserAction();


                    listener.openContextActions();
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

    static int navigationDeltaForHorizontalSwipe(float dx) {
        return dx > 0 ? 1 : -1;
    }

    private boolean valid() {
        return !cancelled && !multiple && owner != null && owner.equals(listener.gestureContext());
    }


    private void signalUserAction() {
        if (!userActionSignaled) {
            userActionSignaled = true;
            listener.onUserAction();
        }
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

    private void requestParentCapture(boolean disallow) {
        ViewParent parent = getParent();
        if (parent != null) {
            parent.requestDisallowInterceptTouchEvent(disallow);
        }
    }

    private void capturePointerStarts(MotionEvent event) {
        int count = Math.min(MAX_READER_FINGERS, event.getPointerCount());
        multiTrackedCount = count;
        for (int i = 0; i < count; i++) {
            multiPointerIds[i] = event.getPointerId(i);
            multiPointerStartX[i] = event.getX(i);
            multiPointerStartY[i] = event.getY(i);
        }
        multiFingerMoved = false;
        lateMultiFingerJoin = false;
    }

    private void rememberNewPointer(MotionEvent event) {
        if (event.getPointerCount() > MAX_READER_FINGERS) {
            multiTrackedCount = 0;
            return;
        }
        if (event.getEventTime() - downTime > MULTI_FINGER_JOIN_MS) {
            lateMultiFingerJoin = true;
        }
        int index = event.getActionIndex();
        if (multiTrackedCount != event.getPointerCount() - 1 || index < 0
                || index >= event.getPointerCount()) {
            multiTrackedCount = 0;
            return;
        }
        for (int i = 0; i < multiTrackedCount; i++) {
            if (multiPointerIds[i] == event.getPointerId(index)) {
                multiTrackedCount = 0;
                return;
            }
        }
        multiPointerIds[multiTrackedCount] = event.getPointerId(index);
        multiPointerStartX[multiTrackedCount] = event.getX(index);
        multiPointerStartY[multiTrackedCount] = event.getY(index);
        multiTrackedCount++;
    }

    private void trackMultiFingerMovement(MotionEvent event) {
        float thresholdPx = Math.min(listener.swipeDistancePx() * 0.6f,
                32f * getResources().getDisplayMetrics().density);
        float limit = thresholdPx * thresholdPx;
        for (int i = 0; i < event.getPointerCount(); i++) {
            int pointerId = event.getPointerId(i);
            for (int saved = 0; saved < multiTrackedCount; saved++) {
                if (multiPointerIds[saved] == pointerId) {
                    float dx = event.getX(i) - multiPointerStartX[saved];
                    float dy = event.getY(i) - multiPointerStartY[saved];
                    if (dx * dx + dy * dy > limit) { multiFingerMoved = true; }
                    break;
                }
            }
        }
    }

    private boolean recoverCancelledMultiTap(MotionEvent event) {


        if (!multiple || cancelled || maxPointers < 3 || maxPointers > 4
                || maxPointers != event.getPointerCount()
                || multiTrackedCount != maxPointers || multiFingerMoved
                || lateMultiFingerJoin || owner == null
                || !owner.equals(listener.gestureContext())
                || !listener.directWorldGesturesEnabled()
                || event.getEventTime() - downTime > MULTI_CANCEL_TAP_MS) {
            return false;
        }
        for (int i = 0; i < event.getPointerCount(); i++) {
            boolean known = false;
            for (int saved = 0; saved < multiTrackedCount; saved++) {
                if (multiPointerIds[saved] == event.getPointerId(i)) {
                    known = true;
                    break;
                }
            }
            if (!known) { return false; }
        }
        trackMultiFingerMovement(event);
        return !multiFingerMoved;
    }

    private void cancelMultiHold() {
        if (multiHoldTimeout != null) {
            handler.removeCallbacks(multiHoldTimeout);
            multiHoldTimeout = null;
        }
    }


    private void scheduleMultiHold(int fingers, String expectedOwner) {
        cancelMultiHold();
        if (fingers < 2 || fingers > 3 || lateMultiFingerJoin
                || expectedOwner == null || !listener.directWorldGesturesEnabled()) {
            return;
        }
        multiHoldTimeout = () -> {
            multiHoldTimeout = null;
            if (cancelled || !multiple || maxPointers != fingers
                    || multiTrackedCount != fingers || multiFingerMoved
                    || lateMultiFingerJoin || owner == null
                    || !owner.equals(expectedOwner)
                    || !owner.equals(listener.gestureContext())
                    || !listener.directWorldGesturesEnabled()) {
                return;
            }
            longPressHandled = true;
            cancelled = true;
            clearMultiTap();
            clearWorldTaps();
            String gesture = ReaderGestureBindings.idForHold(fingers);
            SessionDebugLog.event("GESTURE", "long_press fingers=" + fingers
                    + " id=" + gesture);
            signalUserAction();
            if (!listener.handleCustomGesture(gesture)) {
                listener.unassignedMultiGesture(gesture);
            }
        };
        handler.postDelayed(multiHoldTimeout, ViewConfiguration.getLongPressTimeout());
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

    private void clearWorldTaps() {
        if (worldTapTimeout != null) {
            handler.removeCallbacks(worldTapTimeout);
            worldTapTimeout = null;
        }
        worldTapCount = 0;
        worldTapOwner = null;
    }

    private void queueWorldTap(MotionEvent event, String gestureOwner) {
        long when = event.getEventTime();
        float x = event.getX(), y = event.getY();
        float slop = listener.swipeDistancePx();
        if (worldTapCount > 0 && (gestureOwner == null
                || !gestureOwner.equals(worldTapOwner)
                || when - worldTapLastTime > ViewConfiguration.getDoubleTapTimeout()
                || Math.abs(x - worldTapX) > slop
                || Math.abs(y - worldTapY) > slop)) {
            clearWorldTaps();
        }
        if (worldTapCount == 0) {
            worldTapOwner = gestureOwner;
            worldTapX = x;
            worldTapY = y;
        }
        worldTapCount++;
        worldTapLastTime = when;
        if (worldTapTimeout != null) { handler.removeCallbacks(worldTapTimeout); }
        if (worldTapCount >= 3) {
            String expectedOwner = worldTapOwner;
            clearWorldTaps();
            if (expectedOwner != null && expectedOwner.equals(listener.gestureContext())
                    && listener.directWorldGesturesEnabled()) {
                SessionDebugLog.event("GESTURE", "one_finger_triple_tap_action_hub");
                signalUserAction();
                listener.openActionMenu();
            }
        } else {
            worldTapTimeout = () -> {
                int count = worldTapCount;
                String expectedOwner = worldTapOwner;
                clearWorldTaps();
                if (expectedOwner != null && expectedOwner.equals(listener.gestureContext())
                        && listener.directWorldGesturesEnabled()) {
                    if (count == 1) {



                        listener.stopSpeech();
                    } else if (count == 2) {
                        signalUserAction();
                        listener.activate();
                    }
                }
            };
            handler.postDelayed(worldTapTimeout, ViewConfiguration.getDoubleTapTimeout());
        }
    }


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
        SessionDebugLog.event("GESTURE", "multi_tap fingers=" + fingers
                + " count=" + count);






        if (fingers == 2) {
            if (count == 1 || count == 2 || listener.directWorldGesturesEnabled()
                    || listener.autonomousDriveTravelActive()) {
                signalUserAction();
                listener.twoFingerTap(count);
            }
            return;
        }
        if (!listener.directWorldGesturesEnabled()
                && !listener.autonomousDriveTravelActive()) { return; }
        signalUserAction();
        if (fingers == 3 && count == 1) {
            listener.threeFingerTap();
            return;
        }
        if (fingers == 3 && count == 2) {
            listener.characterDetails();
            return;
        }
        if (fingers == 3 && count == 3) {
            listener.toggleCombatMode();
            return;
        }
        String gesture = ReaderGestureBindings.idForTap(fingers, count);
        if (gesture == null || listener.handleCustomGesture(gesture)) { return; }
        listener.unassignedMultiGesture(gesture);
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
        if (action == MotionEvent.ACTION_POINTER_DOWN && event.getPointerCount() >= 2) {


            requestParentCapture(true);
        } else if (action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_CANCEL) {
            requestParentCapture(false);
        }

        if (ignoreUntilPointersReleased) {
            if (action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_CANCEL) {
                ignoreUntilPointersReleased = false;
            }
            return true;
        }
        if (action == MotionEvent.ACTION_DOWN) {
            cancelMultiHold();
            owner = listener.gestureContext();
            multiple = false;
            maxPointers = 1;
            multiTrackedCount = 0;
            multiFingerMoved = false;
            lateMultiFingerJoin = false;
            backPointerCount = 0;
            cancelled = false;
            userActionSignaled = false;
            activateOnUp = false;
            swipeHandled = false;
            longPressHandled = false;
            downTime = event.getEventTime();
            firstX = event.getX();
            firstY = event.getY();
            oneFingerPath.clear();
            recordOneFinger(firstX, firstY);
        } else if (action == MotionEvent.ACTION_CANCEL) {
            cancelMultiHold();
            SessionDebugLog.event("TOUCH", "cancel fingers=" + maxPointers
                    + " age_ms=" + (event.getEventTime() - downTime));
            boolean recovered = recoverCancelledMultiTap(event);
            String capturedOwner = owner;
            int capturedFingers = maxPointers;
            cancelGesture(recovered);
            detector.onTouchEvent(event);
            if (recovered) {

                queueMultiTap(capturedOwner, capturedFingers);
            }
            return true;
        } else if (action == MotionEvent.ACTION_POINTER_DOWN) {
            cancelMultiHold();
            multiple = true;
            maxPointers = Math.max(maxPointers, event.getPointerCount());
            if (event.getPointerCount() == 2) {
                capturePointerStarts(event);
                lateMultiFingerJoin = event.getEventTime() - downTime
                        > MULTI_FINGER_JOIN_MS;
            } else {
                trackMultiFingerMovement(event);
                rememberNewPointer(event);
            }
            activateOnUp = false;
            MotionEvent cancel = MotionEvent.obtain(event);
            cancel.setAction(MotionEvent.ACTION_CANCEL);
            detector.onTouchEvent(cancel);
            cancel.recycle();
            if (maxPointers > 4) { cancelled = true; }
            if (event.getPointerCount() == 2) {
                backPointerCount = 2;
                for (int i = 0; i < 2; i++) {
                    backPointerIds[i] = event.getPointerId(i);
                    backPointerStartX[i] = event.getX(i);
                    backPointerStartY[i] = event.getY(i);
                }
            } else {
                backPointerCount = 0;
            }

            multiStartX = centroidX(event);
            multiStartY = centroidY(event);
            multiLastX = multiStartX;
            multiLastY = multiStartY;
            scheduleMultiHold(event.getPointerCount(), owner);
        }
        if (action == MotionEvent.ACTION_UP) { cancelMultiHold(); }
        if (multiple) {
            if (action == MotionEvent.ACTION_MOVE
                    || action == MotionEvent.ACTION_POINTER_UP) {
                trackMultiFingerMovement(event);
            }
            if (action == MotionEvent.ACTION_POINTER_UP || multiFingerMoved) {
                cancelMultiHold();
            }



            if (action == MotionEvent.ACTION_POINTER_UP && backPointerCount == 2
                    && !cancelled && owner != null && owner.equals(listener.gestureContext())
                    && !listener.directWorldGesturesEnabled()
                    && !listener.autonomousDriveTravelActive()
                    && event.getEventTime() - downTime
                            <= Math.max(1500L, listener.swipeDurationLimitMs())) {
                boolean left = TwoFingerBackGesture.matchesLeft(event, backPointerIds,
                        backPointerStartX, backPointerStartY, backPointerCount,
                        listener.swipeDistancePx());
                boolean right = !left && TwoFingerBackGesture.matchesRight(event, backPointerIds,
                        backPointerStartX, backPointerStartY, backPointerCount,
                        listener.swipeDistancePx());
                if (left || right) { signalUserAction(); }
                if ((left || right) && listener.handleCraftingMainCategorySwipe(left ? -1 : 1)) {
                    SessionDebugLog.event("GESTURE", "crafting_main_category_two_fingers="
                            + (left ? "left" : "right"));
                    ignoreUntilPointersReleased = true;
                    multiple = false;
                    backPointerCount = 0;
                    cancelled = true;
                    owner = null;
                    previousTapOwner = null;
                    clearMultiTap();
                    return true;
                }
            }


            if (action == MotionEvent.ACTION_POINTER_UP && backPointerCount == 2
                    && !cancelled && owner != null && owner.equals(listener.gestureContext())
                    && !listener.directWorldGesturesEnabled()
                    && !listener.autonomousDriveTravelActive()
                    && event.getEventTime() - downTime
                            <= Math.max(1500L, listener.swipeDurationLimitMs())
                    && TwoFingerBackGesture.matchesUp(event, backPointerIds,
                            backPointerStartX, backPointerStartY, backPointerCount,
                            listener.swipeDistancePx())) {
                ignoreUntilPointersReleased = true;
                multiple = false;
                backPointerCount = 0;
                cancelled = true;
                owner = null;
                previousTapOwner = null;
                clearMultiTap();
                signalUserAction();
                listener.confirm();
                return true;
            }


            if (action == MotionEvent.ACTION_POINTER_UP && backPointerCount == 2
                    && !cancelled && owner != null && owner.equals(listener.gestureContext())
                    && !listener.directWorldGesturesEnabled()
                    && !listener.autonomousDriveTravelActive()
                    && event.getEventTime() - downTime
                            <= Math.max(1500L, listener.swipeDurationLimitMs())
                    && TwoFingerBackGesture.matches(event, backPointerIds,
                            backPointerStartX, backPointerStartY, backPointerCount,
                            listener.swipeDistancePx())) {
                ignoreUntilPointersReleased = true;
                multiple = false;
                backPointerCount = 0;
                cancelled = true;
                owner = null;
                previousTapOwner = null;
                clearMultiTap();
                signalUserAction();
                listener.goBack();
                return true;
            }
            if ((action == MotionEvent.ACTION_MOVE || action == MotionEvent.ACTION_POINTER_UP)
                    && event.getPointerCount() == maxPointers) {
                multiLastX = centroidX(event);
                multiLastY = centroidY(event);
            }
            if (action == MotionEvent.ACTION_UP) {
                SessionDebugLog.event("TOUCH", "finish multi_fingers=" + maxPointers
                        + " duration_ms=" + (event.getEventTime() - downTime)
                        + " dx=" + (multiLastX - multiStartX)
                        + " dy=" + (multiLastY - multiStartY));




                boolean permitted = !cancelled && owner != null
                        && owner.equals(listener.gestureContext())
                        && event.getEventTime() - downTime <= listener.swipeDurationLimitMs()
                        && (listener.directWorldGesturesEnabled() || maxPointers == 2);
                float dx = multiLastX - multiStartX;
                float dy = multiLastY - multiStartY;
                float threshold = listener.swipeDistancePx();
                if (permitted && maxPointers >= 2 && maxPointers <= 4) {
                    String direction = cardinal(dx, dy, threshold);
                    if (direction != null) {
                        clearMultiTap();
                        signalUserAction();
                        if (maxPointers == 2) {


                            int horizontal = "right".equals(direction) ? 1
                                    : "left".equals(direction) ? -1 : 0;
                            int vertical = "down".equals(direction) ? 1
                                    : "up".equals(direction) ? -1 : 0;
                            if (listener.handleVehicleTwoFingerSwipe(horizontal, vertical)) {

                            } else if (listener.directWorldGesturesEnabled()) {
                                listener.scanDirection("right".equals(direction) ? 1
                                        : "left".equals(direction) ? -1 : 0,
                                        "down".equals(direction) ? 1
                                        : "up".equals(direction) ? -1 : 0);
                            }
                        } else if (maxPointers >= 3 && listener.directWorldGesturesEnabled()) {
                            String gesture = ReaderGestureBindings.idForSwipe(
                                    maxPointers, direction);
                            if (gesture != null && !listener.handleCustomGesture(gesture)) {
                                listener.unassignedMultiGesture(gesture);
                            }
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
            } else if (listener.directWorldGesturesEnabled()
                    || listener.autonomousDriveTravelActive()) {
                String gesture = identifyOneFingerSwipe(oneFingerPath, listener.swipeDistancePx());
                if (gesture != null) {
                    SessionDebugLog.event("GESTURE", "one_finger=" + gesture
                            + " duration_ms=" + (event.getEventTime() - downTime));
                    clearMultiTap();
                    signalUserAction();
                    boolean handled = listener.handleCustomGesture(gesture);
                    if (handled) {



                        swipeHandled = true;
                    } else if (gesture.indexOf('_', "swipe_".length()) < 0) {
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
            signalUserAction();
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
            if (tap && listener.directWorldGesturesEnabled()) {
                queueWorldTap(event, owner);
            } else if (!tap) {
                clearWorldTaps();
            }
        }
        return true;
    }

    private void handleSwipe(float dx, float dy) {
        signalUserAction();
        SessionDebugLog.event("GESTURE", "swipe dx=" + dx + " dy=" + dy
                + " world_move=" + listener.worldMovementEnabled());
        swipeHandled = true;
        int horizontal = Math.abs(dx) >= Math.abs(dy) * 1.35f ? (dx > 0 ? 1 : -1) : 0;
        int vertical = Math.abs(dy) >= Math.abs(dx) * 1.35f ? (dy > 0 ? 1 : -1) : 0;
        if (listener.gestureGuideActive()) {


            if (horizontal != 0) { listener.navigate(horizontal); }
            else if (vertical != 0) { listener.navigate(vertical); }
            return;
        }
        if ((horizontal != 0 || vertical != 0)
                && listener.handleCreatorCalendarSwipe(horizontal, vertical)) {
            return;
        }
        if ((horizontal != 0 || vertical != 0)
                && listener.handleSurroundingsSwipe(horizontal, vertical)) {
            return;
        }
        if ((horizontal != 0 || vertical != 0)
                && listener.handleInventoryAxisSwipe(horizontal, vertical)) {
            return;
        }
        if (horizontal != 0 && listener.handleCraftingRecipeSwipe(horizontal)) {
            return;
        }
        if (vertical != 0 && listener.handleCraftingSubcategorySwipe(vertical)) {
            return;
        }
        int[] direction = directionForSwipe(dx, dy);
        if (direction != null && listener.movementPracticeEnabled()) {
            listener.practiceMovementDirection(direction[0], direction[1]);
        } else if (listener.worldMovementEnabled()) {

            if (Math.abs(dx) >= Math.abs(dy) * 1.35f) {
                listener.moveDirection(dx > 0 ? 1 : -1, 0);
            } else if (Math.abs(dy) >= Math.abs(dx) * 1.35f) {
                listener.moveDirection(0, dy > 0 ? 1 : -1);
            }
        } else if (Math.abs(dx) >= Math.abs(dy) * 1.35f) {
            performMappedSwipeAction(listener.horizontalSwipeAction(),
                                     navigationDeltaForHorizontalSwipe(dx));
        } else if (Math.abs(dy) >= Math.abs(dx) * 1.35f) {


            if (!listener.adjustFocusedSetting(dy < 0 ? 1 : -1)) {
                performMappedSwipeAction(listener.verticalSwipeAction(), dy > 0 ? 1 : -1);
            }
        }
    }

    private void performMappedSwipeAction(int action, int navigationDelta) {
        switch (action) {
            case ReaderPreferences.GESTURE_ACTION_NAVIGATE:
                listener.navigate(navigationDelta);
                break;
            default:


                break;
        }
    }

    public void cancelGesture() {
        cancelGesture(false);
    }

    private void cancelGesture(boolean keepMultiTap) {
        cancelMultiHold();
        cancelled = true;
        activateOnUp = false;
        owner = null;
        previousTapOwner = null;
        if (!keepMultiTap) { clearMultiTap(); }
        clearWorldTaps();
        multiTrackedCount = 0;
        multiFingerMoved = false;
        lateMultiFingerJoin = false;
        requestParentCapture(false);
        oneFingerPath.clear();
        backPointerCount = 0;
        ignoreUntilPointersReleased = false;
    }
}

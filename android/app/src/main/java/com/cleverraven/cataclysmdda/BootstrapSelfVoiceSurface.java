package com.cleverraven.cataclysmdda;

import android.content.Context;
import android.graphics.Rect;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CompoundButton;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.TextView;

import java.util.ArrayList;

/** Gesture-driven spoken navigation for the Android startup screens before the game engine starts. */
public final class BootstrapSelfVoiceSurface extends ScrollView {
    public interface Listener {
        boolean isActive();
        void speak(String id, String text);
        void stopSpeech();
        boolean leftHandedNavigation();
        float swipeDistancePx();
        long swipeDurationLimitMs();
    }

    private final Listener listener;
    private final GestureDetector detector;
    private final ArrayList<View> items = new ArrayList<>();
    private int focus;
    private int downFocus = -1;
    private int previousTapFocus = -1;
    private boolean multiplePointers;
    private boolean activateOnDoubleTap;
    private boolean swipeHandled;
    private boolean multiplePointerMoved;
    private long pointerDownTime;
    private long contentGeneration;
    private long touchGeneration;
    private float downX;
    private float downY;
    private final int[] pointerIds = new int[5];
    private final float[] pointerDownX = new float[5];
    private final float[] pointerDownY = new float[5];
    private int pointerCount;

    public BootstrapSelfVoiceSurface(Context context, Listener listener) {
        super(context);
        this.listener = listener;
        setFillViewport(true);
        setFocusable(true);
        setClickable(true);
        detector = new GestureDetector(context, new GestureDetector.SimpleOnGestureListener() {
            @Override
            public boolean onDown(MotionEvent event) { return true; }

            @Override
            public boolean onSingleTapConfirmed(MotionEvent event) {
                speakFocused();
                return true;
            }

            @Override
            public boolean onDoubleTap(MotionEvent event) {
                activateOnDoubleTap = previousTapFocus >= 0 && previousTapFocus == downFocus;
                return true;
            }

            @Override
            public boolean onFling(MotionEvent start, MotionEvent end, float velocityX, float velocityY) {
                if (start == null) { return true; }
                float dx = end.getX() - start.getX();
                float dy = end.getY() - start.getY();
                if (Math.max(Math.abs(dx), Math.abs(dy)) < listener.swipeDistancePx()) {
                    return true;
                }
                if (end.getEventTime() - pointerDownTime > listener.swipeDurationLimitMs()) {
                    swipeHandled = true;
                    return true;
                }
                swipeHandled = true;
                previousTapFocus = -1;
                handleSwipe(dx, dy);
                return true;
            }
        });
    }

    public void setContent(View content) {
        removeAllViews();
        items.clear();
        LinearLayout column = new LinearLayout(getContext());
        column.setOrientation(LinearLayout.VERTICAL);
        int padding = Math.round(20 * getResources().getDisplayMetrics().density);
        column.setPadding(padding, padding, padding, padding);
        if (content.getParent() instanceof ViewGroup) {
            ((ViewGroup) content.getParent()).removeView(content);
        }
        column.addView(content, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        addView(column, new ScrollView.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        collect(content);
        focus = 0;
        previousTapFocus = -1;
        activateOnDoubleTap = false;
        contentGeneration++;
        post(this::speakFocused);
    }

    private void collect(View view) {
        if (view.getVisibility() != View.VISIBLE) { return; }
        if (view instanceof TextView || view instanceof SeekBar) {
            items.add(view);
            return;
        }
        if (view instanceof ViewGroup) {
            ViewGroup group = (ViewGroup) view;
            for (int i = 0; i < group.getChildCount(); i++) { collect(group.getChildAt(i)); }
        }
    }

    public void speakFocused() {
        if (items.isEmpty()) { return; }
        focus = Math.floorMod(focus, items.size());
        View view = items.get(focus);
        String text;
        if (view instanceof CompoundButton) {
            CompoundButton button = (CompoundButton) view;
            text = button.getText() + ". " + getContext().getString(
                    button.isChecked() ? R.string.a11yChecked : R.string.a11yUnchecked);
        } else if (view.getContentDescription() != null) {
            text = view.getContentDescription().toString();
        } else if (view instanceof TextView) {
            text = ((TextView) view).getText().toString();
        } else {
            text = view.toString();
        }
        if (!view.isEnabled()) { text += ". " + getContext().getString(R.string.a11yReaderDisabledEntry); }
        view.requestRectangleOnScreen(new Rect(0, 0, view.getWidth(), view.getHeight()), true);
        listener.speak("bootstrap-focus-" + focus, text);
    }

    private void navigate(int delta) {
        if (items.isEmpty()) { return; }
        int direction = listener.leftHandedNavigation() ? -delta : delta;
        focus = Math.floorMod(focus + direction, items.size());
        speakFocused();
    }

    private void adjustFocusedSetting(int delta) {
        if (items.isEmpty()) { return; }
        View view = items.get(focus);
        if (view instanceof SeekBar) {
            SeekBar bar = (SeekBar) view;
            bar.setProgress(Math.max(0, Math.min(bar.getMax(), bar.getProgress() + delta)));
            speakFocused();
        }
    }

    private void handleSwipe(float dx, float dy) {
        if (Math.abs(dx) > Math.abs(dy)) {
            navigate(dx < 0 ? 1 : -1);
        } else {
            adjustFocusedSetting(dy < 0 ? 1 : -1);
        }
    }

    private void activateFocused() {
        if (items.isEmpty()) { return; }
        View view = items.get(focus);
        if (view.isEnabled() && view.isClickable()) { view.performClick(); }
        else { speakFocused(); }
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent event) {
        if (!listener.isActive()) {
            MotionEvent cancel = MotionEvent.obtain(event);
            cancel.setAction(MotionEvent.ACTION_CANCEL);
            detector.onTouchEvent(cancel);
            cancel.recycle();
            cancelTouchState();
            return super.dispatchTouchEvent(event);
        }
        int action = event.getActionMasked();
        if (action == MotionEvent.ACTION_DOWN) {
            touchGeneration = contentGeneration;
            multiplePointers = false;
            pointerDownTime = event.getEventTime();
            downX = event.getX();
            downY = event.getY();
            downFocus = focus;
            activateOnDoubleTap = false;
            swipeHandled = false;
        } else if (touchGeneration != contentGeneration) {
            MotionEvent cancel = MotionEvent.obtain(event);
            cancel.setAction(MotionEvent.ACTION_CANCEL);
            detector.onTouchEvent(cancel);
            cancel.recycle();
            cancelTouchState();
            return true;
        } else if (action == MotionEvent.ACTION_POINTER_DOWN) {
            multiplePointers = true;
            multiplePointerMoved = false;
            pointerCount = Math.min(event.getPointerCount(), pointerIds.length);
            for (int i = 0; i < pointerCount; i++) {
                pointerIds[i] = event.getPointerId(i);
                pointerDownX[i] = event.getX(i);
                pointerDownY[i] = event.getY(i);
            }
            previousTapFocus = -1;
            MotionEvent cancel = MotionEvent.obtain(event);
            cancel.setAction(MotionEvent.ACTION_CANCEL);
            detector.onTouchEvent(cancel);
            cancel.recycle();
        }
        if (multiplePointers) {
            for (int i = 0; i < event.getPointerCount(); i++) {
                for (int saved = 0; saved < pointerCount; saved++) {
                    if (pointerIds[saved] == event.getPointerId(i)
                            && Math.hypot(event.getX(i) - pointerDownX[saved],
                                          event.getY(i) - pointerDownY[saved]) > listener.swipeDistancePx()) {
                        multiplePointerMoved = true;
                        break;
                    }
                }
            }
            if (action == MotionEvent.ACTION_UP) {
                if (!multiplePointerMoved
                        && event.getEventTime() - pointerDownTime <= listener.swipeDurationLimitMs()) {
                    listener.stopSpeech();
                }
                multiplePointers = false;
                pointerCount = 0;
            }
            return true;
        }
        detector.onTouchEvent(event);
        boolean activated = action == MotionEvent.ACTION_UP && activateOnDoubleTap && focus == downFocus;
        if (activated) { activateFocused(); }
        if (action == MotionEvent.ACTION_UP) {
            float dx = event.getX() - downX;
            float dy = event.getY() - downY;
            boolean enoughDistance = Math.max(Math.abs(dx), Math.abs(dy)) >= listener.swipeDistancePx();
            boolean shortEnough = event.getEventTime() - pointerDownTime <= listener.swipeDurationLimitMs();
            if (enoughDistance && shortEnough && !swipeHandled && !activated) {
                swipeHandled = true;
                previousTapFocus = -1;
                handleSwipe(dx, dy);
            }
            previousTapFocus = !enoughDistance && shortEnough && !activated ? downFocus : -1;
            activateOnDoubleTap = false;
        }
        return true;
    }

    private void cancelTouchState() {
        multiplePointers = false;
        activateOnDoubleTap = false;
        swipeHandled = false;
        multiplePointerMoved = true;
        previousTapFocus = -1;
        touchGeneration = contentGeneration;
    }
}

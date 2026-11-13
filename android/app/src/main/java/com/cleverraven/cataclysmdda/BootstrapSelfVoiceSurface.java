package com.cleverraven.cataclysmdda;

import android.content.Context;
import android.graphics.Rect;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.accessibility.AccessibilityNodeInfo;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.Spinner;
import android.widget.TextView;

import java.util.ArrayList;

/** Gesture-driven spoken navigation for the Android startup screens before the game engine starts. */
public final class BootstrapSelfVoiceSurface extends ScrollView {
    public interface Listener {
        boolean isActive();
        void speak(String id, String text);
        void stopSpeech();
        /** A new focused control supersedes the previous spoken control. */
        default void interruptForFocus() { stopSpeech(); }
        /** Return from this outer Android screen without saving draft settings. */
        void goBack();
        float swipeDistancePx();
        long swipeDurationLimitMs();
    }

    private final Listener listener;
    private final GestureDetector detector;
    private final ArrayList<View> items = new ArrayList<>();
    private View primaryContent;
    private Spinner choosingSpinner;
    private int previousFormFocus;
    private int focus;
    private int downFocus = -1;
    private int previousTapFocus = -1;
    private boolean multiplePointers;
    private boolean ignoreUntilPointersReleased;
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
        setContent(content, 0);
    }

    /** Replace the presented controls and restore their native reader focus. */
    public void setContent(View content, int initialFocus) {
        // The caller's real form remains intact when a spoken choice list opens.
        choosingSpinner = null;
        primaryContent = content;
        showContent(content, initialFocus);
    }

    private void showContent(View content, int initialFocus) {
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
        annotateTalkBackPositions();
        focus = items.isEmpty() ? 0 : Math.max(0, Math.min(initialFocus, items.size() - 1));
        previousTapFocus = -1;
        activateOnDoubleTap = false;
        contentGeneration++;
        post(this::speakFocused);
    }

    /** Keep native Android semantics, adding the same item index as the spoken reader. */
    private void annotateTalkBackPositions() {
        final int total = items.size();
        for (int i = 0; i < total; i++) {
            final View item = items.get(i);
            final String position = getContext().getString(R.string.a11yPosition, i + 1, total);
            item.setAccessibilityDelegate(new View.AccessibilityDelegate() {
                @Override public void onInitializeAccessibilityNodeInfo(
                        View host, AccessibilityNodeInfo info) {
                    super.onInitializeAccessibilityNodeInfo(host, info);
                    CharSequence text = info.getContentDescription();
                    if (text == null || text.length() == 0) { text = info.getText(); }
                    if ((text == null || text.length() == 0) && host instanceof Spinner) {
                        Object selected = ((Spinner) host).getSelectedItem();
                        text = selected == null ? "" : selected.toString();
                    }
                    if (text != null && text.length() > 0) {
                        info.setContentDescription(text + ", " + position);
                    }
                }
            });
        }
    }

    private int controlRole(View view) {
        if (view instanceof Spinner) { return R.string.a11yControlChoice; }
        if (view instanceof SeekBar) { return R.string.a11yControlSlider; }
        if (view instanceof CompoundButton) { return R.string.a11yControlCheckbox; }
        if (view instanceof Button) { return R.string.a11yControlButton; }
        return R.string.a11yControlText;
    }

    /** The same select-by-list interaction as TalkBack's native Spinner popup. */
    private void openChoiceList(Spinner spinner) {
        if (spinner.getCount() == 0 || choosingSpinner != null) { return; }
        previousFormFocus = focus;
        choosingSpinner = spinner;
        LinearLayout choices = new LinearLayout(getContext());
        choices.setOrientation(LinearLayout.VERTICAL);
        TextView title = new TextView(getContext());
        title.setText(spinner.getPrompt());
        title.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
        choices.addView(title);
        final int selected = Math.max(0, spinner.getSelectedItemPosition());
        for (int i = 0; i < spinner.getCount(); i++) {
            final int option = i;
            Button entry = new Button(getContext());
            entry.setText(spinner.getItemAtPosition(i).toString());
            entry.setOnClickListener(v -> {
                spinner.setSelection(option);
                dismissChoiceList();
            });
            choices.addView(entry);
        }
        Button cancel = new Button(getContext());
        cancel.setText(R.string.a11yTtsCancel);
        cancel.setOnClickListener(v -> dismissChoiceList());
        choices.addView(cancel);
        showContent(choices, selected);
    }

    /** Back closes a nested choice list before leaving the settings screen. */
    public boolean dismissChoiceList() {
        if (choosingSpinner == null) { return false; }
        choosingSpinner = null;
        showContent(primaryContent, previousFormFocus);
        return true;
    }

    private void collect(View view) {
        if (view.getVisibility() != View.VISIBLE
                || view.getImportantForAccessibility() == View.IMPORTANT_FOR_ACCESSIBILITY_NO) {
            return;
        }
        if (view instanceof TextView || view instanceof SeekBar
                || view instanceof Spinner) {
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
        if (view instanceof Spinner) {
            Spinner spinner = (Spinner) view;
            Object selected = spinner.getSelectedItem();
            text = selected == null ? "" : selected.toString();
        } else if (view instanceof CompoundButton) {
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
        text += ". " + getContext().getString(controlRole(view))
                + ", " + getContext().getString(R.string.a11yPosition, focus + 1, items.size());
        if (!view.isEnabled()) {
            text += ". " + getContext().getString(R.string.a11yReaderDisabledEntry);
        }
        view.requestRectangleOnScreen(new Rect(0, 0, view.getWidth(), view.getHeight()), true);
        // Without the interruption the speech session rejects a rapid focus change.
        listener.interruptForFocus();
        listener.speak("bootstrap-focus-" + focus, text);
    }

    private void navigate(int delta) {
        if (items.isEmpty()) { return; }
        // Fixed menu rule throughout the app: right = next, left = previous.
        focus = Math.floorMod(focus + delta, items.size());
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
            navigate(dx > 0 ? 1 : -1);
        } else {
            adjustFocusedSetting(dy < 0 ? 1 : -1);
        }
    }

    private void activateFocused() {
        if (items.isEmpty()) { return; }
        View view = items.get(focus);
        if (view instanceof Spinner && view.isEnabled()) {
            // Do not silently rotate choices: display every available option.
            openChoiceList((Spinner) view);
        } else if (view.isEnabled() && view.isClickable()) {
            view.performClick();
        } else {
            speakFocused();
        }
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
        // Consume the final finger-up after a two-finger Back gesture.
        // Never reinterpret it as a one-finger swipe or click.
        if (ignoreUntilPointersReleased) {
            if (action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_CANCEL) {
                ignoreUntilPointersReleased = false;
            }
            return true;
        }
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
            if (action == MotionEvent.ACTION_POINTER_UP && pointerCount == 2
                    && event.getEventTime() - pointerDownTime
                            <= Math.max(1500L, listener.swipeDurationLimitMs())
                    && TwoFingerBackGesture.matches(event, pointerIds,
                            pointerDownX, pointerDownY, pointerCount,
                            listener.swipeDistancePx())) {
                ignoreUntilPointersReleased = true;
                multiplePointers = false;
                pointerCount = 0;
                previousTapFocus = -1;
                listener.goBack();
                return true;
            }
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
        ignoreUntilPointersReleased = false;
        touchGeneration = contentGeneration;
    }
}

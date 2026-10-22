package com.cleverraven.cataclysmdda;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.provider.Settings;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.View;
import android.view.accessibility.AccessibilityManager;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.ScrollView;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.Locale;

/** Voice setup with TalkBack controls and a self-voicing path before enabling the reader. */
public final class ReaderSetupActivity extends Activity {
    public static final String EXTRA_DISABLE_SELF_VOICE =
            "com.cleverraven.cataclysmdda.extra.DISABLE_SETUP_SELF_VOICE";

    private AndroidSpeech speech;
    private TextView status;
    private Button sample;
    private CheckBox online;
    private CheckBox leftHandedNavigation;
    private SeekBar rate;
    private TextView rateValue;
    private SeekBar swipeDistance;
    private TextView swipeDistanceValue;
    private SeekBar swipeDuration;
    private TextView swipeDurationValue;
    private Button horizontalSwipeAction;
    private Button verticalSwipeAction;
    private int horizontalSwipeActionDraft;
    private int verticalSwipeActionDraft;
    private boolean foreground;
    private boolean sampleRequested;
    private boolean sampleAttempted;
    private long sampleSequence;
    private Button heard;
    private boolean testedNetworkConsent;
    private float testedRate = 1.0f;
    private final ArrayList<View> selfVoiceControls = new ArrayList<>();
    private SelfVoicingSetupSurface selfVoicingSurface;
    private int selfVoiceFocus;
    private boolean selfVoiceInstructionsPending;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setTitle(R.string.a11yReaderSetup);
        selfVoicingSurface = new SelfVoicingSetupSurface(this);
        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        int padding = Math.round(20 * getResources().getDisplayMetrics().density);
        layout.setPadding(padding, padding, padding, padding);
        selfVoicingSurface.addView(layout);

        TextView explanation = new TextView(this);
        explanation.setText(R.string.a11yReaderSetupExplanation);
        layout.addView(explanation);
        selfVoiceControls.add(explanation);
        status = new TextView(this);
        status.setId(R.id.a11y_speech_status);
        status.setSaveEnabled(false);
        status.setText(R.string.a11ySpeechNew);
        layout.addView(status);
        selfVoiceControls.add(status);

        online = new CheckBox(this);
        online.setId(R.id.a11y_network_voice);
        online.setSaveEnabled(false);
        online.setText(R.string.a11yAllowNetworkVoice);
        // Online speech is opt-in for this test; never restore it silently.
        online.setChecked(false);
        layout.addView(online);
        selfVoiceControls.add(online);

        TextView rateLabel = new TextView(this);
        rateLabel.setText(R.string.a11ySpeechRateLabel);
        rateLabel.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
        layout.addView(rateLabel);
        rateValue = new TextView(this);
        rateValue.setId(R.id.a11y_speech_rate_value);
        rateValue.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
        layout.addView(rateValue);
        rate = new SeekBar(this);
        rate.setId(R.id.a11y_speech_rate);
        rate.setMax(25); // 0.5x through 3.0x, in 0.1x steps.
        float savedRate = new ReaderPreferences(this).getRate();
        rate.setProgress(Math.max(0, Math.min(rate.getMax(), Math.round((savedRate - 0.5f) * 10.0f))));
        rate.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                if (fromUser) { invalidateSampleForChangedSettings(); }
                updateRateDescription();
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) { }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) { }
        });
        layout.addView(rate);
        selfVoiceControls.add(rate);
        updateRateDescription();

        TextView gestureHeading = new TextView(this);
        gestureHeading.setText(R.string.a11yGestureSettingsHeading);
        layout.addView(gestureHeading);
        selfVoiceControls.add(gestureHeading);

        leftHandedNavigation = new CheckBox(this);
        leftHandedNavigation.setId(R.id.a11y_left_handed_navigation);
        leftHandedNavigation.setText(R.string.a11yLeftHandedNavigation);
        leftHandedNavigation.setChecked(new ReaderPreferences(this).isLeftHandedNavigation());
        leftHandedNavigation.setOnCheckedChangeListener((button, checked) ->
                new ReaderPreferences(this).setLeftHandedNavigation(checked));
        layout.addView(leftHandedNavigation);
        selfVoiceControls.add(leftHandedNavigation);

        TextView swipeDistanceLabel = new TextView(this);
        swipeDistanceLabel.setText(R.string.a11ySwipeDistanceLabel);
        swipeDistanceLabel.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
        layout.addView(swipeDistanceLabel);
        swipeDistanceValue = new TextView(this);
        swipeDistanceValue.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
        layout.addView(swipeDistanceValue);
        swipeDistance = gestureSettingBar(R.id.a11y_swipe_distance,
                new ReaderPreferences(this).getSwipeDistanceLevel());
        swipeDistance.setOnSeekBarChangeListener(gestureSettingListener(true));
        layout.addView(swipeDistance);
        selfVoiceControls.add(swipeDistance);

        TextView swipeDurationLabel = new TextView(this);
        swipeDurationLabel.setText(R.string.a11ySwipeDurationLabel);
        swipeDurationLabel.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
        layout.addView(swipeDurationLabel);
        swipeDurationValue = new TextView(this);
        swipeDurationValue.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
        layout.addView(swipeDurationValue);
        swipeDuration = gestureSettingBar(R.id.a11y_swipe_duration,
                new ReaderPreferences(this).getSwipeDurationLevel());
        swipeDuration.setOnSeekBarChangeListener(gestureSettingListener(false));
        layout.addView(swipeDuration);
        selfVoiceControls.add(swipeDuration);
        updateGestureDescriptions();

        TextView mappingHeading = new TextView(this);
        mappingHeading.setText(R.string.a11yGestureMappingHeading);
        layout.addView(mappingHeading);
        selfVoiceControls.add(mappingHeading);
        ReaderPreferences savedGesturePreferences = new ReaderPreferences(this);
        horizontalSwipeActionDraft = savedGesturePreferences.getHorizontalSwipeAction();
        verticalSwipeActionDraft = savedGesturePreferences.getVerticalSwipeAction();
        horizontalSwipeAction = button(layout, R.string.a11yHorizontalSwipeAction,
                                       view -> cycleSwipeAction(true));
        horizontalSwipeAction.setId(R.id.a11y_horizontal_swipe_action);
        verticalSwipeAction = button(layout, R.string.a11yVerticalSwipeAction,
                                     view -> cycleSwipeAction(false));
        verticalSwipeAction.setId(R.id.a11y_vertical_swipe_action);
        updateGestureMappingDescriptions();
        button(layout, R.string.a11yTryGestureMapping,
               view -> tryGestureMapping()).setId(R.id.a11y_try_gesture_mapping);

        button(layout, R.string.a11yRestoreGestureDefaults, view -> restoreGestureDefaults())
                .setId(R.id.a11y_restore_gesture_defaults);

        sample = button(layout, R.string.a11yTestVietnamese, view -> {
            sampleAttempted = true;
            sampleRequested = true;
            heard.setEnabled(false);
            testedNetworkConsent = online.isChecked();
            testedRate = selectedRate();
            sample.setEnabled(false);
            speech.initialize(null, new Locale("vi", "VN"), testedRate, !online.isChecked());
        });
        sample.setId(R.id.a11y_test_voice);
        sample.setSaveEnabled(false);
        selfVoiceFocus = selfVoiceControls.indexOf(sample);
        button(layout, R.string.a11yStopSpeech, view -> {
            sampleAttempted = false;
            sampleRequested = false;
            heard.setEnabled(false);
            speech.cancel();
            showStatus(R.string.a11ySpeechStopped, true);
        }).setId(R.id.a11y_stop_voice);
        button(layout, R.string.a11yAndroidVoiceSettings,
                view -> openSettings("com.android.settings.TTS_SETTINGS"));
        button(layout, R.string.showAccessibilitySettings,
                view -> openSettings(Settings.ACTION_ACCESSIBILITY_SETTINGS));
        heard = button(layout, R.string.a11yConfirmHeardEnable, view -> {
            new ReaderPreferences(this).confirmHeardAndEnable(testedNetworkConsent, testedRate);
            showStatus(R.string.a11yReaderEnabledNotice, true);
        });
        heard.setEnabled(false);
        online.setOnCheckedChangeListener((button, checked) -> invalidateSampleForChangedSettings());
        button(layout, R.string.a11yDisableInternalReader, view -> {
            new ReaderPreferences(this).disable();
            showStatus(R.string.a11yReaderDisabledNotice, true);
        });
        button(layout, R.string.a11yReturnToStartup, view -> finish()).setId(R.id.a11y_return);
        setContentView(selfVoicingSurface);

        speech = new AndroidSpeech(this, new AndroidSpeech.Listener() {
            @Override
            public void onState(AndroidSpeech.State state) {
                if (isFinishing() || isDestroyed()) {
                    return;
                }
                sample.setEnabled(state != AndroidSpeech.State.INITIALIZING);
                switch (state) {
                    case INITIALIZING:
                        showStatus(R.string.a11ySpeechInitializing, true);
                        break;
                    case READY:
                        if (sampleRequested && foreground) {
                            sampleRequested = false;
                            speech.speak("voice-test-" + (++sampleSequence), getString(R.string.a11ySpeechSample));
                        } else if (!usesSystemAccessibility() && foreground) {
                            if (selfVoiceInstructionsPending) {
                                selfVoiceInstructionsPending = false;
                                speech.speak("setup-instructions", getString(R.string.a11ySetupSelfVoiceInstructions));
                            } else {
                                speakFocusedControl();
                            }
                        } else {
                            showStatus(R.string.a11ySpeechReady, true);
                        }
                        break;
                    case MISSING_LANGUAGE:
                        sampleAttempted = false;
                        sampleRequested = false;
                        heard.setEnabled(false);
                        showStatus(R.string.a11ySpeechMissingVietnamese, true);
                        break;
                    case NETWORK_REQUIRED:
                        sampleAttempted = false;
                        sampleRequested = false;
                        heard.setEnabled(false);
                        showStatus(R.string.a11ySpeechNetworkRequired, true);
                        break;
                    case FAILED:
                        sampleAttempted = false;
                        sampleRequested = false;
                        heard.setEnabled(false);
                        showStatus(R.string.a11ySpeechFailed, true);
                        break;
                    default:
                        break;
                }
            }

            @Override
            public void onUtterance(String id, AndroidSpeech.Event event) {
                if (isFinishing() || isDestroyed()) {
                    return;
                }
                switch (event) {
                    case STARTED:
                        if (id.startsWith("voice-test-")) {
                            showStatus(R.string.a11ySpeechPlaying, false);
                        }
                        break;
                    case DONE:
                        if ("setup-instructions".equals(id)) {
                            speakFocusedControl();
                        } else if (id.startsWith("voice-test-")) {
                            heard.setEnabled(true);
                            if (!usesSystemAccessibility()) {
                                status.setText(R.string.a11ySpeechFinished);
                                selfVoiceFocus = selfVoiceControls.indexOf(heard);
                                scrollFocusedControlIntoView();
                                speakText("sample-finished", getString(R.string.a11ySpeechFinished) + " "
                                          + getString(R.string.a11yConfirmHeardEnable));
                            } else {
                                showStatus(R.string.a11ySpeechFinished, true);
                            }
                        }
                        break;
                    case ERROR:
                    case REJECTED:
                        if (id.startsWith("voice-test-")) {
                            sampleAttempted = false;
                            heard.setEnabled(false);
                            showStatus(R.string.a11ySpeechFailed, true);
                        } else if (!usesSystemAccessibility()) {
                            status.setText(R.string.a11ySpeechFailed);
                            status.performHapticFeedback(android.view.HapticFeedbackConstants.LONG_PRESS);
                        }
                        break;
                    default:
                        break;
                }
            }
        });
    }

    private Button button(LinearLayout layout, int label, View.OnClickListener listener) {
        Button result = new Button(this);
        result.setText(label);
        result.setOnClickListener(listener);
        layout.addView(result);
        selfVoiceControls.add(result);
        return result;
    }

    private SeekBar gestureSettingBar(int id, int progress) {
        SeekBar result = new SeekBar(this);
        result.setId(id);
        result.setMax(2);
        result.setProgress(progress);
        return result;
    }

    private SeekBar.OnSeekBarChangeListener gestureSettingListener(boolean distance) {
        return new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                ReaderPreferences preferences = new ReaderPreferences(ReaderSetupActivity.this);
                if (distance) { preferences.setSwipeDistanceLevel(progress); }
                else { preferences.setSwipeDurationLevel(progress); }
                updateGestureDescriptions();
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) { }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) { }
        };
    }

    private void updateGestureDescriptions() {
        if (swipeDistance == null || swipeDistanceValue == null
                || swipeDuration == null || swipeDurationValue == null) { return; }
        int distanceLabel = swipeDistance.getProgress() == 0 ? R.string.a11ySwipeDistanceShort
                : swipeDistance.getProgress() == 2 ? R.string.a11ySwipeDistanceLong
                : R.string.a11ySwipeDistanceStandard;
        int durationLabel = swipeDuration.getProgress() == 0 ? R.string.a11ySwipeDurationQuick
                : swipeDuration.getProgress() == 2 ? R.string.a11ySwipeDurationDeliberate
                : R.string.a11ySwipeDurationStandard;
        String distanceDescription = getString(R.string.a11ySwipeDistanceValue,
                getString(distanceLabel));
        String durationDescription = getString(R.string.a11ySwipeDurationValue,
                getString(durationLabel));
        swipeDistanceValue.setText(distanceDescription);
        swipeDistance.setContentDescription(distanceDescription);
        swipeDurationValue.setText(durationDescription);
        swipeDuration.setContentDescription(durationDescription);
    }

    private void updateGestureMappingDescriptions() {
        if (horizontalSwipeAction == null || verticalSwipeAction == null) { return; }
        String horizontal = gestureActionName(horizontalSwipeActionDraft);
        String vertical = gestureActionName(verticalSwipeActionDraft);
        horizontalSwipeAction.setText(getString(R.string.a11yHorizontalSwipeAction, horizontal));
        horizontalSwipeAction.setContentDescription(horizontalSwipeAction.getText());
        verticalSwipeAction.setText(getString(R.string.a11yVerticalSwipeAction, vertical));
        verticalSwipeAction.setContentDescription(verticalSwipeAction.getText());
    }

    private String gestureActionName(int action) {
        int label;
        switch (action) {
            case ReaderPreferences.GESTURE_ACTION_DETAIL:
                label = R.string.a11yGestureActionDetail;
                break;
            case ReaderPreferences.GESTURE_ACTION_READ_CURRENT:
                label = R.string.a11yGestureActionReadCurrent;
                break;
            case ReaderPreferences.GESTURE_ACTION_NONE:
                label = R.string.a11yGestureActionNone;
                break;
            default:
                label = R.string.a11yGestureActionNavigate;
                break;
        }
        return getString(label);
    }

    private void cycleSwipeAction(boolean horizontal) {
        int current = horizontal ? horizontalSwipeActionDraft : verticalSwipeActionDraft;
        int other = horizontal ? verticalSwipeActionDraft : horizontalSwipeActionDraft;
        int next = ( current + 1 ) % 4;
        if (next != ReaderPreferences.GESTURE_ACTION_NAVIGATE
                && other != ReaderPreferences.GESTURE_ACTION_NAVIGATE) {
            showStatus(R.string.a11yGestureMappingNeedsNavigation, true);
            return;
        }
        if (horizontal) { horizontalSwipeActionDraft = next; }
        else { verticalSwipeActionDraft = next; }
        updateGestureMappingDescriptions();
        String value = horizontal ? horizontalSwipeAction.getText().toString()
                      : verticalSwipeAction.getText().toString();
        showStatusText(value, true);
    }

    private void tryGestureMapping() {
        if (isTouchExplorationActive()) {
            showStatus(R.string.a11yGestureMappingTalkBack, true);
            return;
        }
        new ReaderPreferences(this).beginGestureMapTrial(horizontalSwipeActionDraft,
                verticalSwipeActionDraft);
        finish();
    }

    private void restoreGestureDefaults() {
        new ReaderPreferences(this).restoreGestureDefaults();
        leftHandedNavigation.setChecked(false);
        swipeDistance.setProgress(1);
        swipeDuration.setProgress(1);
        horizontalSwipeActionDraft = ReaderPreferences.GESTURE_ACTION_NAVIGATE;
        verticalSwipeActionDraft = ReaderPreferences.GESTURE_ACTION_NAVIGATE;
        updateGestureMappingDescriptions();
        updateGestureDescriptions();
        showStatus(R.string.a11yGestureDefaultsRestored, true);
    }

    private boolean usesSystemAccessibility() {
        if (getIntent().getBooleanExtra(EXTRA_DISABLE_SELF_VOICE, false)) { return true; }
        AccessibilityManager manager = (AccessibilityManager) getSystemService(Context.ACCESSIBILITY_SERVICE);
        return manager != null && manager.isEnabled();
    }

    private boolean isTouchExplorationActive() {
        AccessibilityManager manager = (AccessibilityManager) getSystemService(Context.ACCESSIBILITY_SERVICE);
        return manager != null && manager.isEnabled() && manager.isTouchExplorationEnabled();
    }

    private void speakText(String id, String text) {
        if (!foreground || usesSystemAccessibility() || speech == null
                || speech.getState() != AndroidSpeech.State.READY) {
            return;
        }
        speech.speak(id, text);
    }

    private void speakFocusedControl() {
        if (selfVoiceControls.isEmpty()) { return; }
        selfVoiceFocus = Math.floorMod(selfVoiceFocus, selfVoiceControls.size());
        View focused = selfVoiceControls.get(selfVoiceFocus);
        String text;
        if (focused instanceof CheckBox) {
            CheckBox checkBox = (CheckBox) focused;
            text = checkBox.getText() + ". "
                   + getString(checkBox.isChecked() ? R.string.a11yChecked : R.string.a11yUnchecked);
        } else if (focused == rate) {
            text = rate.getContentDescription().toString();
        } else if (focused instanceof SeekBar) {
            text = focused.getContentDescription().toString();
        } else if (focused.getContentDescription() != null) {
            text = focused.getContentDescription().toString();
        } else if (focused instanceof TextView) {
            text = ( ( TextView )focused ).getText().toString();
        } else {
            text = focused.toString();
        }
        if (!focused.isEnabled()) { text += ". " + getString(R.string.a11yReaderDisabledEntry); }
        scrollFocusedControlIntoView();
        speakText("setup-focus-" + selfVoiceFocus, text);
    }

    private void scrollFocusedControlIntoView() {
        if (selfVoicingSurface != null && !selfVoiceControls.isEmpty()) {
            selfVoicingSurface.post(() -> selfVoicingSurface.smoothScrollTo(0,
                                      selfVoiceControls.get(Math.floorMod(selfVoiceFocus,
                                              selfVoiceControls.size())).getTop()));
        }
    }

    private void navigateSelfVoice(int delta) {
        if (selfVoiceControls.isEmpty()) { return; }
        int direction = new ReaderPreferences(this).isLeftHandedNavigation() ? -delta : delta;
        selfVoiceFocus = Math.floorMod(selfVoiceFocus + direction, selfVoiceControls.size());
        speakFocusedControl();
    }

    private void adjustSelfVoiceSetting(int delta) {
        if (selfVoiceControls.isEmpty()) { return; }
        View focused = selfVoiceControls.get(selfVoiceFocus);
        if (focused == rate) {
            int next = Math.max(0, Math.min(rate.getMax(), rate.getProgress() + delta));
            if (next == rate.getProgress()) { speakFocusedControl(); return; }
            boolean needsRetest = sampleAttempted;
            invalidateSampleForChangedSettings();
            rate.setProgress(next);
            if (needsRetest) {
                speakText("setup-rate-retest", rate.getContentDescription() + ". "
                          + getString(R.string.a11yRetestAfterSettingsChange));
            } else {
                speakFocusedControl();
            }
            return;
        }
        if (focused == swipeDistance || focused == swipeDuration) {
            SeekBar setting = (SeekBar) focused;
            int next = Math.max(0, Math.min(setting.getMax(), setting.getProgress() + delta));
            setting.setProgress(next);
            speakFocusedControl();
        }
    }

    private void activateSelfVoiceControl() {
        if (selfVoiceControls.isEmpty()) { return; }
        View focused = selfVoiceControls.get(Math.floorMod(selfVoiceFocus, selfVoiceControls.size()));
        if (!focused.isEnabled() || focused instanceof SeekBar || focused instanceof TextView) {
            speakFocusedControl();
        } else {
            boolean needsRetest = sampleAttempted && focused == online;
            focused.performClick();
            if (focused instanceof CheckBox) {
                CheckBox checkBox = (CheckBox) focused;
                String text = checkBox.getText() + ". "
                              + getString(checkBox.isChecked() ? R.string.a11yChecked : R.string.a11yUnchecked);
                if (needsRetest) { text += ". " + getString(R.string.a11yRetestAfterSettingsChange); }
                speakText("setup-network-setting", text);
            }
        }
    }

    private void stopSelfVoice() {
        if (speech != null) { speech.cancel(); }
        status.setText(R.string.a11ySpeechStopped);
        status.performHapticFeedback(android.view.HapticFeedbackConstants.LONG_PRESS);
    }

    private final class SelfVoicingSetupSurface extends ScrollView {
        private final GestureDetector gestures;
        private boolean multiplePointers;
        private boolean activateOnDoubleTap;
        private boolean swipeHandled;
        private boolean multiplePointerMoved;
        private final int[] multiplePointerIds = new int[5];
        private final float[] multiplePointerDownX = new float[5];
        private final float[] multiplePointerDownY = new float[5];
        private int multiplePointerCount;
        private long pointerDownTime;
        private float downX;
        private float downY;
        private int downFocus = -1;
        private int previousTapFocus = -1;

        SelfVoicingSetupSurface(Context context) {
            super(context);
            gestures = new GestureDetector(context, new GestureDetector.SimpleOnGestureListener() {
                @Override
                public boolean onDown(MotionEvent event) { return true; }

                @Override
                public boolean onSingleTapConfirmed(MotionEvent event) {
                    speakFocusedControl();
                    return true;
                }

                @Override
                public boolean onDoubleTap(MotionEvent event) {
                    activateOnDoubleTap = previousTapFocus >= 0 && previousTapFocus == downFocus;
                    return true;
                }

                @Override
                public boolean onDoubleTapEvent(MotionEvent event) {
                    if (activateOnDoubleTap && event.getActionMasked() == MotionEvent.ACTION_UP
                            && selfVoiceFocus == downFocus) {
                        activateSelfVoiceControl();
                    }
                    return true;
                }

                @Override
                public boolean onFling(MotionEvent start, MotionEvent end, float velocityX, float velocityY) {
                    if (start == null) { return true; }
                    float dx = end.getX() - start.getX();
                    float dy = end.getY() - start.getY();
                    if (Math.max(Math.abs(dx), Math.abs(dy)) < selfVoiceSwipeDistancePx()) {
                        return true;
                    }
                    if (end.getEventTime() - pointerDownTime > selfVoiceSwipeDurationLimitMs()) {
                        swipeHandled = true;
                        return true;
                    }
                    swipeHandled = true;
                    previousTapFocus = -1;
                    if (Math.abs(dx) > Math.abs(dy)) {
                        navigateSelfVoice(dx < 0 ? 1 : -1);
                    } else {
                        adjustSelfVoiceSetting(dy < 0 ? 1 : -1);
                    }
                    return true;
                }
            });
        }

        private float selfVoiceSwipeDistancePx() {
            return new ReaderPreferences(ReaderSetupActivity.this).getSwipeDistanceDp()
                    * ReaderSetupActivity.this.getResources().getDisplayMetrics().density;
        }

        private long selfVoiceSwipeDurationLimitMs() {
            return new ReaderPreferences(ReaderSetupActivity.this).getSwipeDurationLimitMs();
        }

        @Override
        public boolean dispatchTouchEvent(MotionEvent event) {
            if (usesSystemAccessibility()) { return super.dispatchTouchEvent(event); }
            int action = event.getActionMasked();
            if (action == MotionEvent.ACTION_DOWN) {
                multiplePointers = false;
                pointerDownTime = event.getEventTime();
                downX = event.getX();
                downY = event.getY();
                downFocus = selfVoiceFocus;
                activateOnDoubleTap = false;
                swipeHandled = false;
            } else if (action == MotionEvent.ACTION_POINTER_DOWN) {
                multiplePointers = true;
                multiplePointerMoved = false;
                multiplePointerCount = Math.min(event.getPointerCount(), multiplePointerIds.length);
                for (int i = 0; i < multiplePointerCount; i++) {
                    multiplePointerIds[i] = event.getPointerId(i);
                    multiplePointerDownX[i] = event.getX(i);
                    multiplePointerDownY[i] = event.getY(i);
                }
                previousTapFocus = -1;
                MotionEvent cancel = MotionEvent.obtain(event);
                cancel.setAction(MotionEvent.ACTION_CANCEL);
                gestures.onTouchEvent(cancel);
                cancel.recycle();
            }
            if (multiplePointers) {
                for (int i = 0; i < event.getPointerCount(); i++) {
                    int pointerId = event.getPointerId(i);
                    for (int saved = 0; saved < multiplePointerCount; saved++) {
                        if (multiplePointerIds[saved] == pointerId
                                && Math.hypot(event.getX(i) - multiplePointerDownX[saved],
                                              event.getY(i) - multiplePointerDownY[saved])
                                > selfVoiceSwipeDistancePx()) {
                            multiplePointerMoved = true;
                            break;
                        }
                    }
                }
                if (action == MotionEvent.ACTION_UP) {
                    if (!multiplePointerMoved
                            && event.getEventTime() - pointerDownTime <= selfVoiceSwipeDurationLimitMs()) {
                        stopSelfVoice();
                    }
                    multiplePointers = false;
                    multiplePointerCount = 0;
                }
                return true;
            }
            gestures.onTouchEvent(event);
            if (action == MotionEvent.ACTION_UP) {
                float dx = event.getX() - downX;
                float dy = event.getY() - downY;
                boolean enoughDistance = Math.max(Math.abs(dx), Math.abs(dy)) >= selfVoiceSwipeDistancePx();
                boolean shortEnough = event.getEventTime() - pointerDownTime <= selfVoiceSwipeDurationLimitMs();
                if (enoughDistance && shortEnough && !swipeHandled && !activateOnDoubleTap) {
                    swipeHandled = true;
                    previousTapFocus = -1;
                    if (Math.abs(dx) > Math.abs(dy)) {
                        navigateSelfVoice(dx < 0 ? 1 : -1);
                    } else {
                        adjustSelfVoiceSetting(dy < 0 ? 1 : -1);
                    }
                }
                if (enoughDistance || activateOnDoubleTap || !shortEnough) {
                    previousTapFocus = -1;
                } else {
                    previousTapFocus = downFocus;
                }
                activateOnDoubleTap = false;
            }
            return true;
        }
    }

    private float selectedRate() {
        return ( 5 + rate.getProgress() ) / 10.0f;
    }

    private void updateRateDescription() {
        if (rate == null || rateValue == null) { return; }
        String value = String.format(Locale.getDefault(), "%.1f", selectedRate());
        String description = getString(R.string.a11ySpeechRateValue, value);
        rateValue.setText(description);
        rate.setContentDescription(description);
    }

    private void invalidateSampleForChangedSettings() {
        if (!sampleAttempted) { return; }
        sampleAttempted = false;
        sampleRequested = false;
        if (heard != null) { heard.setEnabled(false); }
        if (speech != null) { speech.cancel(); }
        showStatus(R.string.a11yRetestAfterSettingsChange, true);
    }

    private void showStatus(int text, boolean announce) {
        status.setText(text);
        if (foreground && announce) {
            if (usesSystemAccessibility()) {
                status.announceForAccessibility(getString(text));
            } else if (speech != null && speech.getState() == AndroidSpeech.State.READY) {
                speakText("setup-status-" + text, getString(text));
            } else {
                status.performHapticFeedback(android.view.HapticFeedbackConstants.LONG_PRESS);
            }
        }
    }

    private void showStatusText(String text, boolean announce) {
        status.setText(text);
        if (foreground && announce) {
            if (usesSystemAccessibility()) {
                status.announceForAccessibility(text);
            } else if (speech != null && speech.getState() == AndroidSpeech.State.READY) {
                speakText("setup-status-custom-" + (++sampleSequence), text);
            } else {
                status.performHapticFeedback(android.view.HapticFeedbackConstants.LONG_PRESS);
            }
        }
    }

    private void openSettings(String action) {
        sampleAttempted = false;
        sampleRequested = false;
        heard.setEnabled(false);
        speech.cancel();
        try {
            startActivity(new Intent(action));
        } catch (ActivityNotFoundException missing) {
            try {
                startActivity(new Intent(Settings.ACTION_SETTINGS));
            } catch (ActivityNotFoundException unavailable) {
                showStatus(R.string.a11ySettingsUnavailable, true);
            }
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        foreground = true;
        if (!usesSystemAccessibility()) {
            AndroidSpeech.State state = speech.getState();
            if (state == AndroidSpeech.State.NEW || state == AndroidSpeech.State.FAILED
                    || state == AndroidSpeech.State.MISSING_LANGUAGE
                    || state == AndroidSpeech.State.NETWORK_REQUIRED) {
                selfVoiceInstructionsPending = true;
                speech.initialize(null, new Locale("vi", "VN"), new ReaderPreferences(this).getRate(), true);
            } else if (state == AndroidSpeech.State.READY) {
                speakFocusedControl();
            }
        }
    }

    @Override
    protected void onPause() {
        foreground = false;
        sampleRequested = false;
        sampleAttempted = false;
        heard.setEnabled(false);
        if (speech != null) {
            speech.cancel();
        }
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        if (speech != null) {
            speech.close();
        }
        super.onDestroy();
    }
}

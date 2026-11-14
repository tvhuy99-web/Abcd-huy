package com.cleverraven.cataclysmdda;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.view.View;
import android.view.accessibility.AccessibilityManager;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.TextView;
import java.util.Locale;

/** Reader graphics, directional sonar and swipe settings; TTS is separate. */
public final class AccessibilitySettingsActivity extends Activity {
    @Override protected void onCreate(Bundle state) {
        super.onCreate(state);
        setTitle(R.string.a11yAccessibilitySettingsTitle);
        initSurface();
        ReaderPreferences prefs = new ReaderPreferences(this);
        LinearLayout controls = new LinearLayout(this);
        controls.setOrientation(LinearLayout.VERTICAL);
        int padding = Math.round(16 * getResources().getDisplayMetrics().density);
        controls.setPadding(padding, padding, padding, padding);

        CheckBox showGraphics = new CheckBox(this);
        showGraphics.setText(R.string.a11yWorldGraphics);
        showGraphics.setChecked(!prefs.isSpokenOnlyWorldEnabled());
        showGraphics.setOnCheckedChangeListener((button, checked) -> {
            boolean spokenOnly = !checked;
            prefs.setSpokenOnlyWorldEnabled(spokenOnly);
            SessionDebugLog.event("WORLD_VISUAL_STATE", "spoken_only=" + spokenOnly);
            try {
                CataclysmDDA.nativeConfigureReaderPerformance(
                        spokenOnly, SessionDebugLog.enabled(this));
            } catch (UnsatisfiedLinkError unavailable) {
                // The active reader reapplies preferences when gameplay resumes.
            }
        });
        controls.addView(showGraphics);
        TextView graphicsHelp = new TextView(this);
        graphicsHelp.setText(R.string.a11yWorldGraphicsExplanation);
        controls.addView(graphicsHelp);

        CheckBox directionalSonar = new CheckBox(this);
        directionalSonar.setText(R.string.a11yDirectionalSonar);
        directionalSonar.setChecked(prefs.isDirectionalSonarEnabled());
        directionalSonar.setOnCheckedChangeListener((button, checked) -> {
            prefs.setDirectionalSonarEnabled(checked);
            try {
                CataclysmDDA.nativeConfigureDirectionalSonar(checked);
            } catch (UnsatisfiedLinkError unavailable) {
                // Reapplied by the game reader when gameplay resumes.
            }
        });
        controls.addView(directionalSonar);
        TextView sonarHelp = new TextView(this);
        sonarHelp.setText(R.string.a11yDirectionalSonarExplanation);
        controls.addView(sonarHelp);

        TextView distanceValue = new TextView(this);
        // Visible label only: the SeekBar below announces the same value
        // through its contentDescription, so this must not be a second
        // spoken item in the navigation order.
        distanceValue.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
        controls.addView(distanceValue);
        SeekBar distance = new SeekBar(this);
        distance.setMax(2);
        distance.setProgress(prefs.getSwipeDistanceLevel());
        distance.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar bar, int level, boolean fromUser) {
                if (fromUser) { prefs.setSwipeDistanceLevel(level); }
                String choice = getString(new int[] { R.string.a11ySwipeDistanceShort,
                        R.string.a11ySwipeDistanceStandard,
                        R.string.a11ySwipeDistanceLong }[level]);
                String description = getString(R.string.a11ySwipeDistanceValue, choice);
                distanceValue.setText(description);
                bar.setContentDescription(description);
            }
            @Override public void onStartTrackingTouch(SeekBar bar) { }
            @Override public void onStopTrackingTouch(SeekBar bar) { }
        });
        controls.addView(distance);
        String distanceChoice = getString(new int[] { R.string.a11ySwipeDistanceShort,
                R.string.a11ySwipeDistanceStandard,
                R.string.a11ySwipeDistanceLong }[distance.getProgress()]);
        distanceValue.setText(getString(R.string.a11ySwipeDistanceValue, distanceChoice));
        distance.setContentDescription(distanceValue.getText());

        TextView durationValue = new TextView(this);
        // Visible label only: the SeekBar below announces the same value
        // through its contentDescription, so this must not be a second
        // spoken item in the navigation order.
        durationValue.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
        controls.addView(durationValue);
        SeekBar duration = new SeekBar(this);
        duration.setMax(2);
        duration.setProgress(prefs.getSwipeDurationLevel());
        duration.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar bar, int level, boolean fromUser) {
                if (fromUser) { prefs.setSwipeDurationLevel(level); }
                String choice = getString(new int[] { R.string.a11ySwipeDurationQuick,
                        R.string.a11ySwipeDurationStandard,
                        R.string.a11ySwipeDurationDeliberate }[level]);
                String description = getString(R.string.a11ySwipeDurationValue, choice);
                durationValue.setText(description);
                bar.setContentDescription(description);
            }
            @Override public void onStartTrackingTouch(SeekBar bar) { }
            @Override public void onStopTrackingTouch(SeekBar bar) { }
        });
        controls.addView(duration);
        String durationChoice = getString(new int[] { R.string.a11ySwipeDurationQuick,
                R.string.a11ySwipeDurationStandard,
                R.string.a11ySwipeDurationDeliberate }[duration.getProgress()]);
        durationValue.setText(getString(R.string.a11ySwipeDurationValue, durationChoice));
        duration.setContentDescription(durationValue.getText());

        Button back = new Button(this);
        back.setText(R.string.a11yReaderReturn);
        back.setOnClickListener(view -> finish());
        controls.addView(back);
        setContentView(surface);
        surface.setContent(controls);
        initSpeech();
    }

    private BootstrapSelfVoiceSurface surface;
    private AndroidSpeech speech;
    private boolean foreground;
    private long utterance;

    private void initSurface() {
        surface = new BootstrapSelfVoiceSurface(this, new BootstrapSelfVoiceSurface.Listener() {
            @Override public boolean isActive() { return foreground && !usesTalkBack(); }
            @Override public void speak(String id, String text) { speakText(text); }
            @Override public void stopSpeech() { if (speech != null) { speech.cancel(); } }
            @Override public void goBack() { finish(); }
            @Override public float swipeDistancePx() {
                return new ReaderPreferences(AccessibilitySettingsActivity.this).getSwipeDistanceDp()
                        * getResources().getDisplayMetrics().density;
            }
            @Override public long swipeDurationLimitMs() {
                return new ReaderPreferences(AccessibilitySettingsActivity.this).getSwipeDurationLimitMs();
            }
        });
    }

    private void initSpeech() {
        speech = new AndroidSpeech(this, new AndroidSpeech.Listener() {
            @Override public void onState(AndroidSpeech.State state) {
                if (state == AndroidSpeech.State.READY && foreground && !usesTalkBack()) {
                    surface.speakFocused();
                }
            }
            @Override public void onUtterance(String id, AndroidSpeech.Event event) { }
        });
    }

    private boolean usesTalkBack() {
        AccessibilityManager manager = (AccessibilityManager)
                getSystemService(Context.ACCESSIBILITY_SERVICE);
        return manager != null && manager.isTouchExplorationEnabled();
    }

    private void speakText(String text) {
        if (foreground && !usesTalkBack() && speech != null
                && speech.getState() == AndroidSpeech.State.READY) {
            speech.speak("settings-" + (++utterance), text);
        }
    }

    @Override protected void onResume() {
        super.onResume();
        foreground = true;
        ReaderPreferences preferences = new ReaderPreferences(this);
        if (speech != null && preferences.isEnabled()) {
            speech.initialize(preferences.getSpeechEngineName(),
                    Locale.forLanguageTag(preferences.getSpeechLanguageTag()),
                    preferences.getRate(), !preferences.permitsNetworkVoice(),
                    preferences.getSpeechVoiceName(), preferences.getPitch(),
                    preferences.getVolume());
        }
    }

    @Override protected void onPause() {
        foreground = false;
        if (speech != null) { speech.cancel(); }
        super.onPause();
    }

    @Override protected void onDestroy() {
        if (speech != null) { speech.close(); }
        super.onDestroy();
    }

}

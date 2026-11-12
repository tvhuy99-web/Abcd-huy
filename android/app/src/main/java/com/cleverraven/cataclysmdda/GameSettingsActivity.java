package com.cleverraven.cataclysmdda;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.view.accessibility.AccessibilityManager;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;
import java.util.Locale;

/** Additive Android settings hub. Native Options pages are not modified. */
public final class GameSettingsActivity extends Activity {
    @Override protected void onCreate(Bundle state) {
        super.onCreate(state);
        setTitle(R.string.a11yGameSettingsTitle);
        initSurface();
        LinearLayout controls = new LinearLayout(this);
        controls.setOrientation(LinearLayout.VERTICAL);
        int padding = Math.round(16 * getResources().getDisplayMetrics().density);
        controls.setPadding(padding, padding, padding, padding);
        TextView explanation = new TextView(this);
        explanation.setText(R.string.a11yGameSettingsExplanation);
        controls.addView(explanation);
        button(controls, R.string.a11yHubTts, TtsSettingsActivity.class);
        button(controls, R.string.a11yAccessibilitySettingsTitle, AccessibilitySettingsActivity.class);
        button(controls, R.string.a11yBackupRestoreTitle, BackupRestoreActivity.class);
        Button back = new Button(this);
        back.setText(R.string.a11yReaderReturn);
        back.setOnClickListener(view -> finish());
        controls.addView(back);
        setContentView(surface);
        surface.setContent(controls);
        initSpeech();
    }

    private void button(LinearLayout controls, int label, Class<? extends Activity> activity) {
        Button button = new Button(this);
        button.setText(label);
        button.setOnClickListener(view -> startActivity(new Intent(this, activity)));
        controls.addView(button);
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
            @Override public boolean leftHandedNavigation() {
                return new ReaderPreferences(GameSettingsActivity.this).isLeftHandedNavigation();
            }
            @Override public float swipeDistancePx() {
                return new ReaderPreferences(GameSettingsActivity.this).getSwipeDistanceDp()
                        * getResources().getDisplayMetrics().density;
            }
            @Override public long swipeDurationLimitMs() {
                return new ReaderPreferences(GameSettingsActivity.this).getSwipeDurationLimitMs();
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

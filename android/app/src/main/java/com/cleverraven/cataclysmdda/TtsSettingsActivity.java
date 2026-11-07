package com.cleverraven.cataclysmdda;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.os.Bundle;
import android.provider.Settings;
import android.view.View;
import android.view.accessibility.AccessibilityManager;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.Locale;

/**
 * Dedicated, self-voiced TTS settings. All edits are drafts until Save.
 * A successful preview is required before enabling a changed voice.
 */
public final class TtsSettingsActivity extends Activity {
    private final ArrayList<AndroidSpeech.EngineOption> engines = new ArrayList<>();
    private final ArrayList<AndroidSpeech.VoiceOption> voices = new ArrayList<>();
    private AndroidSpeech speech;
    private BootstrapSelfVoiceSurface surface;
    private RescueAudioPlayer rescue;
    private CheckBox enabled;
    private CheckBox online;
    private Button engineButton;
    private Button languageButton;
    private Button voiceButton;
    private Button sampleButton;
    private SeekBar rate;
    private SeekBar pitch;
    private SeekBar volume;
    private TextView rateText;
    private TextView pitchText;
    private TextView volumeText;
    private TextView status;
    private String engineName;
    private String engineLabel;
    private String voiceName;
    private String languageTag;
    private boolean previewRequested;
    private boolean previewPassed;
    private boolean probeRunning;
    private boolean foreground;
    private int probeGeneration;
    private long spokenSequence;

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        setTitle(R.string.a11yTtsSettingsTitle);
        ReaderPreferences saved = new ReaderPreferences(this);
        engineName = saved.getSpeechEngineName();
        voiceName = saved.getSpeechVoiceName();
        languageTag = saved.getSpeechLanguageTag();

        surface = new BootstrapSelfVoiceSurface(this, new BootstrapSelfVoiceSurface.Listener() {
            @Override
            public boolean isActive() { return foreground && !usesTalkBack(); }

            @Override
            public void speak(String id, String text) { speakLocal(text); }

            @Override
            public void stopSpeech() {
                if (speech != null) { speech.cancel(); }
            }

            @Override
            public boolean leftHandedNavigation() {
                return new ReaderPreferences(TtsSettingsActivity.this).isLeftHandedNavigation();
            }

            @Override
            public float swipeDistancePx() {
                return new ReaderPreferences(TtsSettingsActivity.this).getSwipeDistanceDp()
                        * getResources().getDisplayMetrics().density;
            }

            @Override
            public long swipeDurationLimitMs() {
                return new ReaderPreferences(TtsSettingsActivity.this).getSwipeDurationLimitMs();
            }
        });

        LinearLayout column = new LinearLayout(this);
        column.setOrientation(LinearLayout.VERTICAL);
        int padding = Math.round(16 * getResources().getDisplayMetrics().density);
        column.setPadding(padding, padding, padding, padding);

        // Must be the very first control and is not committed until Save.
        enabled = new CheckBox(this);
        enabled.setText(R.string.a11yTtsEnable);
        enabled.setChecked(saved.isEnabled());
        enabled.setOnCheckedChangeListener((view, checked) -> invalidatePreview());
        column.addView(enabled);

        engineButton = button(column, R.string.a11yTtsEngine, view -> cycleEngine());
        languageButton = button(column, R.string.a11yTtsLanguage, view -> cycleLanguage());
        voiceButton = button(column, R.string.a11yTtsVoice, view -> cycleVoice());

        rateText = label(column, R.string.a11yTtsRate);
        rate = slider(column, 25, Math.round((saved.getRate() - 0.5f) * 10.0f));
        pitchText = label(column, R.string.a11yTtsPitch);
        pitch = slider(column, 15, Math.round((saved.getPitch() - 0.5f) * 10.0f));
        volumeText = label(column, R.string.a11yTtsVolume);
        volume = slider(column, 20, Math.round(saved.getVolume() * 20.0f));
        SeekBar.OnSeekBarChangeListener changed = new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar bar, int value, boolean fromUser) {
                updateSliders();
                // Gesture-controlled SeekBar updates may report fromUser=false.
                // Every changed value invalidates the previous voice preview.
                invalidatePreview();
            }

            @Override
            public void onStartTrackingTouch(SeekBar bar) { }

            @Override
            public void onStopTrackingTouch(SeekBar bar) { }
        };
        rate.setOnSeekBarChangeListener(changed);
        pitch.setOnSeekBarChangeListener(changed);
        volume.setOnSeekBarChangeListener(changed);
        updateSliders();

        online = new CheckBox(this);
        online.setText(R.string.a11yTtsAllowNetwork);
        online.setChecked(saved.permitsNetworkVoice());
        online.setOnCheckedChangeListener((view, checked) -> invalidatePreview());
        column.addView(online);

        status = label(column, R.string.a11yTtsLoading);
        button(column, R.string.a11yTtsRestoreDefaults, view -> restoreDefaults());
        button(column, R.string.a11yAndroidVoiceSettings, view -> openAndroidSettings());
        button(column, R.string.a11yTtsCancel, view -> finish());
        sampleButton = button(column, R.string.a11yTtsSample, view -> testVoice());
        button(column, R.string.a11yTtsSave, view -> saveSettings());

        setContentView(surface);
        surface.setContent(column);
        speech = new AndroidSpeech(this, new AndroidSpeech.Listener() {
            @Override
            public void onState(AndroidSpeech.State newState) {
                if (isFinishing() || isDestroyed()) { return; }
                if (newState == AndroidSpeech.State.READY) {
                    if (previewRequested) {
                        previewRequested = false;
                        speech.speak("tts-preview", getString(R.string.a11ySpeechSample));
                    } else if (foreground && !usesTalkBack()) {
                        surface.speakFocused();
                    }
                } else if (newState == AndroidSpeech.State.FAILED
                        || newState == AndroidSpeech.State.MISSING_LANGUAGE
                        || newState == AndroidSpeech.State.MISSING_VOICE
                        || newState == AndroidSpeech.State.NETWORK_REQUIRED) {
                    previewRequested = false;
                    previewPassed = false;
                    status.setText(R.string.a11yTtsUnavailable);
                    if (foreground && !usesTalkBack()) {
                        if (rescue == null) { rescue = new RescueAudioPlayer(TtsSettingsActivity.this); }
                        rescue.play(RescueAudioPlayer.CLIP_TTS_FAILED);
                    }
                }
            }

            @Override
            public void onUtterance(String id, AndroidSpeech.Event event) {
                if (!"tts-preview".equals(id) || isFinishing() || isDestroyed()) { return; }
                if (event == AndroidSpeech.Event.DONE) {
                    previewPassed = true;
                    status.setText(R.string.a11yTtsTestComplete);
                } else if (event == AndroidSpeech.Event.ERROR
                        || event == AndroidSpeech.Event.REJECTED) {
                    previewPassed = false;
                    status.setText(R.string.a11yTtsUnavailable);
                }
            }
        });
        startProbe();
    }

    private Button button(LinearLayout parent, int label, View.OnClickListener action) {
        Button result = new Button(this);
        result.setText(label);
        result.setOnClickListener(action);
        parent.addView(result);
        return result;
    }

    private TextView label(LinearLayout parent, int label) {
        TextView result = new TextView(this);
        result.setText(label);
        result.setTextSize(17.0f);
        parent.addView(result);
        return result;
    }

    private SeekBar slider(LinearLayout parent, int max, int progress) {
        SeekBar result = new SeekBar(this);
        result.setMax(max);
        result.setProgress(Math.max(0, Math.min(max, progress)));
        parent.addView(result);
        return result;
    }

    private float selectedRate() { return (rate.getProgress() + 5) / 10.0f; }
    private float selectedPitch() { return (pitch.getProgress() + 5) / 10.0f; }
    private float selectedVolume() { return volume.getProgress() / 20.0f; }

    private void updateSliders() {
        String r = getString(R.string.a11yTtsRateValue,
                String.format(Locale.getDefault(), "%.1f", selectedRate()));
        String p = getString(R.string.a11yTtsPitchValue,
                String.format(Locale.getDefault(), "%.1f", selectedPitch()));
        String v = getString(R.string.a11yTtsVolumeValue, volume.getProgress() * 5);
        rateText.setText(r);
        pitchText.setText(p);
        volumeText.setText(v);
        rate.setContentDescription(r);
        pitch.setContentDescription(p);
        volume.setContentDescription(v);
    }

    private void invalidatePreview() {
        previewPassed = false;
        previewRequested = false;
        if (speech != null) { speech.cancel(); }
    }

    private void startProbe() {
        final int generation = ++probeGeneration;
        probeRunning = true;
        updateChoices();
        AndroidSpeech.probeVoices(this, engineName, (foundEngines, foundVoices, success) -> {
            if (generation != probeGeneration || isFinishing() || isDestroyed()) { return; }
            probeRunning = false;
            engines.clear();
            engines.addAll(foundEngines);
            voices.clear();
            voices.addAll(foundVoices);
            if (success) {
                adoptAvailableVoice();
                status.setText(R.string.a11yTtsReady);
            } else {
                status.setText(R.string.a11yTtsUnavailable);
            }
            updateChoices();
        });
    }

    private void adoptAvailableVoice() {
        boolean exists = false;
        if (voiceName != null) {
            for (AndroidSpeech.VoiceOption voice : voices) {
                if (voice.voiceName.equals(voiceName)
                        && voice.locale.toLanguageTag().equals(languageTag)) {
                    exists = true;
                    break;
                }
            }
        }
        if (exists) { return; }
        voiceName = null;
        // Keep Vietnamese as the default if any Vietnamese voice exists.
        boolean foundLocale = false;
        for (AndroidSpeech.VoiceOption voice : voices) {
            if (voice.locale.toLanguageTag().equals(languageTag)) {
                foundLocale = true;
                break;
            }
        }
        if (!foundLocale && ReaderPreferences.DEFAULT_SPEECH_LANGUAGE.equals(languageTag)) {
            for (AndroidSpeech.VoiceOption voice : voices) {
                if ("vi".equals(voice.locale.getLanguage())) {
                    languageTag = voice.locale.toLanguageTag();
                    break;
                }
            }
        }
        AndroidSpeech.VoiceOption fallback = null;
        for (AndroidSpeech.VoiceOption voice : voices) {
            if (!voice.locale.toLanguageTag().equals(languageTag)) { continue; }
            if (!voice.networkRequired) {
                fallback = voice;
                break;
            }
            if (fallback == null) { fallback = voice; }
        }
        if (fallback != null) { voiceName = fallback.voiceName; }
    }

    private void cycleEngine() {
        if (probeRunning) { return; }
        ArrayList<String> choices = new ArrayList<>();
        choices.add(null);
        for (AndroidSpeech.EngineOption engine : engines) { choices.add(engine.name); }
        int index = choices.indexOf(engineName);
        engineName = choices.get(Math.floorMod(index + 1, choices.size()));
        voiceName = null;
        engineLabel = null;
        invalidatePreview();
        startProbe();
    }

    private void cycleLanguage() {
        if (probeRunning) { return; }
        ArrayList<String> tags = new ArrayList<>();
        for (AndroidSpeech.VoiceOption voice : voices) {
            String tag = voice.locale.toLanguageTag();
            if (!tags.contains(tag)) { tags.add(tag); }
        }
        if (tags.isEmpty()) { return; }
        int index = tags.indexOf(languageTag);
        languageTag = tags.get(Math.floorMod(index + 1, tags.size()));
        voiceName = null;
        adoptAvailableVoice();
        invalidatePreview();
        updateChoices();
        speakCurrentChoice();
    }

    private void cycleVoice() {
        if (probeRunning) { return; }
        ArrayList<AndroidSpeech.VoiceOption> choices = new ArrayList<>();
        for (AndroidSpeech.VoiceOption voice : voices) {
            if (voice.locale.toLanguageTag().equals(languageTag)) { choices.add(voice); }
        }
        if (choices.isEmpty()) { return; }
        int index = -1;
        for (int i = 0; i < choices.size(); i++) {
            if (choices.get(i).voiceName.equals(voiceName)) { index = i; }
        }
        voiceName = choices.get(Math.floorMod(index + 1, choices.size())).voiceName;
        invalidatePreview();
        updateChoices();
        speakCurrentChoice();
    }

    private void updateChoices() {
        engineButton.setEnabled(!probeRunning);
        languageButton.setEnabled(!probeRunning && !voices.isEmpty());
        voiceButton.setEnabled(!probeRunning && !voices.isEmpty());
        sampleButton.setEnabled(!probeRunning);
        String chosen = getString(R.string.a11yTtsSystemDefault);
        for (AndroidSpeech.EngineOption engine : engines) {
            if (engine.name.equals(engineName)) {
                engineLabel = engine.label;
                chosen = engine.label;
                break;
            }
        }
        if (engineName != null && engineLabel != null) { chosen = engineLabel; }
        engineButton.setText(getString(R.string.a11yTtsEngineChoice, chosen));
        Locale locale = Locale.forLanguageTag(languageTag);
        languageButton.setText(getString(R.string.a11yTtsLanguageChoice,
                locale.getDisplayName(Locale.getDefault())));
        voiceButton.setText(getString(R.string.a11yTtsVoiceChoice,
                voiceName == null ? getString(R.string.a11yTtsSystemDefault) : voiceName));
    }

    private void speakCurrentChoice() {
        if (foreground && surface != null && !usesTalkBack()) { surface.speakFocused(); }
    }

    private void testVoice() {
        if (probeRunning || speech == null) { return; }
        invalidatePreview();
        previewRequested = true;
        status.setText(R.string.a11ySpeechInitializing);
        speech.initialize(engineName, Locale.forLanguageTag(languageTag),
                selectedRate(), !online.isChecked(), voiceName,
                selectedPitch(), selectedVolume());
    }

    private void saveSettings() {
        if (probeRunning) {
            announce(getString(R.string.a11yTtsLoading));
            return;
        }
        if (enabled.isChecked() && selectedVolume() <= 0.0f) {
            announce(getString(R.string.a11yTtsVolumeTooLow));
            return;
        }
        if (enabled.isChecked() && !previewPassed) {
            announce(getString(R.string.a11yTtsTestFirst));
            return;
        }
        new ReaderPreferences(this).saveTtsSettings(enabled.isChecked(), engineName,
                voiceName, languageTag, online.isChecked(), selectedRate(),
                selectedPitch(), selectedVolume());
        setResult(RESULT_OK);
        finish();
    }

    private void restoreDefaults() {
        engineName = null;
        engineLabel = null;
        voiceName = null;
        languageTag = ReaderPreferences.DEFAULT_SPEECH_LANGUAGE;
        enabled.setChecked(true);
        online.setChecked(false);
        rate.setProgress(5);
        pitch.setProgress(5);
        volume.setProgress(20);
        updateSliders();
        invalidatePreview();
        startProbe();
    }

    private void openAndroidSettings() {
        invalidatePreview();
        try {
            startActivity(new Intent("com.android.settings.TTS_SETTINGS"));
        } catch (ActivityNotFoundException missing) {
            try {
                startActivity(new Intent(Settings.ACTION_SETTINGS));
            } catch (ActivityNotFoundException unavailable) {
                announce(getString(R.string.a11ySettingsUnavailable));
            }
        }
    }

    private boolean usesTalkBack() {
        AccessibilityManager manager = (AccessibilityManager)
                getSystemService(ACCESSIBILITY_SERVICE);
        return manager != null && manager.isEnabled();
    }

    private void speakLocal(String text) {
        if (foreground && !usesTalkBack() && speech != null
                && speech.getState() == AndroidSpeech.State.READY) {
            speech.speak("tts-focus-" + (++spokenSequence), text);
        }
    }

    private void announce(String message) {
        status.setText(message);
        if (usesTalkBack()) {
            status.announceForAccessibility(message);
        } else {
            speakLocal(message);
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        foreground = true;
        if (speech != null) {
            speech.initialize(engineName, Locale.forLanguageTag(languageTag),
                    selectedRate(), !online.isChecked(), voiceName,
                    selectedPitch(), selectedVolume());
        }
    }

    @Override
    protected void onPause() {
        foreground = false;
        previewRequested = false;
        previewPassed = false;
        if (speech != null) { speech.cancel(); }
        if (rescue != null) { rescue.stop(); }
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        ++probeGeneration;
        if (speech != null) { speech.close(); }
        if (rescue != null) { rescue.release(); }
        super.onDestroy();
    }
}

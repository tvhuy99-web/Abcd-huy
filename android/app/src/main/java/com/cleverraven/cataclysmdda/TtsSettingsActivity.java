package com.cleverraven.cataclysmdda;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.view.accessibility.AccessibilityManager;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.Spinner;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.Locale;
import java.util.Objects;

/**
 * Dedicated, self-voiced TTS settings. All edits are drafts until Save.
 * Voice preview is optional and never blocks saving a configuration.
 */
public final class TtsSettingsActivity extends Activity {
    private final ArrayList<AndroidSpeech.EngineOption> engines = new ArrayList<>();
    private final ArrayList<AndroidSpeech.VoiceOption> voices = new ArrayList<>();
    private AndroidSpeech speech;
    private BootstrapSelfVoiceSurface surface;
    private RescueAudioPlayer rescue;
    private CheckBox enabled;
    private final ArrayList<String> languageChoices = new ArrayList<>();
    private final ArrayList<AndroidSpeech.VoiceOption> voiceChoices = new ArrayList<>();
    private Spinner engineSpinner;
    private Spinner languageSpinner;
    private Spinner voiceSpinner;
    private boolean allowNetwork;
    private boolean updatingChoices;
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
        allowNetwork = saved.permitsNetworkVoice();

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
            public void goBack() {
                if (!surface.dismissChoiceList()) { TtsSettingsActivity.this.finish(); }
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

        engineSpinner = selector(column, R.string.a11yTtsEngine);
        languageSpinner = selector(column, R.string.a11yTtsLanguage);
        voiceSpinner = selector(column, R.string.a11yTtsVoice);
        bindSelectors();

        rateText = label(column, R.string.a11yTtsRate);
        rateText.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
        rate = slider(column, 95, Math.round((saved.getRate() - 0.5f) * 10.0f));
        pitchText = label(column, R.string.a11yTtsPitch);
        pitchText.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
        pitch = slider(column, 15, Math.round((saved.getPitch() - 0.5f) * 10.0f));
        volumeText = label(column, R.string.a11yTtsVolume);
        volumeText.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
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

        // Status row: visible on screen and announced in both modes (STT 5).
        status = new TextView(this);
        status.setTextSize(17.0f);
        status.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT));
        column.addView(status);
        sampleButton = button(column, R.string.a11yTtsSample, view -> testVoice());
        button(column, R.string.a11yTtsCancel, view -> finish());
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
                    announce(ttsFailureReason(newState));
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
                    announce(getString(R.string.a11yTtsTestComplete));
                } else if (event == AndroidSpeech.Event.ERROR
                        || event == AndroidSpeech.Event.REJECTED) {
                    previewPassed = false;
                    // The event carries no reason; use the probe/speech
                    // state instead (STT 4). The rescue clip is the only
                    // audio while the engine is down, so it plays here
                    // exactly as in the onState failure branch above.
                    announce(ttsFailureReason(speech.getState()));
                    if (foreground && !usesTalkBack()) {
                        if (rescue == null) { rescue = new RescueAudioPlayer(TtsSettingsActivity.this); }
                        rescue.play(RescueAudioPlayer.CLIP_TTS_FAILED);
                    }
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

    private Spinner selector(LinearLayout parent, int label) {
        Spinner result = new Spinner(this);
        result.setPrompt(getString(label));
        parent.addView(result);
        return result;
    }

    private void setChoices(Spinner spinner, ArrayList<String> choices, int selected) {
        ArrayAdapter<String> adapter = new ArrayAdapter<>(this,
                android.R.layout.simple_spinner_item, choices);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spinner.setAdapter(adapter);
        spinner.setSelection(Math.max(0, Math.min(selected, choices.size() - 1)));
    }

    private void bindSelectors() {
        engineSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                if (probeRunning || updatingChoices || position > engines.size()) { return; }
                String selected = position == 0 ? null : engines.get(position - 1).name;
                if (Objects.equals(engineName, selected)) { return; }
                engineName = selected;
                engineLabel = null;
                voiceName = null;
                invalidatePreview();
                startProbe();
            }
            @Override public void onNothingSelected(AdapterView<?> parent) { }
        });
        languageSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                if (probeRunning || updatingChoices || position >= languageChoices.size()) { return; }
                String selected = languageChoices.get(position);
                if (Objects.equals(languageTag, selected)) { return; }
                languageTag = selected;
                voiceName = null;
                adoptAvailableVoice();
                invalidatePreview();
                updateChoices();
            }
            @Override public void onNothingSelected(AdapterView<?> parent) { }
        });
        voiceSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                if (probeRunning || updatingChoices || position >= voiceChoices.size()) { return; }
                AndroidSpeech.VoiceOption selected = voiceChoices.get(position);
                String name = selected == null ? null : selected.voiceName;
                if (Objects.equals(voiceName, name)) { return; }
                voiceName = name;
                invalidatePreview();
            }
            @Override public void onNothingSelected(AdapterView<?> parent) { }
        });
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
                announce(getString(R.string.a11yTtsReady));
            } else {
                announce(getString(R.string.a11yTtsUnavailable));
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

    private void updateChoices() {
        updatingChoices = true;
        try {
            ArrayList<String> engineLabels = new ArrayList<>();
            engineLabels.add(getString(R.string.a11yTtsEngineChoice,
                    getString(R.string.a11yTtsSystemDefault)));
            int engineIndex = 0;
            for (AndroidSpeech.EngineOption engine : engines) {
                engineLabels.add(getString(R.string.a11yTtsEngineChoice, engine.label));
                if (engine.name.equals(engineName)) {
                    engineLabel = engine.label;
                    engineIndex = engineLabels.size() - 1;
                }
            }
            setChoices(engineSpinner, engineLabels, engineIndex);
            engineSpinner.setEnabled(!probeRunning);

            languageChoices.clear();
            languageChoices.add(languageTag);
            for (AndroidSpeech.VoiceOption voice : voices) {
                String tag = voice.locale.toLanguageTag();
                if (!languageChoices.contains(tag)) { languageChoices.add(tag); }
            }
            ArrayList<String> languages = new ArrayList<>();
            for (String tag : languageChoices) {
                Locale locale = Locale.forLanguageTag(tag);
                languages.add(getString(R.string.a11yTtsLanguageChoice,
                        locale.getDisplayName(Locale.getDefault())));
            }
            setChoices(languageSpinner, languages, 0);
            languageSpinner.setEnabled(!probeRunning && !voices.isEmpty());

            voiceChoices.clear();
            voiceChoices.add(null);
            ArrayList<String> voiceLabels = new ArrayList<>();
            voiceLabels.add(getString(R.string.a11yTtsVoiceChoice,
                    getString(R.string.a11yTtsSystemDefault)));
            int voiceIndex = 0;
            for (AndroidSpeech.VoiceOption voice : voices) {
                if (!voice.locale.toLanguageTag().equals(languageTag)) { continue; }
                voiceChoices.add(voice);
                voiceLabels.add(getString(R.string.a11yTtsVoiceChoice, voice.voiceName));
                if (voice.voiceName.equals(voiceName)) {
                    voiceIndex = voiceLabels.size() - 1;
                }
            }
            setChoices(voiceSpinner, voiceLabels, voiceIndex);
            voiceSpinner.setEnabled(!probeRunning && !voices.isEmpty());
            sampleButton.setEnabled(!probeRunning);
        } finally {
            updatingChoices = false;
        }
    }

    private void speakCurrentChoice() {
        if (foreground && surface != null && !usesTalkBack()) { surface.speakFocused(); }
    }

    private void testVoice() {
        if (probeRunning || speech == null) { return; }
        invalidatePreview();
        previewRequested = true;
        announce(getString(R.string.a11ySpeechInitializing));
        speech.initialize(engineName, Locale.forLanguageTag(languageTag),
                selectedRate(), !allowNetwork, voiceName,
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
        new ReaderPreferences(this).saveTtsSettings(enabled.isChecked(), engineName,
                voiceName, languageTag, allowNetwork, selectedRate(),
                selectedPitch(), selectedVolume());
        setResult(RESULT_OK);
        finish();
    }

    private boolean usesTalkBack() {
        AccessibilityManager manager = (AccessibilityManager)
                getSystemService(ACCESSIBILITY_SERVICE);
        return manager != null && manager.isTouchExplorationEnabled();
    }

    private void speakLocal(String text) {
        if (foreground && !usesTalkBack() && speech != null
                && speech.getState() == AndroidSpeech.State.READY) {
            speech.speak("tts-focus-" + (++spokenSequence), text);
        }
    }

    /**
     * Spoken reason for a failed voice state (STT 4). Unknown states fall
     * back to the generic unavailable message; never guess a reason.
     */
    private String ttsFailureReason(AndroidSpeech.State state) {
        if (state == AndroidSpeech.State.MISSING_LANGUAGE) {
            return getString(R.string.a11yTtsMissingLanguage);
        }
        if (state == AndroidSpeech.State.MISSING_VOICE) {
            return getString(R.string.a11yTtsMissingVoice);
        }
        if (state == AndroidSpeech.State.NETWORK_REQUIRED) {
            return getString(R.string.a11yTtsNetworkRequired);
        }
        return getString(R.string.a11yTtsUnavailable);
    }

    private void announce(String message) {
        status.setText(message);
        if (usesTalkBack()) {
            surface.announceForAccessibility(message);
        } else {
            speakLocal(message);
        }
    }

    @Override
    public void onBackPressed() {
        if (!surface.dismissChoiceList()) { super.onBackPressed(); }
    }

    @Override
    protected void onResume() {
        super.onResume();
        foreground = true;
        if (speech != null) {
            speech.initialize(engineName, Locale.forLanguageTag(languageTag),
                    selectedRate(), !allowNetwork, voiceName,
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

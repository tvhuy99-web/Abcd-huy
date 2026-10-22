package com.cleverraven.cataclysmdda;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.os.Bundle;
import android.provider.Settings;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.ScrollView;
import android.widget.TextView;

import java.util.Locale;

/** Native, TalkBack-accessible voice test. Does not enable gameplay narration. */
public final class ReaderSetupActivity extends Activity {
    private AndroidSpeech speech;
    private TextView status;
    private Button sample;
    private CheckBox online;
    private SeekBar rate;
    private TextView rateValue;
    private boolean foreground;
    private boolean sampleRequested;
    private boolean sampleAttempted;
    private long sampleSequence;
    private Button heard;
    private boolean testedNetworkConsent;
    private float testedRate = 1.0f;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setTitle(R.string.a11yReaderSetup);
        ScrollView scroll = new ScrollView(this);
        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        int padding = Math.round(20 * getResources().getDisplayMetrics().density);
        layout.setPadding(padding, padding, padding, padding);
        scroll.addView(layout);

        TextView explanation = new TextView(this);
        explanation.setText(R.string.a11yReaderSetupExplanation);
        layout.addView(explanation);
        status = new TextView(this);
        status.setId(R.id.a11y_speech_status);
        status.setSaveEnabled(false);
        status.setText(R.string.a11ySpeechNew);
        layout.addView(status);

        online = new CheckBox(this);
        online.setId(R.id.a11y_network_voice);
        online.setSaveEnabled(false);
        online.setText(R.string.a11yAllowNetworkVoice);
        // Online speech is opt-in for this test; never restore it silently.
        online.setChecked(false);
        layout.addView(online);

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
        updateRateDescription();

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
        setContentView(scroll);

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
                        heard.setEnabled(true);
                        showStatus(R.string.a11ySpeechPlaying, false);
                        break;
                    case DONE:
                        showStatus(R.string.a11ySpeechFinished, true);
                        break;
                    case ERROR:
                    case REJECTED:
                        sampleAttempted = false;
                        heard.setEnabled(false);
                        showStatus(R.string.a11ySpeechFailed, true);
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
        return result;
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
            status.announceForAccessibility(getString(text));
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

package com.cleverraven.cataclysmdda;

import android.app.Activity;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.view.View;
import android.view.accessibility.AccessibilityManager;
import android.widget.CheckBox;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.TextView;

import java.util.Locale;

/** Four direct controls for the native game's music and sound effects. */
public final class SoundSettingsActivity extends Activity {
    private static final String PREFS = "game_sound_controls";
    private BootstrapSelfVoiceSurface surface;
    private AndroidSpeech speech;
    private CheckBox musicEnabled;
    private CheckBox effectsEnabled;
    private SeekBar musicVolume;
    private SeekBar effectsVolume;
    private TextView musicValue;
    private TextView effectsValue;
    private SharedPreferences preferences;
    private boolean foreground;
    private boolean ready;
    private long speechSequence;

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        setTitle(R.string.a11ySoundSettingsTitle);
        preferences = getSharedPreferences(PREFS, MODE_PRIVATE);
        int nativeMusic = getIntent().getIntExtra("native_music_volume", 100);
        int nativeEffects = getIntent().getIntExtra("native_effects_volume", 100);
        boolean nativeEnabled = getIntent().getBooleanExtra("native_sound_enabled", true);
        int lastMusic = nativeMusic > 0 ? nativeMusic
                : preferences.getInt("music_level", 100);
        int lastEffects = nativeEffects > 0 ? nativeEffects
                : preferences.getInt("effects_level", 100);
        boolean musicOn = nativeEnabled && (nativeMusic > 0
                || preferences.getBoolean("music_on", false));
        boolean effectsOn = nativeEnabled && (nativeEffects > 0
                || preferences.getBoolean("effects_on", false));

        ReaderPreferences reader = new ReaderPreferences(this);
        surface = new BootstrapSelfVoiceSurface(this, new BootstrapSelfVoiceSurface.Listener() {
            @Override public boolean isActive() { return foreground && !usesTalkBack(); }
            @Override public void speak(String id, String text) { speakLocal(text); }
            @Override public void stopSpeech() {
                if (speech != null) { speech.cancel(); }
            }
            @Override public void goBack() { SoundSettingsActivity.this.finish(); }
            @Override public float swipeDistancePx() {
                return reader.getSwipeDistanceDp()
                        * getResources().getDisplayMetrics().density;
            }
            @Override public long swipeDurationLimitMs() {
                return reader.getSwipeDurationLimitMs();
            }
        });

        LinearLayout column = new LinearLayout(this);
        column.setOrientation(LinearLayout.VERTICAL);
        int padding = Math.round(16 * getResources().getDisplayMetrics().density);
        column.setPadding(padding, padding, padding, padding);

        musicEnabled = new CheckBox(this);
        musicEnabled.setText(R.string.a11yMusicEnabled);
        musicEnabled.setChecked(musicOn);
        column.addView(musicEnabled);
        musicValue = valueLabel(column);
        musicVolume = new SeekBar(this);
        musicVolume.setMax(128);
        musicVolume.setProgress(lastMusic);
        column.addView(musicVolume);

        effectsEnabled = new CheckBox(this);
        effectsEnabled.setText(R.string.a11yEffectsEnabled);
        effectsEnabled.setChecked(effectsOn);
        column.addView(effectsEnabled);
        effectsValue = valueLabel(column);
        effectsVolume = new SeekBar(this);
        effectsVolume.setMax(128);
        effectsVolume.setProgress(lastEffects);
        column.addView(effectsVolume);

        musicEnabled.setOnCheckedChangeListener((button, checked) -> persistAndApply());
        effectsEnabled.setOnCheckedChangeListener((button, checked) -> persistAndApply());
        SeekBar.OnSeekBarChangeListener changes = new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar bar, int value, boolean fromUser) {
                updateVolumeLabels();
                // The internal reader adjusts sliders with gestures, so an
                // accessibility change may report fromUser=false.
                persistAndApply();
            }
            @Override public void onStartTrackingTouch(SeekBar bar) { }
            @Override public void onStopTrackingTouch(SeekBar bar) { }
        };
        musicVolume.setOnSeekBarChangeListener(changes);
        effectsVolume.setOnSeekBarChangeListener(changes);
        updateVolumeLabels();
        setContentView(surface);
        surface.setContent(column);
        ready = true;

        speech = new AndroidSpeech(this, new AndroidSpeech.Listener() {
            @Override public void onState(AndroidSpeech.State state) {
                if (state == AndroidSpeech.State.READY && foreground && !usesTalkBack()) {
                    surface.speakFocused();
                }
            }
            @Override public void onUtterance(String id, AndroidSpeech.Event event) { }
        });
    }

    private TextView valueLabel(LinearLayout column) {
        TextView label = new TextView(this);
        label.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
        column.addView(label);
        return label;
    }

    private void updateVolumeLabels() {
        String music = getString(R.string.a11yMusicVolumeValue, musicVolume.getProgress());
        String effects = getString(R.string.a11yEffectsVolumeValue, effectsVolume.getProgress());
        musicValue.setText(music);
        effectsValue.setText(effects);
        musicVolume.setContentDescription(music);
        effectsVolume.setContentDescription(effects);
    }

    private void persistAndApply() {
        if (!ready) { return; }
        int music = musicVolume.getProgress();
        int effects = effectsVolume.getProgress();
        boolean musicOn = musicEnabled.isChecked();
        boolean effectsOn = effectsEnabled.isChecked();
        preferences.edit().putBoolean("music_on", musicOn)
                .putBoolean("effects_on", effectsOn)
                .putInt("music_level", music)
                .putInt("effects_level", effects).apply();
        // JNI only queues the values. The game thread applies its own option
        // changes and saves them; UI gestures never modify C++ options.
        CataclysmDDA.nativeQueueSoundSettings(musicOn ? music : 0,
                effectsOn ? effects : 0, musicOn || effectsOn);
    }

    private boolean usesTalkBack() {
        AccessibilityManager manager = (AccessibilityManager)
                getSystemService(ACCESSIBILITY_SERVICE);
        return manager != null && manager.isTouchExplorationEnabled();
    }

    private void speakLocal(String text) {
        if (foreground && !usesTalkBack() && speech != null
                && speech.getState() == AndroidSpeech.State.READY) {
            speech.speak("sound-focus-" + (++speechSequence), text);
        }
    }

    @Override protected void onResume() {
        super.onResume();
        foreground = true;
        ReaderPreferences reader = new ReaderPreferences(this);
        if (speech != null) {
            speech.initialize(reader.getSpeechEngineName(),
                    Locale.forLanguageTag(reader.getSpeechLanguageTag()),
                    reader.getRate(), !reader.permitsNetworkVoice(),
                    reader.getSpeechVoiceName(), reader.getPitch(), reader.getVolume());
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

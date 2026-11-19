package com.cleverraven.cataclysmdda;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.View;
import android.view.accessibility.AccessibilityManager;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.TextView;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

/**
 * Accessible game-audio settings with live soundpack selection and SAF ZIP
 * import/export. Native SDL/option changes remain game-thread-only.
 */
public final class SoundSettingsActivity extends Activity {
    private static final String PREFS = "game_sound_controls";
    private static final int REQUEST_IMPORT = 7481;
    private static final int REQUEST_EXPORT = 7482;

    private final Handler handler = new Handler(Looper.getMainLooper());
    private BootstrapSelfVoiceSurface surface;
    private AndroidSpeech speech;
    private CheckBox musicEnabled;
    private CheckBox effectsEnabled;
    private SeekBar musicVolume;
    private SeekBar effectsVolume;
    private TextView musicValue;
    private TextView effectsValue;
    private TextView status;
    private Button selectedButton;
    private LinearLayout homeControls;
    private LinearLayout visibleControls;
    private File userSound;
    private File bundledSound;
    private SharedPreferences preferences;
    private String selectedSoundpack;
    private String pendingSelection;
    private String pendingStatus;
    private boolean foreground;
    private boolean ready;
    private boolean busy;
    private long speechSequence;

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        setTitle(R.string.a11ySoundSettingsTitle);
        preferences = getSharedPreferences(PREFS, MODE_PRIVATE);
        try {
            userSound = new File(CataclysmDDA.nativeUserSoundDir());
            bundledSound = new File(CataclysmDDA.nativeDataSoundDir());
        } catch (UnsatisfiedLinkError missingNative) {
            finish();
            return;
        }
        selectedSoundpack = getIntent().getStringExtra("native_selected_soundpack");
        if (selectedSoundpack == null) { selectedSoundpack = ""; }
        if (state != null) {
            selectedSoundpack = state.getString("selected_soundpack", selectedSoundpack);
            pendingSelection = state.getString("pending_soundpack");
        }

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
            @Override public void openContextActions() { showContextActions(); }
            @Override public void goBack() { goBackFromPanel(); }
            @Override public float swipeDistancePx() {
                return reader.getSwipeDistanceDp()
                        * getResources().getDisplayMetrics().density;
            }
            @Override public long swipeDurationLimitMs() {
                return reader.getSwipeDurationLimitMs();
            }
        });

        homeControls = newPanel();
        selectedButton = addButton(homeControls, R.string.a11ySoundpackChoose,
                view -> showSoundpacks());
        selectedButton.setText(getString(R.string.a11ySoundpackCurrent,
                selectedDisplay()));
        addButton(homeControls, R.string.a11ySoundpackImport, view -> chooseZip(false));

        // Deliberate order: toggle music, music slider, toggle effects, effects slider.
        musicEnabled = new CheckBox(this);
        musicEnabled.setText(R.string.a11yMusicEnabled);
        musicEnabled.setChecked(musicOn);
        homeControls.addView(musicEnabled);
        musicValue = valueLabel(homeControls);
        musicVolume = new SeekBar(this);
        musicVolume.setMax(128);
        musicVolume.setProgress(lastMusic);
        homeControls.addView(musicVolume);

        effectsEnabled = new CheckBox(this);
        effectsEnabled.setText(R.string.a11yEffectsEnabled);
        effectsEnabled.setChecked(effectsOn);
        homeControls.addView(effectsEnabled);
        effectsValue = valueLabel(homeControls);
        effectsVolume = new SeekBar(this);
        effectsVolume.setMax(128);
        effectsVolume.setProgress(lastEffects);
        homeControls.addView(effectsVolume);

        addButton(homeControls, R.string.a11ySoundpackExport, view -> chooseZip(true));

        status = new TextView(this);
        status.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
        status.setText(R.string.a11ySoundpackReady);
        homeControls.addView(status);

        musicEnabled.setOnCheckedChangeListener((button, checked) -> persistAndApply());
        effectsEnabled.setOnCheckedChangeListener((button, checked) -> persistAndApply());
        SeekBar.OnSeekBarChangeListener changes = new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar bar, int value, boolean fromUser) {
                updateVolumeLabels();
                persistAndApply();
            }
            @Override public void onStartTrackingTouch(SeekBar bar) { }
            @Override public void onStopTrackingTouch(SeekBar bar) { }
        };
        musicVolume.setOnSeekBarChangeListener(changes);
        effectsVolume.setOnSeekBarChangeListener(changes);
        updateVolumeLabels();
        setContentView(surface);
        showPanel(homeControls);
        ready = true;

        speech = new AndroidSpeech(this, new AndroidSpeech.Listener() {
            @Override public void onState(AndroidSpeech.State state) {
                if (state == AndroidSpeech.State.READY && foreground && !usesTalkBack()) {
                    if (pendingStatus != null) {
                        String message = pendingStatus;
                        pendingStatus = null;
                        speakLocal(message);
                    } else {
                        surface.speakFocused();
                    }
                }
            }
            @Override public void onUtterance(String id, AndroidSpeech.Event event) { }
        });
    }

    private LinearLayout newPanel() {
        LinearLayout panel = new LinearLayout(this);
        panel.setOrientation(LinearLayout.VERTICAL);
        int padding = Math.round(16 * getResources().getDisplayMetrics().density);
        panel.setPadding(padding, padding, padding, padding);
        return panel;
    }

    private Button addButton(LinearLayout panel, int label, View.OnClickListener click) {
        Button button = new Button(this);
        button.setText(label);
        button.setOnClickListener(click);
        panel.addView(button);
        return button;
    }

    private TextView valueLabel(LinearLayout column) {
        TextView label = new TextView(this);
        label.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
        column.addView(label);
        return label;
    }

    private void showPanel(LinearLayout panel) {
        visibleControls = panel;
        surface.setContent(panel);
    }

    private void showContextActions() {
        LinearLayout previous = visibleControls;
        LinearLayout panel = newPanel();
        addButton(panel, R.string.a11yReaderReturn, view -> showPanel(previous));
        // Last context command is always the universal gesture guide.
        addButton(panel, R.string.a11yGestureGuideTitle, view -> showGestureGuide());
        showPanel(panel);
    }

    private void showGestureGuide() {
        LinearLayout previous = visibleControls;
        LinearLayout panel = newPanel();
        TextView text = new TextView(this);
        text.setText(R.string.a11ySoundpackGestures);
        panel.addView(text);
        addButton(panel, R.string.a11yReaderReturn, view -> showPanel(previous));
        showPanel(panel);
    }

    private void goBackFromPanel() {
        if (surface.dismissChoiceList()) { return; }
        if (visibleControls != homeControls) {
            showPanel(homeControls);
            return;
        }
        if (!busy) { finish(); }
    }

    @Override public void onBackPressed() {
        goBackFromPanel();
    }

    private List<SoundpackArchive.Pack> available() {
        try {
            return SoundpackArchive.list(userSound, bundledSound);
        } catch (IOException failed) {
            showStatus(getString(R.string.a11yManagedSoundpackFailed, failed.getMessage()));
            return new ArrayList<>();
        }
    }

    private String selectedDisplay() {
        if (preferences.getBoolean("soundpack_none", false)
                && !musicEnabledIsOnSafe() && !effectsEnabledIsOnSafe()) {
            return getString(R.string.a11ySoundpackNone);
        }
        if (selectedSoundpack == null || selectedSoundpack.isEmpty()) {
            return getString(R.string.a11ySoundpackUnknown);
        }
        try {
            for (SoundpackArchive.Pack pack : SoundpackArchive.list(userSound, bundledSound)) {
                if (pack.id.equals(selectedSoundpack)) { return pack.display; }
            }
        } catch (IOException ignored) {
            // Keep the native selected ID visible if the disk is inaccessible.
        }
        return selectedSoundpack;
    }

    private boolean musicEnabledIsOnSafe() {
        return musicEnabled != null && musicEnabled.isChecked();
    }

    private boolean effectsEnabledIsOnSafe() {
        return effectsEnabled != null && effectsEnabled.isChecked();
    }

    private void updateSelectedButton() {
        selectedButton.setText(getString(R.string.a11ySoundpackCurrent, selectedDisplay()));
    }

    private void showSoundpacks() {
        if (busy || pendingSelection != null) { return; }
        LinearLayout chooser = newPanel();
        TextView title = new TextView(this);
        title.setText(R.string.a11ySoundpackChoose);
        title.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
        chooser.addView(title);

        addButton(chooser, R.string.a11ySoundpackNone, view -> {
            preferences.edit().putBoolean("soundpack_none", true).apply();
            musicEnabled.setChecked(false);
            effectsEnabled.setChecked(false);
            persistAndApply();
            updateSelectedButton();
            showPanel(homeControls);
            showStatus(getString(R.string.a11ySoundpackDisabled));
        });
        for (SoundpackArchive.Pack pack : available()) {
            Button entry = new Button(this);
            entry.setText(pack.display
                    + (pack.id.equals(selectedSoundpack)
                    ? " (" + getString(R.string.a11ySoundpackActive) + ")" : ""));
            entry.setOnClickListener(view -> {
                pendingSelection = pack.id;
                showPanel(homeControls);
                surface.post(() -> showStatus(
                        getString(R.string.a11ySoundpackApplying, pack.display)));
                CataclysmDDA.nativeQueueSoundpackSelection(pack.id);
                pollSelection();
            });
            chooser.addView(entry);
        }
        addButton(chooser, R.string.a11yReaderReturn, view -> showPanel(homeControls));
        showPanel(chooser);
    }

    private void pollSelection() {
        handler.postDelayed(() -> {
            if (pendingSelection == null || isFinishing() || isDestroyed()) { return; }
            String result = CataclysmDDA.nativeTakeSoundpackSelectionResult();
            if (result != null && !result.isEmpty()) {
                String requested = pendingSelection;
                pendingSelection = null;
                if (result.equals("OK:" + requested)) {
                    selectedSoundpack = requested;
                    preferences.edit().putBoolean("soundpack_none", false).apply();
                    updateSelectedButton();
                    showStatus(getString(R.string.a11ySoundpackApplied, selectedDisplay()));
                } else {
                    showStatus(getString(R.string.a11yManagedSoundpackFailed,
                            getString(R.string.a11ySoundpackApplyFailed)));
                }
            } else {
                pollSelection();
            }
        }, 180);
    }

    private void chooseZip(boolean exporting) {
        if (busy || pendingSelection != null) { return; }
        if (exporting) {
            if (preferences.getBoolean("soundpack_none", false)
                    || selectedSoundpack == null || selectedSoundpack.isEmpty()) {
                showStatus(getString(R.string.a11ySoundpackNothingToExport));
                return;
            }
            boolean installed = false;
            for (SoundpackArchive.Pack pack : available()) {
                if (pack.id.equals(selectedSoundpack)) {
                    installed = true;
                    break;
                }
            }
            if (!installed) {
                showStatus(getString(R.string.a11ySoundpackNothingToExport));
                return;
            }
        }
        Intent picker = new Intent(exporting ? Intent.ACTION_CREATE_DOCUMENT
                : Intent.ACTION_OPEN_DOCUMENT);
        picker.addCategory(Intent.CATEGORY_OPENABLE);
        picker.setType("application/zip");
        if (exporting) {
            String filename = selectedSoundpack.replaceAll("[^a-zA-Z0-9_. -]", "_");
            if (filename.isEmpty()) { filename = "soundpack"; }
            picker.putExtra(Intent.EXTRA_TITLE, filename + ".zip");
        }
        try {
            startActivityForResult(picker, exporting ? REQUEST_EXPORT : REQUEST_IMPORT);
        } catch (ActivityNotFoundException unavailable) {
            showStatus(getString(R.string.a11ySoundpackPickerUnavailable));
        }
    }

    @Override protected void onActivityResult(int request, int result, Intent data) {
        super.onActivityResult(request, result, data);
        if (request != REQUEST_IMPORT && request != REQUEST_EXPORT) { return; }
        Uri chosen = result == RESULT_OK && data != null ? data.getData() : null;
        if (chosen == null) {
            showStatus(getString(R.string.a11ySoundpackCancelled));
            return;
        }
        busy = true;
        final String id = selectedSoundpack;
        showStatus(getString(R.string.a11ySoundpackWorking));
        new Thread(() -> {
            String announcement;
            try {
                if (request == REQUEST_IMPORT) {
                    try (InputStream input = getContentResolver().openInputStream(chosen)) {
                        if (input == null) { throw new IOException("Không đọc được ZIP"); }
                        SoundpackArchive.Pack imported =
                                SoundpackArchive.importZip(input, userSound, bundledSound);
                        announcement = getString(R.string.a11ySoundpackImported, imported.display);
                    }
                } else {
                    try (OutputStream output = getContentResolver().openOutputStream(chosen, "w")) {
                        if (output == null) { throw new IOException("Không tạo được ZIP"); }
                        SoundpackArchive.exportZip(output, userSound, bundledSound, id);
                        announcement = getString(R.string.a11ySoundpackExported, id);
                    }
                }
            } catch (IOException | RuntimeException failed) {
                announcement = getString(R.string.a11yManagedSoundpackFailed,
                        failed.getMessage() == null ? "" : failed.getMessage());
            }
            final String completed = announcement;
            runOnUiThread(() -> {
                if (isFinishing() || isDestroyed()) { return; }
                busy = false;
                updateSelectedButton();
                showPanel(homeControls);
                surface.post(() -> showStatus(completed));
            });
        }, "soundpack-zip-io").start();
    }

    private void showStatus(String message) {
        if (status == null) { return; }
        status.setText(message);
        if (!foreground) {
            pendingStatus = message;
        } else if (usesTalkBack()) {
            pendingStatus = null;
            status.announceForAccessibility(message);
        } else if (speech == null || speech.getState() != AndroidSpeech.State.READY) {
            pendingStatus = message;
        } else {
            pendingStatus = null;
            speakLocal(message);
        }
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
                .putInt("effects_level", effects)
                .putBoolean("soundpack_none", !musicOn && !effectsOn
                        && preferences.getBoolean("soundpack_none", false)).apply();
        CataclysmDDA.nativeQueueSoundSettings(musicOn ? music : 0,
                effectsOn ? effects : 0, musicOn || effectsOn);
        updateSelectedButton();
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

    @Override protected void onSaveInstanceState(Bundle state) {
        state.putString("selected_soundpack", selectedSoundpack);
        state.putString("pending_soundpack", pendingSelection);
        super.onSaveInstanceState(state);
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
        if (pendingSelection != null) {
            pollSelection();
        }
        if (pendingStatus != null && usesTalkBack() && status != null) {
            String message = pendingStatus;
            pendingStatus = null;
            status.announceForAccessibility(message);
        }
    }

    @Override protected void onPause() {
        foreground = false;
        if (speech != null) { speech.cancel(); }
        super.onPause();
    }

    @Override protected void onDestroy() {
        if (speech != null) { speech.close(); }
        handler.removeCallbacksAndMessages(null);
        super.onDestroy();
    }
}

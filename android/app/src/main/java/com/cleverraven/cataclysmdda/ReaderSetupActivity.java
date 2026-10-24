package com.cleverraven.cataclysmdda;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
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

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.Locale;

/** Voice setup with TalkBack controls and a self-voicing path before enabling the reader. */
public final class ReaderSetupActivity extends Activity {
    public static final String EXTRA_DISABLE_SELF_VOICE =
            "com.cleverraven.cataclysmdda.extra.DISABLE_SETUP_SELF_VOICE";

    private AndroidSpeech speech;
    private RescueAudioPlayer rescuePlayer;
    private boolean rescueClipPlayed;
    // Backup and restore (R20.2). All of this state is instance memory
    // only: the pending picker action, the staged-but-unconfirmed import
    // and the busy flag are never written to savedInstanceState or to
    // preferences, so an Activity recreate or a process death abandons
    // them instead of completing a restore the user confirmed in a
    // previous instance (R20.3).
    static final String BACKUP_JOURNAL_FILE = "reader_backup_journal.txt";
    private static final int REQUEST_BACKUP_EXPORT = 4201;
    private static final int REQUEST_BACKUP_IMPORT = 4202;
    private static final int BACKUP_ACTION_NONE = 0;
    private static final int BACKUP_ACTION_EXPORT = 1;
    private static final int BACKUP_ACTION_IMPORT = 2;
    private ReaderBackupArchive backupArchive;
    private Button backupExport;
    private Button backupImport;
    private Button backupOpenPicker;
    private Button backupConfirmRestore;
    private Button backupCancelRestore;
    private int pendingBackupAction = BACKUP_ACTION_NONE;
    private boolean stagedBackupReady;
    private boolean backupBusy;
    private int backupOperationSequence;
    private TextView status;
    private Button sample;
    private CheckBox online;
    // Voice selection drafts (R06.1). These are only written to
    // ReaderPreferences after a sample spoken with exactly these values
    // finishes and the user confirms; cycling a choice invalidates the
    // previous sample, so an untested selection can never be saved.
    private Button languageChoice;
    private Button engineChoice;
    private Button voiceChoice;
    private TextView voiceDataStatus;
    private String draftEngineName;
    private String draftEngineLabel;
    private String draftVoiceName;
    private String draftLanguageTag = ReaderPreferences.DEFAULT_SPEECH_LANGUAGE;
    private boolean draftVoiceNetwork;
    private final ArrayList<AndroidSpeech.EngineOption> probedEngines = new ArrayList<>();
    private final ArrayList<AndroidSpeech.VoiceOption> probedVoices = new ArrayList<>();
    private boolean voiceProbeRunning;
    private boolean voiceProbeFailed;
    private String testedEngineName;
    private String testedVoiceName;
    private String testedLanguageTag;
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

        TextView voiceHeading = new TextView(this);
        voiceHeading.setText(R.string.a11yVoiceSelectionHeading);
        layout.addView(voiceHeading);
        selfVoiceControls.add(voiceHeading);
        ReaderPreferences savedSpeechPreferences = new ReaderPreferences(this);
        draftEngineName = savedSpeechPreferences.getSpeechEngineName();
        draftVoiceName = savedSpeechPreferences.getSpeechVoiceName();
        draftLanguageTag = savedSpeechPreferences.getSpeechLanguageTag();
        languageChoice = button(layout, R.string.a11yVoiceProbing, view -> cycleVoiceLanguage());
        languageChoice.setId(R.id.a11y_voice_language);
        engineChoice = button(layout, R.string.a11yVoiceProbing, view -> cycleVoiceEngine());
        engineChoice.setId(R.id.a11y_voice_engine);
        voiceChoice = button(layout, R.string.a11yVoiceProbing, view -> cycleVoiceVoice());
        voiceChoice.setId(R.id.a11y_voice_choice);
        voiceDataStatus = new TextView(this);
        voiceDataStatus.setText(R.string.a11yVoiceProbing);
        layout.addView(voiceDataStatus);
        selfVoiceControls.add(voiceDataStatus);

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
            testedEngineName = draftEngineName;
            testedVoiceName = draftVoiceName;
            testedLanguageTag = draftLanguageTag;
            sample.setEnabled(false);
            speech.initialize(testedEngineName, Locale.forLanguageTag(testedLanguageTag),
                    testedRate, !online.isChecked(), testedVoiceName);
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
        button(layout, R.string.a11yRescueHelp, view -> rescuePlayer().play(
                RescueAudioPlayer.CLIP_SETUP_FAILED)).setId(R.id.a11y_rescue_help);
        heard = button(layout, R.string.a11yConfirmHeardEnable, view -> {
            ReaderPreferences preferences = new ReaderPreferences(this);
            preferences.confirmHeardAndEnable(testedNetworkConsent, testedRate);
            // Save exactly the voice selection the finished sample used;
            // draft values that were never sample-tested cannot reach
            // storage because changing them disables this button.
            preferences.saveSpeechSelection(testedEngineName, testedVoiceName, testedLanguageTag);
            showStatus(R.string.a11yReaderEnabledNotice, true);
        });
        heard.setEnabled(false);
        online.setOnCheckedChangeListener((button, checked) -> invalidateSampleForChangedSettings());
        button(layout, R.string.a11yDisableInternalReader, view -> {
            new ReaderPreferences(this).disable();
            showStatus(R.string.a11yReaderDisabledNotice, true);
        });

        TextView cuesHeading = new TextView(this);
        cuesHeading.setText(R.string.a11yCuesHeading);
        layout.addView(cuesHeading);
        selfVoiceControls.add(cuesHeading);

        // Cue toggles (R21.2) save immediately, like the left-handed
        // toggle: they do not change what is spoken, only whether the
        // optional sound/vibration duplicate of an event also plays.
        // Enabling one plays a sample through a short-lived player so
        // the user hears/feels exactly what they turned on.
        CheckBox soundCues = new CheckBox(this);
        soundCues.setId(R.id.a11y_sound_cues);
        soundCues.setText(R.string.a11ySoundCues);
        soundCues.setChecked(new ReaderPreferences(this).areSoundCuesEnabled());
        soundCues.setOnCheckedChangeListener((button, checked) -> {
            new ReaderPreferences(this).setSoundCuesEnabled(checked);
            if (checked) { sampleCuePlayer().play(ReaderCuePlayer.ACTIVITY_DONE); }
        });
        layout.addView(soundCues);
        selfVoiceControls.add(soundCues);

        CheckBox hapticCues = new CheckBox(this);
        hapticCues.setId(R.id.a11y_haptic_cues);
        hapticCues.setText(R.string.a11yHapticCues);
        hapticCues.setChecked(new ReaderPreferences(this).areHapticCuesEnabled());
        hapticCues.setOnCheckedChangeListener((button, checked) -> {
            new ReaderPreferences(this).setHapticCuesEnabled(checked);
            if (checked) { sampleCuePlayer().play(ReaderCuePlayer.ACTIVITY_DONE); }
        });
        layout.addView(hapticCues);
        selfVoiceControls.add(hapticCues);

        TextView diagnosticsHeading = new TextView(this);
        diagnosticsHeading.setText(R.string.a11yDiagnosticsHeading);
        layout.addView(diagnosticsHeading);
        selfVoiceControls.add(diagnosticsHeading);
        button(layout, R.string.a11yDiagnosticsRead, view -> readDiagnosticsReport())
                .setId(R.id.a11y_diagnostics_read);

        TextView backupHeading = new TextView(this);
        backupHeading.setText(R.string.a11yBackupHeading);
        layout.addView(backupHeading);
        selfVoiceControls.add(backupHeading);
        backupExport = button(layout, R.string.a11yBackupExport,
                view -> chooseBackupPicker(true));
        backupExport.setId(R.id.a11y_backup_export);
        backupImport = button(layout, R.string.a11yBackupImport,
                view -> chooseBackupPicker(false));
        backupImport.setId(R.id.a11y_backup_import);
        backupOpenPicker = button(layout, R.string.a11yBackupOpenPickerExport,
                view -> openBackupPicker());
        backupOpenPicker.setId(R.id.a11y_backup_open_picker);
        backupOpenPicker.setEnabled(false);
        backupConfirmRestore = button(layout, R.string.a11yBackupConfirmRestore,
                view -> confirmBackupRestore());
        backupConfirmRestore.setId(R.id.a11y_backup_confirm_restore);
        backupConfirmRestore.setEnabled(false);
        backupCancelRestore = button(layout, R.string.a11yBackupCancelRestore,
                view -> cancelBackupRestore());
        backupCancelRestore.setId(R.id.a11y_backup_cancel_restore);
        backupCancelRestore.setEnabled(false);
        File backupRoot = getExternalFilesDir(null);
        if (backupRoot != null) {
            backupArchive = new ReaderBackupArchive(backupRoot,
                    new File(getFilesDir(), BACKUP_JOURNAL_FILE));
            // A fresh screen instance owns no staged import: first put
            // back whatever a swap interrupted by process death parked,
            // then discard any staged backup the dead instance left
            // unconfirmed, so it can never be restored from here without
            // a new file choice and a new explicit confirmation.
            try {
                backupArchive.recoverInterruptedSwap();
                if (ReaderGameSession.canRestore()) { backupArchive.cancelStagedImport(); }
            } catch (ReaderBackupArchive.BackupException failure) {
                backupArchive = null;
                status.setText(R.string.a11yBackupRecoveryFailed);
            }
        }
        updateBackupButtons();
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
                        rescueClipPlayed = false;
                        showStatus(R.string.a11ySpeechInitializing, true);
                        break;
                    case READY:
                        rescueClipPlayed = false;
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
                        playRescueClipOnce();
                        break;
                    case MISSING_VOICE:
                        sampleAttempted = false;
                        sampleRequested = false;
                        heard.setEnabled(false);
                        showStatus(R.string.a11yVoiceMissing, true);
                        playRescueClipOnce();
                        break;
                    case NETWORK_REQUIRED:
                        sampleAttempted = false;
                        sampleRequested = false;
                        heard.setEnabled(false);
                        showStatus(R.string.a11ySpeechNetworkRequired, true);
                        playRescueClipOnce();
                        break;
                    case FAILED:
                        sampleAttempted = false;
                        sampleRequested = false;
                        heard.setEnabled(false);
                        showStatus(R.string.a11ySpeechFailed, true);
                        playRescueClipOnce();
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
        startVoiceProbe(draftEngineName);
    }

    private RescueAudioPlayer rescuePlayer() {
        if (rescuePlayer == null) {
            rescuePlayer = new RescueAudioPlayer(this);
        }
        return rescuePlayer;
    }

    private ReaderCuePlayer sampleCues;

    /** Short-lived cue player used only for the toggle samples (R21.2). */
    private ReaderCuePlayer sampleCuePlayer() {
        if (sampleCues == null) {
            sampleCues = new ReaderCuePlayer(this, new ReaderPreferences(this), null);
        }
        return sampleCues;
    }

    /**
     * Speak the diagnostics report (R21.4). The store holds codes and
     * counters only — no utterance text, no save names — so the whole
     * report is safe to read aloud and to share as a bug report.
     */
    private void readDiagnosticsReport() {
        ReaderDiagnostics diagnostics = ReaderDiagnostics.get();
        if (!diagnostics.hasSession()) {
            showStatus(R.string.a11yDiagnosticsNone, true);
            return;
        }
        StringBuilder report = new StringBuilder(getString(R.string.a11yDiagnosticsTitle));
        report.append(' ').append(getString(R.string.a11yDiagnosticsUptime,
                diagnostics.uptimeSeconds()));
        ReaderSpeechQueue.Stats stats = diagnostics.speechStats();
        if (stats != null) {
            report.append(' ').append(getString(R.string.a11yDiagnosticsSpeech,
                    stats.spoken, stats.interrupted, stats.droppedDuplicate,
                    stats.droppedExpired, stats.evicted));
            if (stats.lastStartLatencyMs >= 0) {
                report.append(' ').append(getString(R.string.a11yDiagnosticsLatency,
                        stats.lastStartLatencyMs, stats.maxStartLatencyMs));
            } else {
                report.append(' ').append(getString(R.string.a11yDiagnosticsLatencyNone));
            }
            report.append(' ').append(getString(R.string.a11yDiagnosticsQueue, stats.pending,
                    getString(stats.speaking ? R.string.a11yDiagnosticsYes
                            : R.string.a11yDiagnosticsNo)));
        }
        report.append(' ').append(getString(R.string.a11yDiagnosticsAnnouncements,
                diagnostics.counter("announce_info"), diagnostics.counter("announce_urgent")));
        report.append(' ').append(getString(R.string.a11yDiagnosticsReceipts,
                diagnostics.counter("receipt_stale"), diagnostics.counter("receipt_disabled"),
                diagnostics.counter("receipt_invalid_value"),
                diagnostics.counter("receipt_unsupported")));
        report.append(' ').append(getString(R.string.a11yDiagnosticsCues,
                diagnostics.counter("cue_blocked"), diagnostics.counter("cue_danger"),
                diagnostics.counter("cue_pickup"), diagnostics.counter("cue_activity_done"),
                diagnostics.counter("cue_rejected"), diagnostics.counter("cue_unavailable"),
                diagnostics.counter("cue_focus_denied")));
        java.util.List<String> events = diagnostics.recentEvents();
        if (events.isEmpty()) {
            report.append(' ').append(getString(R.string.a11yDiagnosticsNoEvents));
        } else {
            StringBuilder joined = new StringBuilder();
            for (String event : events) {
                if (joined.length() > 0) {
                    joined.append("; ");
                }
                joined.append(event);
            }
            report.append(' ').append(getString(R.string.a11yDiagnosticsEvents,
                    joined.toString()));
        }
        showStatusText(report.toString(), true);
    }

    /**
     * Play the packaged rescue clip once per failure episode (R06.3):
     * when speech itself is broken the user still hears what the labeled
     * buttons on this screen do, in order.
     */
    private void playRescueClipOnce() {
        if (usesSystemAccessibility() || rescueClipPlayed) {
            return;
        }
        rescueClipPlayed = true;
        rescuePlayer().play(RescueAudioPlayer.CLIP_SETUP_FAILED);
    }

    // ---- Backup and restore (R20.2) ----

    /**
     * Step one of export/import: speak the whole guidance while this
     * screen still owns speech, and only arm the Continue button. The
     * system file picker is a separate, deliberate second activation, so
     * the user has heard what the picker takeover means, what choosing a
     * file there does, and that returning here announces the result
     * before the picker ever opens.
     */
    private void chooseBackupPicker(boolean export) {
        if (!export && !ReaderGameSession.canRestore()) {
            showStatus(R.string.a11yBackupRequiresRestart, true);
            return;
        }
        if (backupBusy) {
            return;
        }
        if (backupArchive == null) {
            showStatus(R.string.a11yBackupStorageUnavailable, true);
            return;
        }
        pendingBackupAction = export ? BACKUP_ACTION_EXPORT : BACKUP_ACTION_IMPORT;
        backupOpenPicker.setText(export ? R.string.a11yBackupOpenPickerExport
                : R.string.a11yBackupOpenPickerImport);
        updateBackupButtons();
        showStatusText(getString(export ? R.string.a11yBackupExportGuidance
                : R.string.a11yBackupImportGuidance), true);
    }

    /** Step two: hand the screen to the system picker and nothing else. */
    private void openBackupPicker() {
        if (backupBusy || pendingBackupAction == BACKUP_ACTION_NONE) {
            return;
        }
        boolean export = pendingBackupAction == BACKUP_ACTION_EXPORT;
        Intent intent = new Intent(export ? Intent.ACTION_CREATE_DOCUMENT
                : Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("application/zip");
        if (export) {
            intent.putExtra(Intent.EXTRA_TITLE, "cdda-backup.zip");
        }
        try {
            startActivityForResult(intent,
                    export ? REQUEST_BACKUP_EXPORT : REQUEST_BACKUP_IMPORT);
        } catch (ActivityNotFoundException unavailable) {
            pendingBackupAction = BACKUP_ACTION_NONE;
            updateBackupButtons();
            showStatus(R.string.a11yBackupPickerUnavailable, true);
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode != REQUEST_BACKUP_EXPORT && requestCode != REQUEST_BACKUP_IMPORT) {
            return;
        }
        boolean export = requestCode == REQUEST_BACKUP_EXPORT;
        pendingBackupAction = BACKUP_ACTION_NONE;
        updateBackupButtons();
        Uri uri = resultCode == RESULT_OK && data != null ? data.getData() : null;
        if (uri == null) {
            // Returning without a choice is announced on this screen, the
            // clear way back the pre-picker guidance promised.
            showStatus(R.string.a11yBackupPickerCancelled, true);
            return;
        }
        if (backupArchive == null) {
            showStatus(R.string.a11yBackupStorageUnavailable, true);
            return;
        }
        if (export) {
            runBackupOperation(() -> {
                OutputStream out;
                try {
                    out = getContentResolver().openOutputStream(uri, "w");
                } catch (IOException e) {
                    throw new ReaderBackupArchive.BackupException(
                            ReaderBackupArchive.ERR_IO, String.valueOf(e.getMessage()));
                }
                if (out == null) {
                    throw new ReaderBackupArchive.BackupException(
                            ReaderBackupArchive.ERR_IO, "no output stream for chosen file");
                }
                return backupArchive.exportTo(out);
            }, summary -> showStatusText(getString(R.string.a11yBackupExportDone,
                    summary.files, summary.bytes), true));
        } else {
            runBackupOperation(() -> {
                InputStream in;
                try {
                    in = getContentResolver().openInputStream(uri);
                } catch (IOException e) {
                    throw new ReaderBackupArchive.BackupException(
                            ReaderBackupArchive.ERR_INVALID, String.valueOf(e.getMessage()));
                }
                if (in == null) {
                    throw new ReaderBackupArchive.BackupException(
                            ReaderBackupArchive.ERR_INVALID, "no input stream for chosen file");
                }
                try (InputStream stream = in) {
                    return backupArchive.stageImport(stream);
                } catch (IOException e) {
                    throw new ReaderBackupArchive.BackupException(
                            ReaderBackupArchive.ERR_IO, String.valueOf(e.getMessage()));
                }
            }, summary -> {
                stagedBackupReady = true;
                updateBackupButtons();
                showStatusText(getString(R.string.a11yBackupImportReady,
                        summary.files, summary.bytes), true);
            });
        }
    }

    /**
     * The only restore trigger: an explicit activation of the Confirm
     * button for the import staged and announced in this same screen
     * instance. There is no other caller of restoreStagedImport, and in
     * particular nothing in onResume/onActivityResult restores on its own.
     */
    private void confirmBackupRestore() {
        if (!ReaderGameSession.canRestore()) {
            showStatus(R.string.a11yBackupRequiresRestart, true);
            return;
        }
        if (backupBusy || !stagedBackupReady || backupArchive == null) {
            return;
        }
        runBackupOperation(() -> backupArchive.restoreStagedImport(), summary -> {
            stagedBackupReady = false;
            updateBackupButtons();
            showStatusText(getString(R.string.a11yBackupRestoreDone, summary.files), true);
        });
    }

    private void cancelBackupRestore() {
        if (backupArchive != null) {
            backupArchive.cancelStagedImport();
        }
        stagedBackupReady = false;
        pendingBackupAction = BACKUP_ACTION_NONE;
        updateBackupButtons();
        showStatus(R.string.a11yBackupRestoreCancelled, true);
    }

    private void updateBackupButtons() {
        if (backupExport == null || backupImport == null || backupOpenPicker == null
                || backupConfirmRestore == null || backupCancelRestore == null) {
            return;
        }
        backupExport.setEnabled(!backupBusy && backupArchive != null);
        backupImport.setEnabled(!backupBusy && backupArchive != null);
        backupOpenPicker.setEnabled(!backupBusy && pendingBackupAction != BACKUP_ACTION_NONE);
        backupConfirmRestore.setEnabled(!backupBusy && stagedBackupReady && ReaderGameSession.canRestore());
        backupCancelRestore.setEnabled(!backupBusy && stagedBackupReady);
    }

    private interface BackupWork {
        ReaderBackupArchive.Summary run() throws ReaderBackupArchive.BackupException;
    }

    private interface BackupDone {
        void onDone(ReaderBackupArchive.Summary summary);
    }

    /**
     * Backup file work runs off the main thread; its result is only
     * announced if this exact screen instance is still alive and still
     * owns the operation, so a completion from before a recreate can
     * never speak or change state in the new instance.
     */
    private void runBackupOperation(BackupWork work, BackupDone done) {
        backupBusy = true;
        updateBackupButtons();
        showStatus(R.string.a11yBackupWorking, true);
        final int operation = ++backupOperationSequence;
        new Thread(() -> {
            try {
                ReaderBackupArchive.Summary summary = work.run();
                runOnUiThread(() -> {
                    if (operation != backupOperationSequence || isFinishing() || isDestroyed()) {
                        return;
                    }
                    backupBusy = false;
                    updateBackupButtons();
                    done.onDone(summary);
                });
            } catch (ReaderBackupArchive.BackupException failure) {
                runOnUiThread(() -> {
                    if (operation != backupOperationSequence || isFinishing() || isDestroyed()) {
                        return;
                    }
                    backupBusy = false;
                    stagedBackupReady = backupArchive != null && backupArchive.hasStagedImport();
                    updateBackupButtons();
                    announceBackupError(failure);
                });
            } catch (RuntimeException failure) {
                runOnUiThread(() -> {
                    if (operation != backupOperationSequence || isFinishing() || isDestroyed()) {
                        return;
                    }
                    backupBusy = false;
                    stagedBackupReady = backupArchive != null && backupArchive.hasStagedImport();
                    updateBackupButtons();
                    showStatusText(getString(R.string.a11yBackupFailed,
                            ReaderBackupArchive.ERR_IO), true);
                });
            }
        }).start();
    }

    /** Every backup failure is spoken and states that nothing changed. */
    private void announceBackupError(ReaderBackupArchive.BackupException failure) {
        switch (failure.code) {
            case ReaderBackupArchive.ERR_NO_SPACE:
                showStatus(R.string.a11yBackupNoSpace, true);
                break;
            case ReaderBackupArchive.ERR_INVALID:
                showStatus(R.string.a11yBackupInvalid, true);
                break;
            case ReaderBackupArchive.ERR_VERIFY:
                showStatus(R.string.a11yBackupVerifyFailed, true);
                break;
            case ReaderBackupArchive.ERR_EMPTY:
                showStatus(R.string.a11yBackupEmpty, true);
                break;
            case ReaderBackupArchive.ERR_SWAP:
                showStatus(R.string.a11yBackupSwapFailed, true);
                break;
            case ReaderBackupArchive.ERR_GAME_ACTIVE:
                showStatus(R.string.a11yBackupRequiresRestart, true);
                break;
            default:
                showStatusText(getString(R.string.a11yBackupFailed, failure.code), true);
                break;
        }
    }

    // ---- Voice selection (R06.1) ----

    private void startVoiceProbe(String engineName) {
        voiceProbeRunning = true;
        voiceProbeFailed = false;
        updateVoiceChoiceDescriptions();
        AndroidSpeech.probeVoices(this, engineName, (engines, voices, success) -> {
            if (isFinishing() || isDestroyed()) {
                return;
            }
            voiceProbeRunning = false;
            voiceProbeFailed = !success;
            probedEngines.clear();
            probedEngines.addAll(engines);
            probedVoices.clear();
            probedVoices.addAll(voices);
            if (success) {
                adoptProbedDefaults();
            }
            updateVoiceChoiceDescriptions();
        });
    }

    /** Keep the draft valid against what the probed engine really offers. */
    private void adoptProbedDefaults() {
        if (probedVoices.isEmpty()) {
            draftVoiceName = null;
            draftVoiceNetwork = false;
            return;
        }
        boolean languagePresent = false;
        boolean voicePresent = false;
        for (AndroidSpeech.VoiceOption voice : probedVoices) {
            if (voice.locale.toLanguageTag().equals(draftLanguageTag)) {
                languagePresent = true;
            }
            if (voice.voiceName.equals(draftVoiceName)) {
                voicePresent = true;
                draftVoiceNetwork = voice.networkRequired;
            }
        }
        if (!languagePresent) {
            draftLanguageTag = probedVoices.get(0).locale.toLanguageTag();
            for (AndroidSpeech.VoiceOption voice : probedVoices) {
                if ("vi".equals(voice.locale.getLanguage())) {
                    draftLanguageTag = voice.locale.toLanguageTag();
                    break;
                }
            }
            voicePresent = false;
        }
        if (!voicePresent) {
            AndroidSpeech.VoiceOption chosen = null;
            for (AndroidSpeech.VoiceOption voice : probedVoices) {
                if (voice.locale.toLanguageTag().equals(draftLanguageTag)
                        && !voice.networkRequired) {
                    chosen = voice;
                    break;
                }
            }
            if (chosen == null) {
                for (AndroidSpeech.VoiceOption voice : probedVoices) {
                    if (voice.locale.toLanguageTag().equals(draftLanguageTag)) {
                        chosen = voice;
                        break;
                    }
                }
            }
            if (chosen != null) {
                draftVoiceName = chosen.voiceName;
                draftVoiceNetwork = chosen.networkRequired;
            }
        }
    }

    private void cycleVoiceEngine() {
        ArrayList<String> options = new ArrayList<>();
        options.add(null);
        for (AndroidSpeech.EngineOption engine : probedEngines) {
            options.add(engine.name);
        }
        int index = options.indexOf(draftEngineName);
        draftEngineName = options.get(Math.floorMod(index + 1, options.size()));
        draftEngineLabel = null;
        for (AndroidSpeech.EngineOption engine : probedEngines) {
            if (engine.name.equals(draftEngineName)) {
                draftEngineLabel = engine.label;
            }
        }
        draftVoiceName = null;
        draftVoiceNetwork = false;
        invalidateSampleForChangedSettings();
        startVoiceProbe(draftEngineName);
    }

    private void cycleVoiceLanguage() {
        ArrayList<String> tags = new ArrayList<>();
        for (AndroidSpeech.VoiceOption voice : probedVoices) {
            String tag = voice.locale.toLanguageTag();
            if (!tags.contains(tag)) {
                tags.add(tag);
            }
        }
        if (tags.isEmpty()) {
            return;
        }
        int index = tags.indexOf(draftLanguageTag);
        draftLanguageTag = tags.get(Math.floorMod(index + 1, tags.size()));
        draftVoiceName = null;
        draftVoiceNetwork = false;
        adoptProbedDefaults();
        invalidateSampleForChangedSettings();
        updateVoiceChoiceDescriptions();
    }

    private void cycleVoiceVoice() {
        ArrayList<AndroidSpeech.VoiceOption> choices = voicesForDraftLanguage();
        if (choices.isEmpty()) {
            return;
        }
        int index = -1;
        for (int i = 0; i < choices.size(); i++) {
            if (choices.get(i).voiceName.equals(draftVoiceName)) {
                index = i;
            }
        }
        AndroidSpeech.VoiceOption next = choices.get(Math.floorMod(index + 1, choices.size()));
        draftVoiceName = next.voiceName;
        draftVoiceNetwork = next.networkRequired;
        invalidateSampleForChangedSettings();
        updateVoiceChoiceDescriptions();
    }

    private ArrayList<AndroidSpeech.VoiceOption> voicesForDraftLanguage() {
        ArrayList<AndroidSpeech.VoiceOption> result = new ArrayList<>();
        for (AndroidSpeech.VoiceOption voice : probedVoices) {
            if (voice.locale.toLanguageTag().equals(draftLanguageTag)) {
                result.add(voice);
            }
        }
        return result;
    }

    private void updateVoiceChoiceDescriptions() {
        if (languageChoice == null || engineChoice == null || voiceChoice == null
                || voiceDataStatus == null) {
            return;
        }
        if (voiceProbeRunning) {
            languageChoice.setText(R.string.a11yVoiceProbing);
            engineChoice.setText(R.string.a11yVoiceProbing);
            voiceChoice.setText(R.string.a11yVoiceProbing);
            voiceDataStatus.setText(R.string.a11yVoiceProbing);
            return;
        }
        if (draftEngineLabel == null && draftEngineName != null) {
            for (AndroidSpeech.EngineOption engine : probedEngines) {
                if (engine.name.equals(draftEngineName)) {
                    draftEngineLabel = engine.label;
                }
            }
        }
        String languageName = Locale.forLanguageTag(draftLanguageTag)
                .getDisplayName(Locale.getDefault());
        languageChoice.setText(getString(R.string.a11yVoiceLanguage, languageName));
        engineChoice.setText(getString(R.string.a11yVoiceEngine,
                draftEngineName == null ? getString(R.string.a11yVoiceEngineDefault)
                        : draftEngineLabel != null ? draftEngineLabel : draftEngineName));
        voiceChoice.setText(getString(R.string.a11yVoiceSelect,
                draftVoiceName == null ? getString(R.string.a11yVoiceDefaultVoice) : draftVoiceName));
        if (voiceProbeFailed) {
            voiceDataStatus.setText(R.string.a11yVoiceProbeFailed);
        } else if (draftVoiceName == null) {
            voiceDataStatus.setText(R.string.a11yVoiceDefaultData);
        } else {
            voiceDataStatus.setText(draftVoiceNetwork
                    ? R.string.a11yVoiceNetworkData : R.string.a11yVoiceOfflineData);
        }
        languageChoice.setContentDescription(languageChoice.getText());
        engineChoice.setContentDescription(engineChoice.getText());
        voiceChoice.setContentDescription(voiceChoice.getText());
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
                    || state == AndroidSpeech.State.MISSING_VOICE
                    || state == AndroidSpeech.State.NETWORK_REQUIRED) {
                selfVoiceInstructionsPending = true;
                ReaderPreferences preferences = new ReaderPreferences(this);
                speech.initialize(preferences.getSpeechEngineName(),
                        Locale.forLanguageTag(preferences.getSpeechLanguageTag()),
                        preferences.getRate(), !preferences.permitsNetworkVoice(),
                        preferences.getSpeechVoiceName());
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
        if (rescuePlayer != null) {
            rescuePlayer.stop();
        }
        if (speech != null) {
            speech.cancel();
        }
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        // A backup worker finishing after this instance is gone must not
        // announce or mutate anything in a successor instance (R20.3).
        backupOperationSequence++;
        if (sampleCues != null) {
            sampleCues.close();
            sampleCues = null;
        }
        if (rescuePlayer != null) {
            rescuePlayer.release();
            rescuePlayer = null;
        }
        if (speech != null) {
            speech.close();
        }
        super.onDestroy();
    }
}

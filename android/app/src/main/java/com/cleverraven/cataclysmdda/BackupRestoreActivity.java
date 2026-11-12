package com.cleverraven.cataclysmdda;

import android.app.Activity;
import android.app.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.view.accessibility.AccessibilityManager;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Locale;

/** Two-step safe backup workflow extracted from the retired reader setup UI. */
public final class BackupRestoreActivity extends Activity {
    private static final String BACKUP_JOURNAL_FILE = "reader_backup_journal.txt";
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

    @Override protected void onCreate(Bundle state) {
        super.onCreate(state);
        setTitle(R.string.a11yBackupRestoreTitle);
        initSurface();
        LinearLayout controls = new LinearLayout(this);
        controls.setOrientation(LinearLayout.VERTICAL);
        int padding = Math.round(16 * getResources().getDisplayMetrics().density);
        controls.setPadding(padding, padding, padding, padding);
        TextView heading = new TextView(this);
        heading.setText(R.string.a11yBackupHeading);
        controls.addView(heading);
        status = new TextView(this);
        status.setSaveEnabled(false);
        controls.addView(status);
        backupExport = button(controls, R.string.a11yBackupExport,
                view -> chooseBackupPicker(true));
        backupImport = button(controls, R.string.a11yBackupImport,
                view -> chooseBackupPicker(false));
        backupOpenPicker = button(controls, R.string.a11yBackupOpenPickerExport,
                view -> openBackupPicker());
        backupConfirmRestore = button(controls, R.string.a11yBackupConfirmRestore,
                view -> confirmBackupRestore());
        backupCancelRestore = button(controls, R.string.a11yBackupCancelRestore,
                view -> cancelBackupRestore());
        button(controls, R.string.a11yReaderReturn, view -> finish());
        File backupRoot = getExternalFilesDir(null);
        if (backupRoot != null) {
            backupArchive = new ReaderBackupArchive(backupRoot,
                    new File(getFilesDir(), BACKUP_JOURNAL_FILE));
            try {
                backupArchive.recoverInterruptedSwap();
                if (ReaderGameSession.canRestore()) { backupArchive.cancelStagedImport(); }
            } catch (ReaderBackupArchive.BackupException failure) {
                backupArchive = null;
                status.setText(R.string.a11yBackupRecoveryFailed);
            }
        } else {
            status.setText(R.string.a11yBackupStorageUnavailable);
        }
        updateBackupButtons();
        setContentView(surface);
        surface.setContent(controls);
        initSpeech();
    }

    private Button button(LinearLayout parent, int label, android.view.View.OnClickListener click) {
        Button button = new Button(this);
        button.setText(label);
        button.setOnClickListener(click);
        parent.addView(button);
        return button;
    }

    private void showStatus(int id, boolean announce) {
        showStatusText(getString(id), announce);
    }

    private void showStatusText(String message, boolean announce) {
        status.setText(message);
        if (!foreground || !announce) { return; }
        if (usesTalkBack()) {
            status.announceForAccessibility(message);
        } else {
            speakText(message);
        }
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
                return new ReaderPreferences(BackupRestoreActivity.this).isLeftHandedNavigation();
            }
            @Override public float swipeDistancePx() {
                return new ReaderPreferences(BackupRestoreActivity.this).getSwipeDistanceDp()
                        * getResources().getDisplayMetrics().density;
            }
            @Override public long swipeDurationLimitMs() {
                return new ReaderPreferences(BackupRestoreActivity.this).getSwipeDurationLimitMs();
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
        backupOperationSequence++;
        if (speech != null) { speech.close(); }
        super.onDestroy();
    }

}

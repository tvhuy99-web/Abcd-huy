package com.cleverraven.cataclysmdda;

import android.app.Activity;
import android.content.ActivityNotFoundException;
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
    private File backupRoot;
    private Button backupExport;
    private Button backupImport;
    private Button backupOpenPicker;
    private Button backupConfirmRestore;
    private Button backupCancelRestore;
    private int pendingBackupAction = BACKUP_ACTION_NONE;
    private boolean stagedBackupReady;
    // STT 7: a staged import the user explicitly confirmed while the
    // engine was running, waiting to be applied at the next launch.
    private boolean deferredRestorePending;
    private boolean backupBusy;
    private int backupOperationSequence;
    private TextView status;
    // Failure found while opening this screen (recovery failed, or no
    // storage): onCreate runs before the screen is foreground and before
    // speech is ready, so it cannot be announced yet. It is spoken once
    // speech becomes ready, instead of the heading (STT 105).
    private int pendingOpenAnnouncement;

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
        backupRoot = getExternalFilesDir(null);
        if (backupRoot != null) {
            backupArchive = new ReaderBackupArchive(backupRoot,
                    new File(getFilesDir(), BACKUP_JOURNAL_FILE));
            try {
                backupArchive.recoverInterruptedSwap();
                // A merely staged import still dies with its screen
                // instance (R20.3) — but never discard one the user
                // already confirmed for deferred restore (STT 7): that
                // confirmation is what the next launch applies, and
                // dropping it here would silently void it.
                if (ReaderGameSession.canRestore()
                        && !ReaderGameSession.hasDeferredRestore(backupRoot)) {
                    backupArchive.cancelStagedImport();
                }
            } catch (ReaderBackupArchive.BackupException failure) {
                backupArchive = null;
                status.setText(R.string.a11yBackupRecoveryFailed);
                pendingOpenAnnouncement = R.string.a11yBackupRecoveryFailed;
            }
            // A deferred restore confirmed earlier in this session (or in
            // a previous process, if this screen is ever reached before
            // the game starts): show it as pending instead of as a fresh
            // stage, and keep Cancel available. Confirm stays enabled
            // when the engine has not started, so the already-confirmed
            // stage can be restored immediately here instead of waiting
            // for the next launch.
            if (backupArchive != null && backupArchive.hasStagedImport()
                    && ReaderGameSession.hasDeferredRestore(backupRoot)) {
                deferredRestorePending = true;
                stagedBackupReady = true;
                status.setText(R.string.a11yBackupRestoreDeferred);
                pendingOpenAnnouncement = R.string.a11yBackupRestoreDeferred;
            }
        } else {
            status.setText(R.string.a11yBackupStorageUnavailable);
            pendingOpenAnnouncement = R.string.a11yBackupStorageUnavailable;
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
        // STT 7: import may be staged while the engine runs. Staging
        // only copies the chosen archive into the staging directory and
        // verifies it; no live tree is touched, so the running engine
        // cannot be disturbed. Only the restore itself is deferred.
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
                // A newly staged import is unconfirmed: staging a new
                // file voids any earlier deferred confirmation (the
                // marker died with the old staging directory, and this
                // flag must not claim otherwise).
                deferredRestorePending = false;
                updateBackupButtons();
                showStatusText(getString(R.string.a11yBackupImportReady,
                        summary.files, summary.bytes), true);
            });
        }
    }

    /**
     * The only restore trigger: an explicit activation of the Confirm
     * button for the import staged and announced in this same screen
     * instance. Nothing in onResume/onActivityResult restores on its own,
     * and a merely staged, unconfirmed import is never restored anywhere.
     * The one exception is the deferred execution of this same
     * confirmation: when the engine is already running, Confirm records
     * itself on disk and SplashScreen applies that exact staged import at
     * the next launch, before the engine starts (STT 7).
     */
    private void confirmBackupRestore() {
        if (backupBusy || !stagedBackupReady || backupArchive == null) {
            return;
        }
        if (!ReaderGameSession.canRestore()) {
            confirmDeferredRestore();
            return;
        }
        runBackupOperation(() -> backupArchive.restoreStagedImport(), summary -> {
            stagedBackupReady = false;
            deferredRestorePending = false;
            updateBackupButtons();
            showStatusText(getString(R.string.a11yBackupRestoreDone, summary.files), true);
        });
    }

    /**
     * STT 7 (deferred restore): the engine is running, so the swap
     * itself cannot run now. Record this explicit Confirm on disk as a
     * deferred restore and apply nothing yet: SplashScreen applies the
     * staged import at the next launch, before reserveGameStart(). No
     * live tree changes here, and the staged import can still be
     * cancelled from this screen until the app is closed.
     */
    private void confirmDeferredRestore() {
        try {
            ReaderGameSession.markDeferredRestore(backupRoot);
        } catch (IOException failure) {
            // The confirmation was not recorded, so nothing is pending:
            // keep the staged import so the user can try Confirm again.
            showStatusText(getString(R.string.a11yBackupFailed,
                    ReaderBackupArchive.ERR_IO), true);
            return;
        }
        deferredRestorePending = true;
        stagedBackupReady = false;
        updateBackupButtons();
        // STT 7: dedicated deferred-restore string — the restore now
        // applies automatically on reopen, so the old "restore there"
        // wording of a11yBackupRequiresRestart no longer applies here.
        showStatus(R.string.a11yBackupRestoreDeferred, true);
    }

    private void cancelBackupRestore() {
        if (backupArchive != null) {
            // Also deletes the deferred marker (it lives inside the
            // staging directory), so a cancelled deferred restore is
            // never applied at the next launch.
            backupArchive.cancelStagedImport();
        }
        stagedBackupReady = false;
        deferredRestorePending = false;
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
        // Cancel stays available while a deferred restore is pending, so
        // the user can still void the confirmation before restarting.
        backupCancelRestore.setEnabled(!backupBusy && (stagedBackupReady || deferredRestorePending));
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
                if (state == AndroidSpeech.State.READY && foreground) {
                    if (pendingOpenAnnouncement != 0) {
                        int announcement = pendingOpenAnnouncement;
                        pendingOpenAnnouncement = 0;
                        showStatus(announcement, true);
                        return;
                    }
                    if (!usesTalkBack()) {
                        surface.speakFocused();
                    }
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
        // TalkBack without the internal reader never gets a speech READY
        // callback, so announce an open failure here instead (STT 105).
        if (pendingOpenAnnouncement != 0 && usesTalkBack()) {
            int announcement = pendingOpenAnnouncement;
            pendingOpenAnnouncement = 0;
            showStatus(announcement, true);
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

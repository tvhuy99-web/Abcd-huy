package com.cleverraven.cataclysmdda;

import android.app.Activity;
import android.app.ActivityNotFoundException;
import android.app.AlertDialog;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.view.accessibility.AccessibilityManager;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Locale;

/**
 * Android Storage Access Framework boundary for manual game mods.
 * The game thread owns mod activation. This Activity only changes files,
 * then marks the mod catalogue dirty for the native world-menu loop.
 */
public final class ManualModsActivity extends Activity {
    static final String EXTRA_ACTION = "manual_mod_action";
    static final String EXTRA_MOD_ID = "manual_mod_id";
    private static final int REQUEST_IMPORT = 7401;
    private static final int REQUEST_EXPORT = 7402;

    private String operation;
    private String selectedMod;
    private String modsPath;
    private TextView status;
    private Button pickerButton;
    private boolean busy;
    private boolean foreground;
    private String pendingStatus;
    private BootstrapSelfVoiceSurface surface;
    private AndroidSpeech speech;
    private long utterance;

    @Override protected void onCreate(Bundle state) {
        super.onCreate(state);
        operation = getIntent().getStringExtra(EXTRA_ACTION);
        selectedMod = getIntent().getStringExtra(EXTRA_MOD_ID);
        if (!"import".equals(operation) && !"export".equals(operation)
                && !"delete".equals(operation)) {
            finish();
            return;
        }
        try {
            modsPath = CataclysmDDA.nativeUserModDir();
        } catch (UnsatisfiedLinkError unavailable) {
            finish();
            return;
        }
        setTitle(R.string.a11yManualModTitle);
        initSurface();

        LinearLayout controls = new LinearLayout(this);
        controls.setOrientation(LinearLayout.VERTICAL);
        int padding = Math.round(16 * getResources().getDisplayMetrics().density);
        controls.setPadding(padding, padding, padding, padding);
        TextView heading = new TextView(this);
        heading.setText(R.string.a11yManualModTitle);
        controls.addView(heading);
        status = new TextView(this);
        status.setSaveEnabled(false);
        status.setText("import".equals(operation) ? R.string.a11yManualModImportHint
                : "export".equals(operation) ? R.string.a11yManualModExportHint
                : R.string.a11yManualModDeleteWarning);
        controls.addView(status);

        if (!"delete".equals(operation)) {
            pickerButton = new Button(this);
            pickerButton.setText("import".equals(operation)
                    ? R.string.a11yManualModChooseZip : R.string.a11yManualModChooseExport);
            pickerButton.setOnClickListener(v -> openPicker());
            controls.addView(pickerButton);
        }
        Button guideButton = new Button(this);
        guideButton.setText(R.string.a11yGestureGuideTitle);
        guideButton.setOnClickListener(v -> showGestureGuide());
        controls.addView(guideButton);

        Button close = new Button(this);
        close.setText(R.string.a11yReaderReturn);
        close.setOnClickListener(v -> { if (!busy) { finish(); } });
        controls.addView(close);

        setContentView(surface);
        surface.setContent(controls);
        initSpeech();
        if (state == null) {
            if ("delete".equals(operation)) {
                surface.post(this::confirmDelete);
            } else {
                // The native section's top button already initiated this
                // action; open the system picker immediately, not another mod list.
                surface.post(this::openPicker);
            }
        }
    }

    private void initSurface() {
        surface = new BootstrapSelfVoiceSurface(this, new BootstrapSelfVoiceSurface.Listener() {
            @Override public boolean isActive() { return foreground && !usesTalkBack(); }
            @Override public void speak(String id, String text) { speakText(text); }
            @Override public void stopSpeech() { if (speech != null) { speech.cancel(); } }
            @Override public void goBack() { if (!busy) { finish(); } }
            @Override public void openContextActions() { showContextActions(); }
            @Override public float swipeDistancePx() {
                return new ReaderPreferences(ManualModsActivity.this).getSwipeDistanceDp()
                        * getResources().getDisplayMetrics().density;
            }
            @Override public long swipeDurationLimitMs() {
                return new ReaderPreferences(ManualModsActivity.this).getSwipeDurationLimitMs();
            }
        });
    }

    private void initSpeech() {
        speech = new AndroidSpeech(this, new AndroidSpeech.Listener() {
            @Override public void onState(AndroidSpeech.State state) {
                if (state == AndroidSpeech.State.READY && foreground) {
                    if (pendingStatus != null) {
                        String message = pendingStatus;
                        pendingStatus = null;
                        speakText(message);
                    } else if (!usesTalkBack()) {
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
            speech.speak("manual-mod-" + (++utterance), text);
        }
    }

    private void showStatus(String message) {
        status.setText(message);
        if (!foreground) {
            pendingStatus = message;
        } else if (usesTalkBack()) {
            status.announceForAccessibility(message);
        } else {
            if (speech == null || speech.getState() != AndroidSpeech.State.READY) {
                pendingStatus = message;
            } else {
                speakText(message);
            }
        }
    }

    /** New modal pages retain an explicit gesture help entry after long hold. */
    private void showContextActions() {
        new AlertDialog.Builder(this)
                .setTitle(R.string.a11yContextCommands)
                .setItems(new String[] { getString(R.string.a11yGestureGuideTitle) },
                        (dialog, which) -> showGestureGuide())
                .setNegativeButton(R.string.a11yManualModCancel, null)
                .show();
    }

    private void showGestureGuide() {
        new AlertDialog.Builder(this)
                .setTitle(R.string.a11yGestureGuideTitle)
                .setMessage(R.string.a11yManualModGestureHelp)
                .setPositiveButton(R.string.a11yContextCommandsClose, null)
                .show();
    }

    private void openPicker() {
        if (busy) { return; }
        boolean export = "export".equals(operation);
        Intent intent = new Intent(export ? Intent.ACTION_CREATE_DOCUMENT
                : Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("application/zip");
        if (export) {
            intent.putExtra(Intent.EXTRA_TITLE, selectedMod + ".zip");
        }
        try {
            startActivityForResult(intent, export ? REQUEST_EXPORT : REQUEST_IMPORT);
        } catch (ActivityNotFoundException failure) {
            showStatus(getString(R.string.a11yManualModPickerUnavailable));
        }
    }

    @Override protected void onActivityResult(int request, int result, Intent data) {
        super.onActivityResult(request, result, data);
        if (request != REQUEST_IMPORT && request != REQUEST_EXPORT) { return; }
        Uri selected = result == RESULT_OK && data != null ? data.getData() : null;
        if (selected == null) {
            showStatus(getString(R.string.a11yManualModCancelled));
            return;
        }
        runWork(() -> {
            if (request == REQUEST_IMPORT) {
                try (InputStream input = getContentResolver().openInputStream(selected)) {
                    if (input == null) { throw new IOException("Không mở được tệp ZIP"); }
                    String id = ManualModArchive.importZip(input, modsPath);
                    CataclysmDDA.nativeQueueManualModsChanged();
                    return getString(R.string.a11yManualModImported, id);
                }
            }
            try (OutputStream output = getContentResolver().openOutputStream(selected, "w")) {
                if (output == null) { throw new IOException("Không mở được tệp xuất"); }
                ManualModArchive.exportZip(output, modsPath, selectedMod);
                return getString(R.string.a11yManualModExported, selectedMod);
            }
        });
    }

    private void confirmDelete() {
        if (busy) { return; }
        new AlertDialog.Builder(this)
                .setTitle(R.string.a11yManualModDelete)
                .setMessage(getString(R.string.a11yManualModConfirmDelete, selectedMod))
                .setPositiveButton(R.string.a11yManualModDelete, (dialog, which) ->
                        runWork(() -> {
                            ManualModArchive.deleteMod(modsPath, selectedMod);
                            CataclysmDDA.nativeQueueManualModsChanged();
                            return getString(R.string.a11yManualModDeleted, selectedMod);
                        }))
                .setNegativeButton(R.string.a11yManualModCancel, (dialog, which) -> finish())
                .show();
    }

    private interface FileWork {
        String run() throws IOException;
    }

    private void runWork(FileWork work) {
        if (busy) { return; }
        busy = true;
        if (pickerButton != null) { pickerButton.setEnabled(false); }
        showStatus(getString(R.string.a11yManualModWorking));
        new Thread(() -> {
            String message;
            try {
                message = work.run();
            } catch (IOException | RuntimeException failure) {
                message = getString(R.string.a11yManualModFailed,
                        failure.getMessage() == null ? "" : failure.getMessage());
            }
            final String result = message;
            runOnUiThread(() -> {
                if (isFinishing() || isDestroyed()) { return; }
                busy = false;
                if (pickerButton != null) { pickerButton.setEnabled(true); }
                showStatus(result);
            });
        }, "manual-mod-file-io").start();
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
        super.onDestroy();
    }
}

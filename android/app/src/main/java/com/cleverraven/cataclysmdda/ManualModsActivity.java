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
    private LinearLayout homeControls;
    private LinearLayout visibleControls;
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
        homeControls = controls;
        showControls(homeControls);
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

    private LinearLayout newControls(int title) {
        LinearLayout content = new LinearLayout(this);
        content.setOrientation(LinearLayout.VERTICAL);
        int padding = Math.round(16 * getResources().getDisplayMetrics().density);
        content.setPadding(padding, padding, padding, padding);
        TextView label = new TextView(this);
        label.setText(title);
        content.addView(label);
        return content;
    }

    private Button addButton(LinearLayout content, int label,
                             android.view.View.OnClickListener listener) {
        Button button = new Button(this);
        button.setText(label);
        button.setOnClickListener(listener);
        content.addView(button);
        return button;
    }

    private void showControls(LinearLayout controls) {
        visibleControls = controls;
        // The native voice reader works on each Android sub-screen too,
        // including confirmation and help. Never use an unspoken AlertDialog.
        surface.setContent(controls);
    }

    /**
     * Every held menu retains the universal Gesture guide as its last
     * action, including the file picker's own confirmation/help panels.
     */
    private void showContextActions() {
        if (busy) { return; }
        final LinearLayout previous = visibleControls;
        LinearLayout menu = newControls(R.string.a11yContextCommands);
        addButton(menu, R.string.a11yManualModCancel, view -> showControls(previous));
        addButton(menu, R.string.a11yGestureGuideTitle, view -> showGestureGuide());
        showControls(menu);
    }

    private void showGestureGuide() {
        final LinearLayout previous = visibleControls;
        LinearLayout page = newControls(R.string.a11yGestureGuideTitle);
        TextView instructions = new TextView(this);
        instructions.setText(R.string.a11yManualModGestureHelp);
        page.addView(instructions);
        addButton(page, R.string.a11yReaderReturn, view -> showControls(previous));
        showControls(page);
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
        LinearLayout page = newControls(R.string.a11yManualModDelete);
        TextView warning = new TextView(this);
        warning.setText(getString(R.string.a11yManualModConfirmDelete, selectedMod));
        page.addView(warning);
        addButton(page, R.string.a11yManualModCancel, view -> finish());
        addButton(page, R.string.a11yManualModDelete, view -> runWork(() -> {
            ManualModArchive.deleteMod(modsPath, selectedMod);
            CataclysmDDA.nativeQueueManualModsChanged();
            return getString(R.string.a11yManualModDeleted, selectedMod);
        }));
        addButton(page, R.string.a11yGestureGuideTitle, view -> showGestureGuide());
        showControls(page);
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
                showControls(homeControls);
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

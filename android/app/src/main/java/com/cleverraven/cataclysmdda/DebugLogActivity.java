package com.cleverraven.cataclysmdda;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.view.accessibility.AccessibilityManager;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import java.io.OutputStream;
import java.util.Locale;

/** One manual recording switch, spoken-only toggle, export and clear. */
public final class DebugLogActivity extends Activity {
    private static final int EXPORT_FILE_REQUEST = 1917;
    private BootstrapSelfVoiceSurface surface;
    private AndroidSpeech speech;
    private boolean foreground;
    private byte[] pendingExport;
    private long utteranceSequence;
    private LinearLayout mainControls;
    private Button clearButton;
    private boolean confirmingClear;

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        SessionDebugLog.start(this);
        setTitle(R.string.a11yDebugLogTitle);
        SessionDebugLog.event("SCREEN", "debug_log_open");
        ReaderPreferences reader = new ReaderPreferences(this);
        surface = new BootstrapSelfVoiceSurface(this, new BootstrapSelfVoiceSurface.Listener() {
            @Override public boolean isActive() { return foreground && !usesTalkBack(); }
            @Override public void speak(String id, String text) { speakText(text); }
            @Override public void stopSpeech() {
                if (speech != null) { speech.cancel(); }
            }
            @Override public void goBack() { DebugLogActivity.this.onBackPressed(); }
            @Override public float swipeDistancePx() {
                return reader.getSwipeDistanceDp()
                        * getResources().getDisplayMetrics().density;
            }
            @Override public long swipeDurationLimitMs() {
                return reader.getSwipeDurationLimitMs();
            }
        });
        mainControls = new LinearLayout(this);
        mainControls.setOrientation(LinearLayout.VERTICAL);
        int padding = Math.round(16 * getResources().getDisplayMetrics().density);
        mainControls.setPadding(padding, padding, padding, padding);

        CheckBox enabled = new CheckBox(this);
        enabled.setText(R.string.a11yDebugLogEnabled);
        enabled.setChecked(SessionDebugLog.enabled(this));
        enabled.setOnCheckedChangeListener((button, checked) -> {
            SessionDebugLog.setEnabled(this, checked);
            // Stop/start the native timing switch immediately, even though
            // the game activity is paused behind this Android settings UI.
            // This JNI method changes atomics only, not world/game state.
            try {
                CataclysmDDA.nativeConfigureReaderPerformance(
                        reader.isSpokenOnlyWorldEnabled(), checked);
            } catch (UnsatisfiedLinkError unavailable) {
                // The next foreground reader rebind also applies the switch.
            }
        });
        mainControls.addView(enabled);

        Button export = new Button(this);
        export.setText(R.string.a11yDebugLogExport);
        export.setOnClickListener(view -> exportLog());
        mainControls.addView(export);

        clearButton = new Button(this);
        clearButton.setText(R.string.a11yDebugLogClear);
        clearButton.setOnClickListener(view -> showClearConfirmation());
        mainControls.addView(clearButton);

        setContentView(surface);
        surface.setContent(mainControls);

        speech = new AndroidSpeech(this, new AndroidSpeech.Listener() {
            @Override public void onState(AndroidSpeech.State state) {
                if (state == AndroidSpeech.State.READY && foreground && !usesTalkBack()) {
                    surface.speakFocused();
                }
            }
            @Override public void onUtterance(String id, AndroidSpeech.Event event) { }
        });
    }

    /** Keep confirmation inside the same self-voiced gesture surface.
     * A platform AlertDialog steals touches and leaves the internal reader
     * unable to activate the positive or negative button.
     */
    private void showClearConfirmation() {
        confirmingClear = true;
        clearButton.setText(R.string.a11yDebugLogClear);
        LinearLayout confirmation = new LinearLayout(this);
        confirmation.setOrientation(LinearLayout.VERTICAL);
        TextView question = new TextView(this);
        question.setText(R.string.a11yDebugLogConfirmClear);
        confirmation.addView(question);

        Button confirm = new Button(this);
        confirm.setText(R.string.a11yDebugLogConfirmClearAction);
        confirm.setOnClickListener(view -> {
            SessionDebugLog.clear(this);
            Toast.makeText(this, R.string.a11yDebugLogCleared,
                    Toast.LENGTH_SHORT).show();
            // Speak the result when focus returns, even without TalkBack.
            clearButton.setText(getString(R.string.a11yDebugLogCleared)
                    + ". " + getString(R.string.a11yDebugLogClear));
            showMainControls();
        });
        confirmation.addView(confirm);

        Button cancel = new Button(this);
        cancel.setText(R.string.a11yTtsCancel);
        cancel.setOnClickListener(view -> showMainControls());
        confirmation.addView(cancel);
        surface.setContent(confirmation);
    }

    private void showMainControls() {
        confirmingClear = false;
        // The third control remains Clear. Returning to it avoids making the
        // user navigate from the checkbox after cancelling a confirmation.
        surface.setContent(mainControls, 2);
    }

    @Override
    public void onBackPressed() {
        if (confirmingClear) {
            showMainControls();
        } else {
            super.onBackPressed();
        }
    }

    private void exportLog() {
        // Export includes only events explicitly captured in this launch.
        // It remains available after recording is manually switched off.
        byte[] bytes = SessionDebugLog.snapshot(this);
        if (bytes.length == 0) {
            Toast.makeText(this, R.string.a11yDebugLogEmpty, Toast.LENGTH_SHORT).show();
            return;
        }
        pendingExport = bytes;
        Intent picker = new Intent(Intent.ACTION_CREATE_DOCUMENT);
        picker.addCategory(Intent.CATEGORY_OPENABLE);
        picker.setType("text/plain");
        picker.putExtra(Intent.EXTRA_TITLE, "cataclysm-session-debug.txt");
        try {
            startActivityForResult(picker, EXPORT_FILE_REQUEST);
        } catch (RuntimeException failure) {
            pendingExport = null;
            SessionDebugLog.event("ERROR", "export_picker_failed " + failure);
            Toast.makeText(this, R.string.a11yDebugLogExportError,
                    Toast.LENGTH_LONG).show();
        }
    }

    @Override
    protected void onActivityResult(int request, int result, Intent data) {
        super.onActivityResult(request, result, data);
        if (request != EXPORT_FILE_REQUEST) { return; }
        byte[] bytes = pendingExport;
        pendingExport = null;
        if (result != RESULT_OK || data == null || data.getData() == null
                || bytes == null) {
            SessionDebugLog.event("CONTROL", "export_cancelled");
            return;
        }
        Uri target = data.getData();
        try (OutputStream stream = getContentResolver().openOutputStream(target, "w")) {
            if (stream == null) { throw new java.io.IOException("No output stream"); }
            stream.write(bytes);
            stream.flush();
            SessionDebugLog.event("CONTROL", "export_complete bytes=" + bytes.length);
            Toast.makeText(this, R.string.a11yDebugLogExported, Toast.LENGTH_SHORT).show();
        } catch (Exception error) {
            SessionDebugLog.event("ERROR", "export_failed " + error);
            Toast.makeText(this, R.string.a11yDebugLogExportError,
                    Toast.LENGTH_LONG).show();
        }
    }

    private boolean usesTalkBack() {
        AccessibilityManager manager = (AccessibilityManager)
                getSystemService(Context.ACCESSIBILITY_SERVICE);
        return manager != null && manager.isTouchExplorationEnabled();
    }

    private void speakText(String text) {
        if (foreground && !usesTalkBack() && speech != null
                && speech.getState() == AndroidSpeech.State.READY) {
            speech.speak("debug-log-" + (++utteranceSequence), text);
        }
    }

    @Override protected void onResume() {
        super.onResume();
        foreground = true;
        ReaderPreferences prefs = new ReaderPreferences(this);
        if (speech != null && prefs.isEnabled()) {
            speech.initialize(prefs.getSpeechEngineName(),
                    Locale.forLanguageTag(prefs.getSpeechLanguageTag()),
                    prefs.getRate(), !prefs.permitsNetworkVoice(),
                    prefs.getSpeechVoiceName(), prefs.getPitch(), prefs.getVolume());
        }
    }

    @Override protected void onPause() {
        foreground = false;
        if (speech != null) { speech.cancel(); }
        super.onPause();
    }

    @Override protected void onDestroy() {
        if (speech != null) { speech.close(); }
        SessionDebugLog.event("SCREEN", "debug_log_closed");
        super.onDestroy();
    }
}

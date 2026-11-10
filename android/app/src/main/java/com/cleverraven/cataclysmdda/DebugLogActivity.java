package com.cleverraven.cataclysmdda;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.view.accessibility.AccessibilityManager;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.LinearLayout;
import android.widget.Toast;

import java.io.OutputStream;
import java.util.Locale;

/** Three visible controls, with an explicit crash-recovery export choice. */
public final class DebugLogActivity extends Activity {
    private static final int EXPORT_FILE_REQUEST = 1917;
    private BootstrapSelfVoiceSurface surface;
    private AndroidSpeech speech;
    private boolean foreground;
    private byte[] pendingExport;
    private boolean exportingCrash;
    private long utteranceSequence;

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
            @Override public void goBack() { finish(); }
            @Override public boolean leftHandedNavigation() {
                return reader.isLeftHandedNavigation();
            }
            @Override public float swipeDistancePx() {
                return reader.getSwipeDistanceDp()
                        * getResources().getDisplayMetrics().density;
            }
            @Override public long swipeDurationLimitMs() {
                return reader.getSwipeDurationLimitMs();
            }
        });
        LinearLayout controls = new LinearLayout(this);
        controls.setOrientation(LinearLayout.VERTICAL);
        int padding = Math.round(16 * getResources().getDisplayMetrics().density);
        controls.setPadding(padding, padding, padding, padding);

        CheckBox enabled = new CheckBox(this);
        enabled.setText(R.string.a11yDebugLogEnabled);
        enabled.setChecked(SessionDebugLog.enabled(this));
        enabled.setOnCheckedChangeListener((button, checked) ->
                SessionDebugLog.setEnabled(this, checked));
        controls.addView(enabled);

        // These are independent of the ordinary crash/session journal. The
        // visual switch affects only the world scene; native menus still draw.
        // Adaptive profiling turns itself off after sustained fast walking.
        CheckBox spokenOnlyWorld = new CheckBox(this);
        spokenOnlyWorld.setText(R.string.a11ySpokenOnlyWorld);
        spokenOnlyWorld.setChecked(reader.isSpokenOnlyWorldEnabled());
        spokenOnlyWorld.setOnCheckedChangeListener((button, checked) -> {
            reader.setSpokenOnlyWorldEnabled(checked);
            SessionDebugLog.event("WORLD_VISUAL_STATE", "spoken_only=" + checked);
        });
        controls.addView(spokenOnlyWorld);

        CheckBox profiling = new CheckBox(this);
        profiling.setText(R.string.a11yAdaptiveProfiler);
        profiling.setChecked(reader.isPerformanceProfilingEnabled());
        profiling.setOnCheckedChangeListener((button, checked) -> {
            reader.setPerformanceProfilingEnabled(checked);
            SessionDebugLog.setVerboseWorldTrace(this, checked);
            SessionDebugLog.event("CONTROL",
                    checked ? "profile=manual_enabled" : "profile=manual_paused");
        });
        controls.addView(profiling);

        Button export = new Button(this);
        export.setText(R.string.a11yDebugLogExport);
        export.setOnClickListener(view -> chooseExport());
        controls.addView(export);

        Button clear = new Button(this);
        clear.setText(R.string.a11yDebugLogClear);
        clear.setOnClickListener(view -> new AlertDialog.Builder(this)
                .setMessage(R.string.a11yDebugLogConfirmClear)
                .setPositiveButton(android.R.string.ok, (dialog, which) -> {
                    SessionDebugLog.clear(this);
                    Toast.makeText(this, R.string.a11yDebugLogCleared,
                            Toast.LENGTH_SHORT).show();
                })
                .setNegativeButton(android.R.string.cancel, null)
                .show());
        controls.addView(clear);

        setContentView(surface);
        surface.setContent(controls);

        speech = new AndroidSpeech(this, new AndroidSpeech.Listener() {
            @Override public void onState(AndroidSpeech.State state) {
                if (state == AndroidSpeech.State.READY && foreground && !usesTalkBack()) {
                    surface.speakFocused();
                    if (SessionDebugLog.hasCrash(DebugLogActivity.this)) {
                        speakText(getString(R.string.a11yDebugLogCrashAvailable));
                    }
                }
            }
            @Override public void onUtterance(String id, AndroidSpeech.Event event) { }
        });
    }

    private void chooseExport() {
        if (SessionDebugLog.hasCrash(this)) {
            new AlertDialog.Builder(this)
                    .setTitle(R.string.a11yDebugLogExport)
                    .setItems(new CharSequence[]{
                            getString(R.string.a11yDebugLogPreviousCrash),
                            getString(R.string.a11yDebugLogCurrentSession)
                    }, (dialog, selected) -> exportLog(selected == 0))
                    .show();
        } else {
            exportLog(false);
        }
    }

    private void exportLog(boolean crash) {
        SessionDebugLog.event("CONTROL", "export_requested crash=" + crash);
        byte[] bytes = SessionDebugLog.snapshot(this, crash);
        if (bytes.length == 0) {
            Toast.makeText(this, R.string.a11yDebugLogEmpty, Toast.LENGTH_SHORT).show();
            return;
        }
        pendingExport = bytes;
        exportingCrash = crash;
        Intent picker = new Intent(Intent.ACTION_CREATE_DOCUMENT);
        picker.addCategory(Intent.CATEGORY_OPENABLE);
        picker.setType("text/plain");
        picker.putExtra(Intent.EXTRA_TITLE, crash
                ? "cataclysm-crash-recovery.txt" : "cataclysm-session-debug.txt");
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
            SessionDebugLog.event("CONTROL", "export_complete crash="
                    + exportingCrash + " bytes=" + bytes.length);
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

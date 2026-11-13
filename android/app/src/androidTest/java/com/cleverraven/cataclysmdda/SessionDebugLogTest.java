package com.cleverraven.cataclysmdda;

import android.content.Context;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.File;
import java.io.FileOutputStream;
import java.nio.charset.StandardCharsets;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

/** Runtime checks for one manual journal, a fresh file on each launch. */
@RunWith(AndroidJUnit4.class)
public final class SessionDebugLogTest {
    @Test
    public void manualOnlyRecordingAndNewLaunchNeverReuseOldData() throws Exception {
        Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        final boolean originalEnabled = SessionDebugLog.enabled(context);
        try {
            SessionDebugLog.start(context);
            SessionDebugLog.setEnabled(context, true);
            SessionDebugLog.clear(context);

            StringBuilder large = new StringBuilder();
            for (int i = 0; i < 21000; i++) { large.append('X'); }
            SessionDebugLog.event("BEGIN", "old_sentinel");
            for (int i = 0; i < 280; i++) {
                SessionDebugLog.event("ROTATION_TEST", i + ":" + large);
            }
            SessionDebugLog.event("END", "recent_sentinel");
            byte[] bounded = SessionDebugLog.snapshot(context);
            assertTrue(bounded.length > 0);
            assertTrue(bounded.length <= SessionDebugLog.MAX_BYTES);
            String current = new String(bounded, StandardCharsets.UTF_8);
            assertFalse(current.contains("old_sentinel"));
            assertTrue(current.contains("recent_sentinel"));

            // OFF is strict: ordinary events and even diagnostic errors
            // cannot write anything. Existing bytes remain exportable.
            SessionDebugLog.setEnabled(context, false);
            assertFalse(SessionDebugLog.isRecording());
            File activeJournal = new File(context.getFilesDir(), "session_debug/active.txt");
            long lengthBefore = activeJournal.length();
            SessionDebugLog.event("MUST_NOT_APPEAR", "disabled");
            SessionDebugLog.event("FATAL", "must_not_appear");
            assertTrue(lengthBefore == activeJournal.length());
            assertFalse(new String(SessionDebugLog.snapshot(context),
                    StandardCharsets.UTF_8).contains("MUST_NOT_APPEAR"));

            // Re-enabling in the same launch appends to the same journal.
            SessionDebugLog.setEnabled(context, true);
            SessionDebugLog.event("RESUME", "same_launch");
            assertTrue(new String(SessionDebugLog.snapshot(context),
                    StandardCharsets.UTF_8).contains("same_launch"));
            SessionDebugLog.finishNormally();

            // Simulate a previous run with a legacy crash marker. A new
            // start must discard BOTH, never report a recovered old session.
            File folder = new File(context.getFilesDir(), "session_debug");
            try (FileOutputStream active = new FileOutputStream(
                    new File(folder, "active.txt"))) {
                active.write("previous_fatal_sentinel\n".getBytes(StandardCharsets.UTF_8));
            }
            try (FileOutputStream marker = new FileOutputStream(
                    new File(folder, "active.marker"))) {
                marker.write("open".getBytes(StandardCharsets.UTF_8));
            }
            SessionDebugLog.start(context);
            assertTrue(SessionDebugLog.isRecording());
            String newLaunch = new String(SessionDebugLog.snapshot(context),
                    StandardCharsets.UTF_8);
            assertTrue(newLaunch.contains("SESSION"));
            assertFalse(newLaunch.contains("previous_fatal_sentinel"));
            assertFalse(newLaunch.contains("same_launch"));
            assertFalse(new File(folder, "active.marker").exists());

            SessionDebugLog.setEnabled(context, false);
            SessionDebugLog.finishNormally();
            SessionDebugLog.start(context);
            assertFalse(SessionDebugLog.isRecording());
            assertTrue(SessionDebugLog.snapshot(context).length == 0);
        } finally {
            SessionDebugLog.setEnabled(context, originalEnabled);
            SessionDebugLog.finishNormally();
        }
    }
}

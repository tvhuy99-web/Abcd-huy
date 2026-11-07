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

/** Runtime checks: bounded rotation, explicit toggle and crash preservation. */
@RunWith(AndroidJUnit4.class)
public final class SessionDebugLogTest {
    @Test
    public void rotateRecoverAndClearWithoutDeletingCrashEvidence() throws Exception {
        Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
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
        byte[] bounded = SessionDebugLog.snapshot(context, false);
        assertTrue(bounded.length > 0);
        assertTrue(bounded.length <= SessionDebugLog.MAX_BYTES);
        String current = new String(bounded, StandardCharsets.UTF_8);
        assertFalse(current.contains("old_sentinel"));
        assertTrue(current.contains("recent_sentinel"));

        SessionDebugLog.setEnabled(context, false);
        File activeJournal = new File(context.getFilesDir(),
                "session_debug/active.txt");
        long lengthBefore = activeJournal.length();
        SessionDebugLog.event("MUST_NOT_APPEAR", "disabled");
        assertTrue(lengthBefore == activeJournal.length());
        SessionDebugLog.setEnabled(context, true);
        SessionDebugLog.finishNormally();

        File folder = new File(context.getFilesDir(), "session_debug");
        assertFalse(new File(folder, "active.marker").exists());
        try (FileOutputStream active = new FileOutputStream(
                new File(folder, "active.txt"))) {
            active.write("previous_fatal_sentinel\n".getBytes(StandardCharsets.UTF_8));
        }
        try (FileOutputStream marker = new FileOutputStream(
                new File(folder, "active.marker"))) {
            marker.write("open".getBytes(StandardCharsets.UTF_8));
        }
        SessionDebugLog.start(context);
        assertTrue(SessionDebugLog.hasCrash(context));
        String recovered = new String(SessionDebugLog.snapshot(context, true),
                StandardCharsets.UTF_8);
        assertTrue(recovered.contains("previous_fatal_sentinel"));

        SessionDebugLog.finishNormally();
        assertTrue(new File(folder, "crash_recovery.txt").exists());
        SessionDebugLog.start(context);
        SessionDebugLog.clear(context);
        SessionDebugLog.finishNormally();
        assertFalse(new File(folder, "crash_recovery.txt").exists());
    }
}

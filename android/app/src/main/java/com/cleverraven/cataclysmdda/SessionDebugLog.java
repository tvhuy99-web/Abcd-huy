package com.cleverraven.cataclysmdda;

import android.content.Context;
import android.os.Build;
import android.os.SystemClock;
import android.util.Log;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.charset.StandardCharsets;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.util.TimeZone;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * One journal per game launch. The user controls all recording with exactly
 * one persistent switch. OFF means no events, measurements, crash logging or
 * native diagnostics, with no auto-enable, auto-pause or hidden exceptions.
 *
 * Every new process discards the preceding launch's active journal. Enabling
 * during a launch starts/appends to only that launch's file, and disabling
 * never destroys data already captured for the current launch. The user
 * explicitly exports it before the next launch if needed.
 *
 * Log files remain local and size-bounded, without automatic uploading.
 */
public final class SessionDebugLog {
    static final long MAX_BYTES = 5L * 1024 * 1024;
    private static final long KEEP_BYTES = 4L * 1024 * 1024;
    private static final String PREFS = "session_debug_log";
    private static final String ACTIVE = "active.txt";
    private static final Object LOCK = new Object();
    private static SessionDebugLog instance;

    private final Context app;
    private final File activeFile;
    private final Thread.UncaughtExceptionHandler previousHandler;
    private final AtomicBoolean fatal = new AtomicBoolean();
    private final long launchId;
    private volatile boolean recording;
    private boolean closed;

    private SessionDebugLog(Context context) {
        app = context.getApplicationContext();
        launchId = System.currentTimeMillis();
        File directory = new File(app.getFilesDir(), "session_debug");
        if (!directory.exists() && !directory.mkdirs()) {
            Log.w("SessionDebugLog", "Could not create session log directory");
        }
        File freshJournal = new File(directory, ACTIVE);

        // Never resurrect a previous launch, whether it ended normally or
        // crashed. Also clean up legacy crash-recovery files on upgrade.
        if (freshJournal.exists() && !freshJournal.delete()) {
            try (FileOutputStream empty = new FileOutputStream(freshJournal, false)) {
                empty.flush();
            } catch (IOException error) {
                Log.e("SessionDebugLog", "Cannot reset previous launch log", error);
                // A unique fallback prevents accidentally appending to a
                // previous launch even if deleting/truncating active.txt
                // failed on this device.
                freshJournal = new File(directory, "active-" + launchId + ".txt");
            }
        }
        activeFile = freshJournal;
        for (String legacy : new String[] {"active.marker", "crash_recovery.txt"}) {
            File oldFile = new File(directory, legacy);
            if (oldFile.exists() && !oldFile.delete()) {
                Log.w("SessionDebugLog", "Could not delete old " + legacy);
            }
        }
        recording = enabled(app);

        previousHandler = Thread.getDefaultUncaughtExceptionHandler();
        Thread.setDefaultUncaughtExceptionHandler((thread, error) -> {
            if (fatal.compareAndSet(false, true) && recording) {
                try {
                    record("FATAL", "thread=" + thread.getName()
                            + " exception=" + Log.getStackTraceString(error));
                } catch (Throwable ignored) {
                    // Delegate the original crash, even if journaling fails.
                }
            }
            if (previousHandler != null) {
                previousHandler.uncaughtException(thread, error);
            } else {
                android.os.Process.killProcess(android.os.Process.myPid());
                System.exit(10);
            }
        });
        if (recording) {
            record("SESSION", "START app=" + app.getPackageName()
                    + " version=" + BuildConfig.VERSION_NAME
                    + " sdk=" + Build.VERSION.SDK_INT
                    + " device=" + Build.MANUFACTURER + "/" + Build.MODEL
                    + " sessionId=" + launchId);
        }
    }

    public static void start(Context context) {
        synchronized (LOCK) {
            if (instance == null) {
                instance = new SessionDebugLog(context);
            }
        }
    }

    /** No SharedPreferences lookups, I/O or clock reads when recording is off. */
    public static boolean isRecording() {
        SessionDebugLog log = instance;
        return log != null && log.recording && !log.closed;
    }

    public static void event(String category, String detail) {
        SessionDebugLog log = instance;
        if (log != null && log.recording) {
            log.record(category, detail);
        }
    }

    public static boolean enabled(Context context) {
        return context.getApplicationContext().getSharedPreferences(
                PREFS, Context.MODE_PRIVATE).getBoolean("enabled", false);
    }

    public static void setEnabled(Context context, boolean enabled) {
        start(context);
        synchronized (LOCK) {
            SessionDebugLog log = instance;
            if (log == null || log.closed || log.recording == enabled) {
                return;
            }
            // The switch is authoritative. No native perf auto-pause may
            // change it. On OFF the guard takes effect before any later I/O.
            log.recording = enabled;
            context.getApplicationContext().getSharedPreferences(
                    PREFS, Context.MODE_PRIVATE).edit()
                    .putBoolean("enabled", enabled).apply();
            if (enabled) {
                log.recordLocked("SESSION", "RECORDING_ENABLED sessionId=" + log.launchId
                        + " app=" + log.app.getPackageName()
                        + " version=" + BuildConfig.VERSION_NAME);
            }
        }
    }

    /** Only the current launch. Exporting after OFF remains possible. */
    public static byte[] snapshot(Context context) {
        start(context);
        synchronized (LOCK) {
            SessionDebugLog log = instance;
            if (log == null) { return new byte[0]; }
            try {
                return readBounded(log.activeFile);
            } catch (IOException error) {
                Log.e("SessionDebugLog", "Export failed", error);
                return new byte[0];
            }
        }
    }

    public static void clear(Context context) {
        start(context);
        synchronized (LOCK) {
            SessionDebugLog log = instance;
            if (log == null) { return; }
            if (log.activeFile.exists() && !log.activeFile.delete()) {
                Log.w("SessionDebugLog", "Could not delete current session log");
            }
        }
    }

    /** Quit never writes while OFF. The next launch resets the file. */
    public static void finishNormally() {
        synchronized (LOCK) {
            SessionDebugLog log = instance;
            if (log == null || log.closed || log.fatal.get()) { return; }
            if (log.recording) {
                log.recordLocked("SESSION", "NORMAL_EXIT sessionId=" + log.launchId);
            }
            log.closed = true;
            log.recording = false;
            Thread.setDefaultUncaughtExceptionHandler(log.previousHandler);
            instance = null;
        }
    }

    private void record(String category, String detail) {
        if (!recording || closed) { return; }
        synchronized (LOCK) {
            if (!recording || closed) { return; }
            recordLocked(category, detail);
        }
    }

    /** Caller must hold LOCK and have already checked recording. */
    private void recordLocked(String category, String detail) {
        if (!recording || closed) { return; }
        String data = detail == null ? "" : detail;
        data = data.replace("\r", "\\r").replace("\n", "\\n");
        if (data.length() > 24576) {
            data = data.substring(0, 24576) + " [event_truncated]";
        }
        String line = utcNow() + " | elapsed_ms=" + SystemClock.elapsedRealtime()
                + " | " + category + " | " + data + "\n";
        byte[] bytes = line.getBytes(StandardCharsets.UTF_8);
        try {
            if (activeFile.length() + bytes.length > MAX_BYTES) {
                trimOldest();
            }
            try (FileOutputStream out = new FileOutputStream(activeFile, true)) {
                out.write(bytes);
            }
        } catch (IOException error) {
            Log.e("SessionDebugLog", "Journal write failed", error);
        }
    }

    private void trimOldest() throws IOException {
        byte[] tail = readBounded(activeFile);
        int from = (int) Math.max(0, tail.length - KEEP_BYTES);
        while (from < tail.length && tail[from] != '\n') { from++; }
        if (from < tail.length) { from++; }
        try (FileOutputStream out = new FileOutputStream(activeFile, false)) {
            out.write(("[older events evicted by 5 MiB limit]\n")
                    .getBytes(StandardCharsets.UTF_8));
            out.write(tail, from, tail.length - from);
        }
    }

    private static byte[] readBounded(File file) throws IOException {
        if (!file.isFile()) { return new byte[0]; }
        try (RandomAccessFile in = new RandomAccessFile(file, "r");
                ByteArrayOutputStream out = new ByteArrayOutputStream()) {
            long start = Math.max(0, in.length() - MAX_BYTES);
            in.seek(start);
            if (start > 0) { in.readLine(); }
            byte[] buffer = new byte[8192];
            int n;
            while ((n = in.read(buffer)) >= 0) { out.write(buffer, 0, n); }
            return out.toByteArray();
        }
    }

    private static String utcNow() {
        SimpleDateFormat time = new SimpleDateFormat(
                "yyyy-MM-dd'T'HH:mm:ss.SSS'Z'", Locale.US);
        time.setTimeZone(TimeZone.getTimeZone("UTC"));
        return time.format(new Date());
    }
}

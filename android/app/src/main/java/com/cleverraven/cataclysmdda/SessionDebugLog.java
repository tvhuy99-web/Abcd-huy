package com.cleverraven.cataclysmdda;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Build;
import android.os.SystemClock;
import android.util.Log;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
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
 * Per-process, crash-recoverable, bounded diagnostic journal.
 *
 * A clean exit deletes the active log and marker; an abnormal process death
 * leaves both, which the next launch retains separately for explicit export.
 * Never uploads anything, never stores gesture coordinates beyond the app,
 * and never records text-field contents or credentials intentionally.
 */
public final class SessionDebugLog {
    static final long MAX_BYTES = 5L * 1024 * 1024;
    private static final long KEEP_BYTES = 4L * 1024 * 1024;
    private static final String PREFS = "session_debug_log";
    private static final String ACTIVE = "active.txt";
    private static final String MARKER = "active.marker";
    private static final String CRASH = "crash_recovery.txt";
    private static final Object LOCK = new Object();
    private static SessionDebugLog instance;
    private final Context app;
    private final File directory;
    private final File activeFile;
    private final File markerFile;
    private final File crashFile;
    private final Thread.UncaughtExceptionHandler previousHandler;
    private final AtomicBoolean fatal = new AtomicBoolean();
    private volatile boolean closed;
    // Once native walking has proved stable, preserve important lifecycle
    // and crash events but stop recording high-frequency performance traces.
    private volatile boolean verboseWorldTrace;

    private SessionDebugLog(Context context) {
        app = context.getApplicationContext();
        verboseWorldTrace = new ReaderPreferences(app).isPerformanceProfilingEnabled();
        directory = new File(app.getFilesDir(), "session_debug");
        directory.mkdirs();
        activeFile = new File(directory, ACTIVE);
        markerFile = new File(directory, MARKER);
        crashFile = new File(directory, CRASH);

        // A marker is created before events. If the process disappeared
        // without a clean exit, its last file can still be exported.
        if (markerFile.exists() && activeFile.exists() && activeFile.length() > 0) {
            if (crashFile.exists() && !crashFile.delete()) {
                Log.w("SessionDebugLog", "Could not replace older crash journal");
            }
            if (!activeFile.renameTo(crashFile)) {
                try {
                    copyFile(activeFile, crashFile);
                    if (!activeFile.delete()) {
                        Log.w("SessionDebugLog", "Could not clear recovered file");
                    }
                } catch (IOException error) {
                    Log.e("SessionDebugLog", "Recovering previous journal failed", error);
                }
            }
        }
        try (FileOutputStream out = new FileOutputStream(activeFile, false);
                FileOutputStream marker = new FileOutputStream(markerFile, false)) {
            out.flush();
            marker.write("open".getBytes(StandardCharsets.UTF_8));
            marker.getFD().sync();
        } catch (IOException error) {
            Log.e("SessionDebugLog", "Starting session failed", error);
        }
        previousHandler = Thread.getDefaultUncaughtExceptionHandler();
        Thread.setDefaultUncaughtExceptionHandler((thread, error) -> {
            if (fatal.compareAndSet(false, true) && enabled(app)) {
                try {
                    recordForce("FATAL", "thread=" + thread.getName()
                            + " exception=" + Log.getStackTraceString(error));
                } catch (Throwable ignored) {
                    // Always delegate the original crash after best-effort logging.
                }
            }
            if (previousHandler != null) {
                previousHandler.uncaughtException(thread, error);
            } else {
                android.os.Process.killProcess(android.os.Process.myPid());
                System.exit(10);
            }
        });
        record("SESSION", "START app=" + app.getPackageName()
                + " version=" + BuildConfig.VERSION_NAME
                + " sdk=" + Build.VERSION.SDK_INT
                + " device=" + Build.MANUFACTURER + "/" + Build.MODEL
                + " sessionId=" + System.currentTimeMillis());
    }

    public static void start(Context context) {
        synchronized (LOCK) {
            if (instance == null) {
                instance = new SessionDebugLog(context);
            }
        }
    }

    private static SessionDebugLog current() {
        synchronized (LOCK) { return instance; }
    }

    public static void event(String category, String detail) {
        SessionDebugLog log = current();
        if (log != null) { log.record(category, detail); }
    }

    public static void setVerboseWorldTrace(Context context, boolean enabled) {
        start(context);
        SessionDebugLog log = current();
        if (log == null || log.verboseWorldTrace == enabled) { return; }
        log.verboseWorldTrace = enabled;
        log.record("CONTROL",
                enabled ? "world_trace=verbose_enabled" : "world_trace=verbose_suspended");
    }

    private static boolean isHighFrequencyTrace(String category, String detail) {
        if (category == null) { return false; }
        // Auto-pausing is a performance decision, not permission to hide new
        // failures. Always retain broken footsteps, speech errors, safety
        // warnings and native crash diagnostics.
        if (category.equals("WORLD_MOVE_BLOCKED")
                || category.equals("WORLD_STEP_BUSY")
                || category.equals("WORLD_STEP_DEFERRED")
                || category.equals("WORLD_PROFILE_STATE")
                || category.equals("WORLD_VISUAL_STATE")
                || category.equals("TTS_STATE")) {
            return false;
        }
        if (category.equals("TTS_EVENT")) {
            return detail == null || !(detail.contains("event=ERROR")
                    || detail.contains("event=REJECTED"));
        }
        if (category.equals("INTENT_RESULT")) {
            return detail == null || !(detail.contains("status=stale")
                    || detail.contains("status=disabled")
                    || detail.contains("status=unsupported")
                    || detail.contains("status=invalid_value")
                    || detail.contains("status=busy")
                    || detail.contains("status=error")
                    || detail.contains("status=failed"));
        }
        if (category.startsWith("WORLD_") || category.startsWith("GESTURE")
                || category.startsWith("INTENT") || category.startsWith("TTS_")
                || category.equals("NATIVE_ACTION") || category.equals("SNAPSHOT")
                || category.equals("NAVIGATE")) {
            return true;
        }
        // Retain modal/menu transitions for recovery but suppress the
        // repeated world screen update on each footstep.
        return category.equals("SCREEN") && detail != null
                && detail.contains("category=DEFAULTMODE");
    }

    public static boolean enabled(Context context) {
        return context.getApplicationContext().getSharedPreferences(
                PREFS, Context.MODE_PRIVATE).getBoolean("enabled", true);
    }

    public static void setEnabled(Context context, boolean enabled) {
        start(context);
        SessionDebugLog log = current();
        if (log == null) { return; }
        if (!enabled) { log.record("CONTROL", "recording=OFF"); }
        context.getApplicationContext().getSharedPreferences(
                PREFS, Context.MODE_PRIVATE).edit().putBoolean("enabled", enabled).apply();
        if (enabled) { log.record("CONTROL", "recording=ON"); }
    }

    public static boolean hasCrash(Context context) {
        start(context);
        SessionDebugLog log = current();
        return log != null && log.crashFile.exists() && log.crashFile.length() > 0;
    }

    /** Export snapshots are bounded and independent of subsequent deletion. */
    public static byte[] snapshot(Context context, boolean crash) {
        start(context);
        synchronized (LOCK) {
            SessionDebugLog log = instance;
            if (log == null) { return new byte[0]; }
            File file = crash ? log.crashFile : log.activeFile;
            try {
                byte[] own = readBounded(file);
                if (own.length == 0) { return own; }
                // Also attach the engine's own debug/crash tail. Native errors
                // do not always reach the Java accessibility callback.
                File external = log.app.getExternalFilesDir(null);
                if (external == null) { return own; }
                File config = new File(external, "config");
                String[] sources = crash
                        ? new String[] {"crash.log", "debug.log.prev", "debug.log"}
                        : new String[] {"crash.log", "debug.log"};
                ByteArrayOutputStream combined = new ByteArrayOutputStream();
                combined.write(own);
                for (String name : sources) {
                    File nativeFile = new File(config, name);
                    if (!nativeFile.isFile()) { continue; }
                    byte[] section = readTail(nativeFile, 192 * 1024);
                    combined.write(("\n[NATIVE_LOG " + name + " latest_bytes="
                            + section.length + "]\n").getBytes(StandardCharsets.UTF_8));
                    combined.write(section);
                    combined.write('\n');
                }
                byte[] result = combined.toByteArray();
                if (result.length <= MAX_BYTES) { return result; }
                int begin = result.length - (int) MAX_BYTES + 64;
                while (begin < result.length && result[begin] != '\n') { begin++; }
                if (begin < result.length) { begin++; }
                ByteArrayOutputStream bounded = new ByteArrayOutputStream();
                bounded.write("[EXPORT TRUNCATED TO 5 MiB]\n"
                        .getBytes(StandardCharsets.UTF_8));
                bounded.write(result, begin, result.length - begin);
                return bounded.toByteArray();
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
            if (log.crashFile.exists() && !log.crashFile.delete()) {
                Log.w("SessionDebugLog", "Cannot clear crash log");
            }
            try (FileOutputStream ignored = new FileOutputStream(log.activeFile, false)) {
                // Clearing is explicit; leave an empty active log for new events.
            } catch (IOException error) {
                Log.e("SessionDebugLog", "Cannot clear active log", error);
            }
        }
    }

    /**
     * Only a confirmed normal quit clears the active journal. Pausing or
     * losing the window is not exit and never discards diagnostic evidence.
     */
    public static void finishNormally() {
        SessionDebugLog log = current();
        if (log == null) { return; }
        synchronized (LOCK) {
            if (log.closed || log.fatal.get()) { return; }
            log.recordForce("SESSION", "NORMAL_EXIT delete_active");
            log.closed = true;
            if (log.activeFile.exists() && !log.activeFile.delete()) {
                Log.w("SessionDebugLog", "Cannot delete finished journal");
            }
            if (log.markerFile.exists() && !log.markerFile.delete()) {
                Log.w("SessionDebugLog", "Cannot delete session marker");
            }
            // Older crash-recovery evidence survives until manual export/delete.
            Thread.setDefaultUncaughtExceptionHandler(log.previousHandler);
            instance = null;
        }
    }

    private void record(String category, String detail) {
        if (closed || !enabled(app)) { return; }
        if (!verboseWorldTrace && isHighFrequencyTrace(category, detail)) { return; }
        recordForce(category, detail);
    }

    private void recordForce(String category, String detail) {
        synchronized (LOCK) {
            if (closed) { return; }
            String timestamp = utcNow();
            String data = detail == null ? "" : detail;
            data = data.replace("\r", "\\r").replace("\n", "\\n");
            // One very large stack trace cannot evict an entire session.
            if (data.length() > 24576) {
                data = data.substring(0, 24576) + " [event_truncated]";
            }
            String line = timestamp + " | elapsed_ms=" + SystemClock.elapsedRealtime()
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

    private static byte[] readTail(File file, int length) throws IOException {
        try (RandomAccessFile in = new RandomAccessFile(file, "r");
                ByteArrayOutputStream out = new ByteArrayOutputStream()) {
            long begin = Math.max(0L, in.length() - length);
            in.seek(begin);
            if (begin > 0) { in.readLine(); }
            byte[] buf = new byte[8192];
            int count;
            while ((count = in.read(buf)) >= 0) { out.write(buf, 0, count); }
            return out.toByteArray();
        }
    }

    private static void copyFile(File source, File destination) throws IOException {
        try (FileInputStream in = new FileInputStream(source);
                FileOutputStream out = new FileOutputStream(destination, false)) {
            byte[] buffer = new byte[8192];
            int count;
            while ((count = in.read(buffer)) >= 0) { out.write(buffer, 0, count); }
        }
    }

    private static String utcNow() {
        SimpleDateFormat time = new SimpleDateFormat(
                "yyyy-MM-dd'T'HH:mm:ss.SSS'Z'", Locale.US);
        time.setTimeZone(TimeZone.getTimeZone("UTC"));
        return time.format(new Date());
    }

}

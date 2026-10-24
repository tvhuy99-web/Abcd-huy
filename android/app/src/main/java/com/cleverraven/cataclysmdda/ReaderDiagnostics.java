package com.cleverraven.cataclysmdda;

import android.os.SystemClock;

import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/**
 * Bounded, content-free diagnostics for the reader layer (R21.4).
 *
 * The store holds event CODES and counters only — never utterance text,
 * never message text, never save or world names — so the spoken report
 * can be shared as a bug report without leaking the player's data. The
 * event ring is capped at {@link #MAX_EVENTS} entries and counters are
 * plain longs; nothing here grows with play time. Times are seconds
 * since this process started (monotonic), never wall-clock.
 *
 * One process-wide instance: the game screen's controller and queue
 * write, the setup screen reads the report. If no game session ran in
 * this process, the report says so instead of inventing data.
 */
public final class ReaderDiagnostics {
    private static final int MAX_EVENTS = 200;
    private static final int REPORT_EVENTS = 30;

    private static final ReaderDiagnostics INSTANCE = new ReaderDiagnostics();

    public static ReaderDiagnostics get() {
        return INSTANCE;
    }

    private final long startedAt = SystemClock.elapsedRealtime();
    private final ArrayDeque<String> events = new ArrayDeque<>();
    private final Map<String, Long> counters = new LinkedHashMap<>();
    private ReaderSpeechQueue.Stats speechStats;
    private boolean sessionSeen;

    private ReaderDiagnostics() { }

    /** Record one lifecycle/health event by its internal code. */
    public synchronized void event(String code) {
        sessionSeen = true;
        long seconds = (SystemClock.elapsedRealtime() - startedAt) / 1000;
        events.addLast("T+" + seconds + "s " + code);
        while (events.size() > MAX_EVENTS) {
            events.removeFirst();
        }
    }

    /** Increment a named counter (created on first use). */
    public synchronized void count(String code) {
        sessionSeen = true;
        Long value = counters.get(code);
        counters.put(code, value == null ? 1L : value + 1L);
    }

    public synchronized long counter(String code) {
        Long value = counters.get(code);
        return value == null ? 0L : value;
    }

    public synchronized void setSpeechStats(ReaderSpeechQueue.Stats stats) {
        speechStats = stats;
    }

    public synchronized ReaderSpeechQueue.Stats speechStats() {
        return speechStats;
    }

    public synchronized boolean hasSession() {
        return sessionSeen;
    }

    public synchronized long uptimeSeconds() {
        return (SystemClock.elapsedRealtime() - startedAt) / 1000;
    }

    public synchronized Map<String, Long> countersSnapshot() {
        return new LinkedHashMap<>(counters);
    }

    /** The newest events, oldest first, capped for the spoken report. */
    public synchronized List<String> recentEvents() {
        ArrayList<String> all = new ArrayList<>(events);
        if (all.size() <= REPORT_EVENTS) {
            return all;
        }
        return new ArrayList<>(all.subList(all.size() - REPORT_EVENTS, all.size()));
    }
}

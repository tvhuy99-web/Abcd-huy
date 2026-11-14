package com.cleverraven.cataclysmdda;

import android.os.SystemClock;

import java.util.ArrayDeque;
import java.util.LinkedHashMap;
import java.util.Map;

/**
 * Bounded, content-free diagnostics for the reader layer (R21.4).
 *
 * The store holds event CODES and counters only — never utterance text,
 * never message text, never save or world names — so a count can be shared
 * as a bug report without leaking the player's data. The event ring is
 * capped at {@link #MAX_EVENTS} entries and counters are plain longs;
 * nothing here grows with play time. Times are seconds since this
 * process started (monotonic), never wall-clock.
 *
 * One process-wide instance: the game screen's controller, the speech
 * queue and the cue player write event codes and counters. Nothing reads
 * a report back — there is no diagnostics screen — so this class only
 * collects; the live API is {@link #get()}, {@link #event(String)} and
 * {@link #count(String)}.
 */
public final class ReaderDiagnostics {
    private static final int MAX_EVENTS = 200;

    private static final ReaderDiagnostics INSTANCE = new ReaderDiagnostics();

    public static ReaderDiagnostics get() {
        return INSTANCE;
    }

    private final long startedAt = SystemClock.elapsedRealtime();
    private final ArrayDeque<String> events = new ArrayDeque<>();
    private final Map<String, Long> counters = new LinkedHashMap<>();

    private ReaderDiagnostics() { }

    /** Record one lifecycle/health event by its internal code. */
    public synchronized void event(String code) {
        long seconds = (SystemClock.elapsedRealtime() - startedAt) / 1000;
        events.addLast("T+" + seconds + "s " + code);
        while (events.size() > MAX_EVENTS) {
            events.removeFirst();
        }
    }

    /** Increment a named counter (created on first use). */
    public synchronized void count(String code) {
        Long value = counters.get(code);
        counters.put(code, value == null ? 1L : value + 1L);
    }
}

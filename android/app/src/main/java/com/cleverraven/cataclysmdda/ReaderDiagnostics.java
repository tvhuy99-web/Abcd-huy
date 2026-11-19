package com.cleverraven.cataclysmdda;

import android.os.SystemClock;

import java.util.ArrayDeque;
import java.util.LinkedHashMap;
import java.util.Map;

















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


    public synchronized void event(String code) {
        long seconds = (SystemClock.elapsedRealtime() - startedAt) / 1000;
        events.addLast("T+" + seconds + "s " + code);
        while (events.size() > MAX_EVENTS) {
            events.removeFirst();
        }
    }


    public synchronized void count(String code) {
        Long value = counters.get(code);
        counters.put(code, value == null ? 1L : value + 1L);
    }
}

package android.os;

/** Tiny monotonic clock stub for JVM-only speech tests. */
public final class SystemClock {
    private SystemClock() {}
    public static long elapsedRealtime() {
        return System.nanoTime() / 1000000L;
    }
}

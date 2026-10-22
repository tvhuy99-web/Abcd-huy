package android.os;

import java.util.Comparator;
import java.util.PriorityQueue;

/** Deterministic serial looper for host tests, not Android emulation. */
public final class Looper {
    static final class Task {
        final long time;
        final long sequence;
        final Runnable runnable;

        Task(long time, long sequence, Runnable runnable) {
            this.time = time;
            this.sequence = sequence;
            this.runnable = runnable;
        }
    }

    private static final Looper MAIN = new Looper();
    static final PriorityQueue<Task> TASKS = new PriorityQueue<>(
            Comparator.comparingLong((Task t) -> t.time).thenComparingLong(t -> t.sequence));
    static long now;
    static long sequence;

    public static Looper getMainLooper() {
        return MAIN;
    }

    public static void reset() {
        TASKS.clear();
        now = 0;
        sequence = 0;
    }

    public static void runReady() {
        int guard = 0;
        while (!TASKS.isEmpty() && TASKS.peek().time <= now) {
            if (++guard > 1000) {
                throw new AssertionError("Runaway callback loop");
            }
            TASKS.remove().runnable.run();
        }
    }

    public static void advance(long millis) {
        now += millis;
        runReady();
    }
}

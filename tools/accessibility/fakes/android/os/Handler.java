package android.os;

public final class Handler {
    public Handler(Looper looper) {
        if (looper == null) {
            throw new IllegalArgumentException("looper");
        }
    }

    public boolean post(Runnable runnable) {
        return postDelayed(runnable, 0);
    }

    public boolean postDelayed(Runnable runnable, long delay) {
        Looper.TASKS.add(new Looper.Task(Looper.now + delay, ++Looper.sequence, runnable));
        return true;
    }

    public void removeCallbacks(Runnable runnable) {
        Looper.TASKS.removeIf(task -> task.runnable == runnable);
    }
}

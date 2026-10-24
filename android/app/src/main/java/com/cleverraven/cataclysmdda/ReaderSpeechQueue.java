package com.cleverraven.cataclysmdda;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;

import java.text.BreakIterator;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Locale;

/** Main-looper policy above AndroidSpeech: bounded, expiring and context-owned. */
public final class ReaderSpeechQueue implements AutoCloseable {
    public static final int LONG_TEXT = 0;
    public static final int INFORMATION = 1;
    public static final int FOCUS = 2;
    public static final int URGENT = 3;

    public interface Listener {
        void onHealth(AndroidSpeech.State state);

        /**
         * The confirmed voice selection no longer exists on this device
         * and the queue fell back to the system default voice (R06.2).
         * The stale selection has already been cleared from preferences;
         * the user can pick a new voice in reader setup.
         */
        default void onVoiceRecovered() { }
    }

    /**
     * Completion/interruption notice for one speech key (R04.4). The
     * document reader uses it to know whether its current paragraph
     * finished or was cut off by higher-priority speech, so it can resume
     * at the paragraph instead of restarting the whole document. The
     * listener must only record state; it must not speak from inside the
     * callback (the queue is mid-transition).
     */
    public interface CompletionListener {
        void onFinished(String key);
        void onInterrupted(String key);
    }

    private static final int MAX_PENDING = 8;
    // Duplicate suppression (R21.1): the game announces bursts of similar
    // events, and identical wording inside a short window carries no new
    // information for the listener — the engine already coalesces exact
    // repeats inside its own message log, this is the same policy at the
    // speech boundary. Urgent warnings are never dropped by the window;
    // they are still deduplicated against the active and pending requests
    // so a repeated warning cannot queue up copies of itself.
    private static final long DUPLICATE_WINDOW_MS = 4000;
    private final Handler handler = new Handler(Looper.getMainLooper());
    private final ArrayDeque<Request> pending = new ArrayDeque<>();
    private final java.util.HashMap<String, Long> recentStarts = new java.util.HashMap<>();
    // Bounded counters for the diagnostics report (R21.4). Counts only —
    // never utterance text.
    private long statSpoken;
    private long statInterrupted;
    private long statDroppedDuplicate;
    private long statDroppedExpired;
    private long statEvicted;
    private long statLastStartLatencyMs = -1;
    private long statMaxStartLatencyMs = -1;
    private final AndroidSpeech speech;
    private final Listener listener;
    private String contextKey = "";
    private String lastText = "";
    private String lastKey = "";
    private Request active;
    private String activeChunk;
    private long sequence;
    private int chunkIndex;
    private Runnable focusDebounce;
    private Runnable completionTimeout;
    private boolean closed;
    private boolean healthy;
    private CompletionListener completionListener;

    // Bounded TTS recovery (R06.2). Initialization parameters are kept so a
    // failed engine can be retried a limited number of times and a removed
    // voice can fall back once to the system default; recovery never loops.
    private static final int MAX_INIT_RETRIES = 2;
    private static final long INIT_RETRY_BASE_MS = 1000;
    private ReaderPreferences initPreferences;
    private String initEngineName;
    private String initVoiceName;
    private Locale initLocale;
    private float initRate;
    private boolean initOfflineOnly;
    private int initRetries;
    private boolean voiceRecoveryUsed;
    private Runnable initRetry;

    public void setCompletionListener(CompletionListener listener) {
        completionListener = listener;
    }

    private void fireFinished(String key) {
        if (completionListener != null && key != null) {
            completionListener.onFinished(key);
        }
    }

    private void fireInterrupted(String key) {
        // Every path that cuts a request off (preemption, stop, lifecycle
        // clear, engine error, stale context) funnels through here, so
        // this is the single honest place to count interruptions (R21.4).
        statInterrupted++;
        if (completionListener != null && key != null) {
            completionListener.onInterrupted(key);
        }
    }

    private static final class Request {
        final String context;
        final String key;
        final String text;
        final int priority;
        final long created;
        final long expires;
        final List<String> chunks;

        Request(String context, String key, String text, int priority) {
            this(context, key, text, priority, priority == FOCUS ? 2000 : 15000);
        }

        Request(String context, String key, String text, int priority, long ttlMs) {
            this.context = context;
            this.key = key;
            this.text = text;
            this.priority = priority;
            created = SystemClock.elapsedRealtime();
            expires = created + ttlMs;
            chunks = chunks(text);
        }
    }

    /** Counters and latency marks for the diagnostics report (R21.4). */
    public static final class Stats {
        public final long spoken;
        public final long interrupted;
        public final long droppedDuplicate;
        public final long droppedExpired;
        public final long evicted;
        public final long lastStartLatencyMs;
        public final long maxStartLatencyMs;
        public final int pending;
        public final boolean speaking;

        Stats(long spoken, long interrupted, long droppedDuplicate, long droppedExpired,
              long evicted, long lastStartLatencyMs, long maxStartLatencyMs,
              int pending, boolean speaking) {
            this.spoken = spoken;
            this.interrupted = interrupted;
            this.droppedDuplicate = droppedDuplicate;
            this.droppedExpired = droppedExpired;
            this.evicted = evicted;
            this.lastStartLatencyMs = lastStartLatencyMs;
            this.maxStartLatencyMs = maxStartLatencyMs;
            this.pending = pending;
            this.speaking = speaking;
        }
    }

    public Stats snapshotStats() {
        return new Stats(statSpoken, statInterrupted, statDroppedDuplicate, statDroppedExpired,
                statEvicted, statLastStartLatencyMs, statMaxStartLatencyMs,
                pending.size(), active != null);
    }

    public ReaderSpeechQueue(Context context, Listener listener) {
        this.listener = listener;
        speech = new AndroidSpeech(context, new AndroidSpeech.Listener() {
            @Override
            public void onState(AndroidSpeech.State state) {
                healthy = state == AndroidSpeech.State.READY;
                if (healthy) {
                    initRetries = 0;
                    cancelInitRetry();
                    listener.onHealth(state);
                    return;
                }
                clearRequests();
                if (closed) {
                    return;
                }
                // A confirmed voice that was uninstalled (or whose engine
                // was replaced) falls back once to the system default
                // voice; the stale selection is cleared so setup offers a
                // fresh choice instead of failing on every launch.
                if (state == AndroidSpeech.State.MISSING_VOICE && initVoiceName != null
                        && !voiceRecoveryUsed && initPreferences != null) {
                    voiceRecoveryUsed = true;
                    initPreferences.clearSpeechSelection();
                    initEngineName = null;
                    initVoiceName = null;
                    listener.onVoiceRecovered();
                    initializeSpeechEngine();
                    return;
                }
                // Bounded retry for transient engine/binder failures:
                // attempts at +1s and +3s, then the failure is reported.
                if (state == AndroidSpeech.State.FAILED && initPreferences != null
                        && initRetries < MAX_INIT_RETRIES) {
                    initRetries++;
                    cancelInitRetry();
                    final int attempt = initRetries;
                    initRetry = () -> {
                        initRetry = null;
                        if (!closed) {
                            initializeSpeechEngine();
                        }
                    };
                    handler.postDelayed(initRetry, INIT_RETRY_BASE_MS * (2L * attempt - 1));
                    return;
                }
                listener.onHealth(state);
            }

            @Override
            public void onUtterance(String id, AndroidSpeech.Event event) {
                if (activeChunk == null || !activeChunk.equals(id) || closed) {
                    return;
                }
                if (event == AndroidSpeech.Event.STARTED) {
                    // Latency mark for the diagnostics report (R21.4):
                    // time from enqueue to the engine actually starting
                    // this utterance. Recorded, never compared against a
                    // threshold here — no latency claim is made from it.
                    if (active != null) {
                        long latency = SystemClock.elapsedRealtime() - active.created;
                        statLastStartLatencyMs = latency;
                        if (latency > statMaxStartLatencyMs) {
                            statMaxStartLatencyMs = latency;
                        }
                    }
                    return;
                }
                removeCompletionTimeout();
                activeChunk = null;
                if (event == AndroidSpeech.Event.DONE) {
                    chunkIndex++;
                    if (active != null && chunkIndex < active.chunks.size()
                            && active.context.equals(contextKey)) {
                        speakChunk();
                    } else {
                        Request finished = active;
                        active = null;
                        statSpoken++;
                        if (finished != null && finished.context.equals(contextKey)) {
                            fireFinished(finished.key);
                        } else if (finished != null) {
                            fireInterrupted(finished.key);
                        }
                        advance();
                    }
                } else {
                    // Never advance through a backlog after a cancellation/error.
                    Request interrupted = active;
                    active = null;
                    for (Request dropped : pending) {
                        if (dropped.expires < SystemClock.elapsedRealtime()) {
                            statDroppedExpired++;
                        }
                    }
                    pending.clear();
                    if (interrupted != null) {
                        fireInterrupted(interrupted.key);
                    }
                }
            }
        });
    }

    public void initialize(ReaderPreferences preferences) {
        stop();
        cancelInitRetry();
        healthy = false;
        initRetries = 0;
        voiceRecoveryUsed = false;
        initPreferences = preferences;
        initEngineName = preferences.getSpeechEngineName();
        initVoiceName = preferences.getSpeechVoiceName();
        initLocale = Locale.forLanguageTag(preferences.getSpeechLanguageTag());
        initRate = preferences.getRate();
        initOfflineOnly = !preferences.permitsNetworkVoice();
        initializeSpeechEngine();
    }

    private void initializeSpeechEngine() {
        speech.initialize(initEngineName, initLocale, initRate, initOfflineOnly, initVoiceName);
    }

    private void cancelInitRetry() {
        if (initRetry != null) {
            handler.removeCallbacks(initRetry);
            initRetry = null;
        }
    }

    public boolean isReady() {
        return healthy && !closed;
    }

    public void setContext(String key) {
        if (!contextKey.equals(key)) {
            stop();
            contextKey = key;
            lastText = "";
            lastKey = "";
        }
    }

    public void focus(String key, String text) {
        if (focusDebounce != null) {
            handler.removeCallbacks(focusDebounce);
        }
        final String owner = contextKey;
        focusDebounce = () -> {
            focusDebounce = null;
            if (owner.equals(contextKey)) {
                say(key, text, FOCUS);
            }
        };
        handler.postDelayed(focusDebounce, 120);
    }

    public void say(String key, String text, int priority) {
        say(key, text, priority, priority == FOCUS ? 2000 : 15000);
    }

    /**
     * Enqueue with an explicit time-to-live. Game messages use a shorter
     * life than screen speech (R21.1): a burst of turn messages must not
     * still be talking about an old turn many seconds later, while an
     * urgent warning keeps the standard life.
     */
    public void say(String key, String text, int priority, long ttlMs) {
        if (!isReady() || text == null || text.trim().isEmpty()) {
            return;
        }
        if (active != null && active.context.equals(contextKey) && active.key.equals(key)
                && active.text.equals(text)) {
            return;
        }
        // Duplicate suppression (R21.1): identical wording that is already
        // speaking, already queued at an equal or higher priority, or was
        // started inside the duplicate window adds nothing. The window
        // never applies to URGENT — a repeated warning is spoken again —
        // but an urgent request still cannot duplicate the active or a
        // pending copy of itself.
        if (active != null && active.context.equals(contextKey) && active.text.equals(text)) {
            statDroppedDuplicate++;
            return;
        }
        for (Request queued : pending) {
            if (queued.context.equals(contextKey) && queued.text.equals(text)
                    && queued.priority >= priority) {
                statDroppedDuplicate++;
                return;
            }
        }
        if (priority < URGENT) {
            Long started = recentStarts.get(text);
            if (started != null
                    && SystemClock.elapsedRealtime() - started < DUPLICATE_WINDOW_MS) {
                statDroppedDuplicate++;
                return;
            }
        }
        Request request = new Request(contextKey, key, text, priority, ttlMs);
        for (Iterator<Request> iterator = pending.iterator(); iterator.hasNext();) {
            Request old = iterator.next();
            if ((old.key.equals(key) && old.priority <= priority)
                    || (priority == FOCUS && old.priority == FOCUS)
                    || (priority > FOCUS && old.priority == FOCUS)
                    || (priority >= URGENT && old.priority < URGENT)) {
                iterator.remove();
            }
        }
        if (active != null && (priority > active.priority ||
                (priority == FOCUS && active.priority == FOCUS))) {
            Request preempted = active;
            active = null;
            activeChunk = null;
            removeCompletionTimeout();
            speech.cancel();
            fireInterrupted(preempted.key);
        }
        // Drop expired work before applying the limit.  When full, preserve
        // higher-priority requests; if the new request ties the lowest
        // priority, replace its oldest item so recent state wins.
        long now = SystemClock.elapsedRealtime();
        for (Iterator<Request> iterator = pending.iterator(); iterator.hasNext();) {
            Request queued = iterator.next();
            if (!queued.context.equals(contextKey)) {
                iterator.remove();
            } else if (queued.expires < now) {
                iterator.remove();
                statDroppedExpired++;
            }
        }
        if (pending.size() >= MAX_PENDING) {
            Request lowest = null;
            for (Request queued : pending) {
                if (lowest == null || queued.priority < lowest.priority) {
                    lowest = queued;
                }
            }
            if (lowest != null && lowest.priority > priority) {
                statEvicted++;
                return;
            }
            if (lowest != null) {
                pending.remove(lowest);
                statEvicted++;
            }
        }
        pending.addLast(request);
        advance();
    }

    private void advance() {
        if (active != null || !isReady()) {
            return;
        }
        Request best = null;
        long now = SystemClock.elapsedRealtime();
        for (Iterator<Request> iterator = pending.iterator(); iterator.hasNext();) {
            Request request = iterator.next();
            if (!request.context.equals(contextKey)) {
                iterator.remove();
            } else if (request.expires < now) {
                iterator.remove();
                statDroppedExpired++;
            } else if (best == null || request.priority > best.priority) {
                best = request;
            }
        }
        if (best != null) {
            pending.remove(best);
            active = best;
            chunkIndex = 0;
            lastText = best.text;
            lastKey = best.key;
            // Start-time record for the duplicate window in say(): the
            // map is a bounded heuristic (cleared when it grows), losing
            // old entries only weakens suppression, never correctness.
            recentStarts.put(best.text, now);
            if (recentStarts.size() > 64) {
                recentStarts.clear();
                recentStarts.put(best.text, now);
            }
            speakChunk();
        }
    }

    private void speakChunk() {
        if (active == null || !active.context.equals(contextKey) || !isReady()) {
            return;
        }
        activeChunk = "reader-" + (++sequence);
        final String token = activeChunk;
        String chunk = active.chunks.get(chunkIndex);
        speech.speak(token, chunk);
        completionTimeout = () -> {
            if (token.equals(activeChunk)) {
                stop();
                healthy = false;
                listener.onHealth(AndroidSpeech.State.FAILED);
            }
        };
        handler.postDelayed(completionTimeout, Math.min(90000, 15000 + chunk.length() * 120L));
    }

    public void repeat() {
        String text = lastText;
        String key = lastKey;
        stop();
        say("repeat:" + key, text, FOCUS);
    }

    public void stop() {
        clearRequests();
        speech.cancel();
    }

    /**
     * Lifecycle boundary (R20.3): the screen lost the foreground or the
     * window focus, so nothing from before the boundary may be spoken
     * after it. {@link #stop()} alone is not enough for this: it keeps
     * {@code lastText}, so a Repeat control pressed after a resume could
     * replay an utterance from the previous foreground session, and it
     * keeps the context key, so a same-context snapshot after the resume
     * would not clear that text either ({@link #setContext} only clears
     * on a context change). A pending bounded TTS retry is also dropped:
     * the engine must not revive in the background, the owner's resume
     * path re-initializes the queue if it is not ready. Explicit Stop
     * inside one foreground session keeps using {@link #stop()}, where
     * Repeat remains available by design.
     */
    public void clearForLifecycle() {
        cancelInitRetry();
        clearRequests();
        speech.cancel();
        contextKey = "";
        lastText = "";
        lastKey = "";
        // The duplicate window must not reach across a lifecycle
        // boundary either: what was said before a pause says nothing
        // about what the fresh foreground session needs to hear.
        recentStarts.clear();
    }

    private void clearRequests() {
        if (focusDebounce != null) {
            handler.removeCallbacks(focusDebounce);
            focusDebounce = null;
        }
        removeCompletionTimeout();
        pending.clear();
        Request interrupted = active;
        active = null;
        activeChunk = null;
        if (interrupted != null) {
            fireInterrupted(interrupted.key);
        }
    }

    private void removeCompletionTimeout() {
        if (completionTimeout != null) {
            handler.removeCallbacks(completionTimeout);
            completionTimeout = null;
        }
    }

    private static List<String> chunks(String text) {
        ArrayList<String> result = new ArrayList<>();
        BreakIterator boundaries = BreakIterator.getCharacterInstance(new Locale("vi"));
        boundaries.setText(text);
        int offset = 0;
        while (offset < text.length()) {
            int end = Math.min(text.length(), offset + 700);
            if (end < text.length()) {
                int space = text.lastIndexOf(' ', end);
                if (space > offset + 250) {
                    end = space;
                }
                if (!boundaries.isBoundary(end)) {
                    end = boundaries.preceding(end);
                }
                if (end <= offset) {
                    end = boundaries.following(offset);
                }
            }
            result.add(text.substring(offset, end));
            offset = end;
        }
        return result;
    }

    @Override
    public void close() {
        closed = true;
        healthy = false;
        cancelInitRetry();
        clearRequests();
        speech.close();
    }
}

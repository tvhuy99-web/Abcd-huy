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
    // Normal game events remain FIFO. Urgent player warnings take the next
    // available speech turn ahead of older nonurgent messages, but do not
    // cut off speech that is already playing. Focus remains independent.
    public static final int GAME_EVENT = FOCUS;
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
    private static final String GAME_EVENT_CONTEXT = "__reader_game_event__";
    // Game messages survive temporary screen changes and retain order
    // *within* their own urgency level. Active speech is never cut off by a
    // newer event, but warnings do not wait behind long ordinary chatter.
    private final ArrayDeque<Request> orderedGameEvents = new ArrayDeque<>();
    private final ArrayDeque<Request> urgentGameEvents = new ArrayDeque<>();

    private static boolean isOrderedGameEvent(Request request) {
        return request != null && GAME_EVENT_CONTEXT.equals(request.context);
    }

    // Only identical in-flight informational messages are coalesced.
    // A focus gesture must always announce its newly selected item, even if
    // the same text was spoken moments ago.
    private final Handler handler = new Handler(Looper.getMainLooper());
    private final ArrayDeque<Request> pending = new ArrayDeque<>();
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
    // Chunk of the last utterance where speech stopped (STT 175). Updated
    // whenever an utterance is cut off, so resume() can continue a long
    // document from the interrupted chunk instead of restarting it.
    private int lastChunkIndex;
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
    private float initPitch = 1.0f;
    private float initVolume = 1.0f;
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
        if (key != null && key.equals(lastKey)) {
            // The utterance ran to the end; a later resume starts over.
            lastChunkIndex = 0;
        }
        if (completionListener != null && key != null) {
            completionListener.onFinished(key);
        }
    }

    private void fireInterrupted(String key) {
        // Every path that cuts a request off (preemption, stop, lifecycle
        // clear, engine error, stale context) funnels through here, so
        // this is the single honest place to count interruptions (R21.4).
        statInterrupted++;
        if (key != null && key.equals(lastKey)) {
            // chunkIndex is the chunk being spoken when the cut happened;
            // resume() replays that chunk (STT 175).
            lastChunkIndex = chunkIndex;
        }
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
        // Chunk to start speaking from (STT 175). Normally 0; resume()
        // sets it to the interrupted chunk so a long document continues
        // instead of restarting.
        final int startChunk;

        Request(String context, String key, String text, int priority) {
            this(context, key, text, priority, priority == FOCUS ? 2000 : 15000);
        }

        Request(String context, String key, String text, int priority, long ttlMs) {
            this(context, key, text, priority, ttlMs, 0);
        }

        Request(String context, String key, String text, int priority, long ttlMs,
                int startChunk) {
            this.context = context;
            this.key = key;
            this.text = text;
            this.priority = priority;
            created = SystemClock.elapsedRealtime();
            expires = created + ttlMs;
            chunks = chunks(text);
            this.startChunk = startChunk;
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
                pending.size() + orderedGameEvents.size() + urgentGameEvents.size(),
                active != null);
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
                    advance();
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
                            && (isOrderedGameEvent(active) || active.context.equals(contextKey))) {
                        speakChunk();
                    } else {
                        Request finished = active;
                        active = null;
                        statSpoken++;
                        if (finished != null && (isOrderedGameEvent(finished)
                                || finished.context.equals(contextKey))) {
                            fireFinished(finished.key);
                        } else if (finished != null) {
                            fireInterrupted(finished.key);
                        }
                        advance();
                    }
                } else {
                    // An unexpected voice-engine cancellation must not erase
                    // *other* game messages. Only explicit Stop or lifecycle
                    // clearing deliberately empties the FIFO.
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
                    advance();
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
        initPitch = preferences.getPitch();
        initVolume = preferences.getVolume();
        initOfflineOnly = !preferences.permitsNetworkVoice();
        initializeSpeechEngine();
    }

    private void initializeSpeechEngine() {
        speech.initialize(initEngineName, initLocale, initRate, initOfflineOnly,
                initVoiceName, initPitch, initVolume);
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
            contextKey = key;
            // Only UI focus belongs to a screen. An ongoing game event
            // survives transient aiming/activity screens and keeps talking
            // until complete, never getting erased by another snapshot.
            pending.clear();
            if (active != null && !isOrderedGameEvent(active)) {
                Request interrupted = active;
                active = null;
                activeChunk = null;
                removeCompletionTimeout();
                speech.cancel();
                fireInterrupted(interrupted.key);
            }
            lastText = "";
            lastKey = "";
            lastChunkIndex = 0;
            advance();
        }
    }

    public void focus(String key, String text) {
        // Do not debounce a deliberate swipe. say() preempts the previous
        // focus utterance and replaces pending focus speech, so when swipes
        // come in quick succession only the final selection remains active.
        say(key, text, FOCUS);
    }

    public void say(String key, String text, int priority) {
        say(key, text, priority, priority == FOCUS ? 2000 : 15000);
    }

    public void say(String key, String text, int priority, long ttlMs) {
        say(key, text, priority, ttlMs, 0);
    }

    /**
     * Enqueue with an explicit time-to-live. Game messages use a shorter
     * life than screen speech (R21.1): a burst of turn messages must not
     * still be talking about an old turn many seconds later, while an
     * urgent warning keeps the standard life.
     */
    public void say(String key, String text, int priority, long ttlMs, int startChunk) {
        if (!isReady() || text == null || text.trim().isEmpty()) {
            return;
        }
        text = ReaderSpokenText.clean(text);
        if (text.trim().isEmpty()) { return; }
        if (key != null && key.startsWith("game-message:")) {
            // Urgent warnings must reach the player before older, nonurgent
            // chatter. Preserve FIFO within each severity; never interrupt
            // an active message. Keep normal damage reports even when equal.
            Request request = new Request(GAME_EVENT_CONTEXT, key, text,
                    priority >= URGENT ? URGENT : GAME_EVENT,
                    365L * 24 * 60 * 60 * 1000, startChunk);
            if (request.priority >= URGENT) {
                // A persistent status can fire several times in one turn.
                // Only coalesce identical urgent messages that are already
                // being spoken or waiting; distinct threat changes survive.
                if (active != null && isOrderedGameEvent(active)
                        && active.priority >= URGENT && active.text.equals(text)) {
                    statDroppedDuplicate++;
                    return;
                }
                for (Request old : urgentGameEvents) {
                    if (old.text.equals(text)) {
                        statDroppedDuplicate++;
                        return;
                    }
                }
                urgentGameEvents.addLast(request);
            } else {
                orderedGameEvents.addLast(request);
            }
            advance();
            return;
        }
        // Focus is user-directed, even when it repeats the same label.
        // For other speech, suppress only duplicate requests still active
        // or waiting in this context; never suppress based on elapsed time.
        if (priority != FOCUS) {
            if (active != null && active.context.equals(contextKey)
                    && active.text.equals(text)) {
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
        }
        Request request = new Request(contextKey, key, text, priority, ttlMs, startChunk);
        for (Iterator<Request> iterator = pending.iterator(); iterator.hasNext();) {
            Request old = iterator.next();
            if ((old.key.equals(key) && old.priority <= priority)
                    || (priority == FOCUS && old.priority == FOCUS)
                    || (priority > FOCUS && old.priority == FOCUS)
                    || (priority >= URGENT && old.priority < URGENT)) {
                iterator.remove();
            }
        }
        if (active != null && !isOrderedGameEvent(active)
                && (priority > active.priority
                    || (priority == FOCUS && active.priority == FOCUS))) {
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
        if (!urgentGameEvents.isEmpty() || !orderedGameEvents.isEmpty()) {
            // An urgent health warning outranks queued nonurgent chatter.
            // Already-playing game speech is allowed to finish first.
            active = !urgentGameEvents.isEmpty()
                    ? urgentGameEvents.removeFirst() : orderedGameEvents.removeFirst();
            chunkIndex = startChunkOf(active);
            lastText = active.text;
            lastKey = active.key;
            speakChunk();
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
            chunkIndex = startChunkOf(active);
            lastText = best.text;
            lastKey = best.key;
            speakChunk();
        }
    }

    private static int startChunkOf(Request request) {
        if (request == null || request.chunks.isEmpty()) {
            return 0;
        }
        return Math.max(0, Math.min(request.startChunk, request.chunks.size() - 1));
    }

    private void speakChunk() {
        if (active == null || !(isOrderedGameEvent(active)
                || active.context.equals(contextKey)) || !isReady()) {
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
        // Repeat means "from the start": forget the interruption point so
        // a later resume() does not jump back mid-document (STT 175).
        lastChunkIndex = 0;
        say("repeat:" + key, text, FOCUS);
    }

    /**
     * Continue the last utterance from the chunk where it was cut off
     * (STT 175): a long document (MOTD/Credits) interrupted by a focus
     * move, Stop or an engine error resumes mid-text instead of
     * restarting from chunk 0. Falls back to {@link #repeat()} when
     * nothing was interrupted mid-utterance. Stops whatever is speaking
     * now. Wiring a gesture or command to this is the controller's job
     * (GameReaderController); this class only provides the mechanism.
     */
    public void resume() {
        String text = lastText;
        String key = lastKey;
        int start = lastChunkIndex;
        if (text == null || text.isEmpty()) {
            return;
        }
        if (start <= 0) {
            repeat();
            return;
        }
        start = Math.min(start, Math.max(0, chunks(text).size() - 1));
        stop();
        say("resume:" + key, text, FOCUS, 2000, start);
    }

    /** Whether the active utterance has another chunk in this direction. */
    public boolean canSkipChunk(int delta) {
        return active != null && active.chunks.size() > 1
                && chunkIndex + delta >= 0 && chunkIndex + delta < active.chunks.size();
    }

    /**
     * Jump the active utterance forward (+1) or back (-1) by one chunk
     * (STT 175). Returns false when there is no chunk in that direction.
     * The current chunk is cancelled and the target chunk speaks at once;
     * the stale utterance callback is ignored because activeChunk is
     * cleared before cancelling. Wiring a gesture to this is the
     * controller's job (GameReaderController).
     */
    public boolean skipChunk(int delta) {
        if (!canSkipChunk(delta)) {
            return false;
        }
        chunkIndex += delta;
        removeCompletionTimeout();
        activeChunk = null;
        speech.cancel();
        speakChunk();
        return true;
    }

    public boolean skipToNextChunk() {
        return skipChunk(1);
    }

    public boolean skipToPreviousChunk() {
        return skipChunk(-1);
    }

    /** 0-based index of the chunk being spoken, or -1 when idle. */
    public int getChunkIndex() {
        return active == null ? -1 : chunkIndex;
    }

    /** Number of chunks in the active utterance, or 0 when idle. */
    public int getChunkCount() {
        return active == null ? 0 : active.chunks.size();
    }

    public void stop() {
        clearRequests();
        speech.cancel();
    }

    /**
     * Interrupt audible and queued game speech immediately when the player
     * touches the screen. Return whether an utterance was in progress or
     * queued so a solitary touch can silence without opening a world tile.
     * All access is on the Android UI thread, as with every queue operation.
     */
    public boolean stopForTouch() {
        boolean interrupted = active != null || !pending.isEmpty()
                || !orderedGameEvents.isEmpty() || !urgentGameEvents.isEmpty();
        stop();
        return interrupted;
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
        lastChunkIndex = 0;
        // The duplicate window must not reach across a lifecycle
        // boundary either: what was said before a pause says nothing
        // about what the fresh foreground session needs to hear.
    }

    private void clearRequests() {
        removeCompletionTimeout();
        pending.clear();
        orderedGameEvents.clear();
        urgentGameEvents.clear();
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

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


public final class ReaderSpeechQueue implements AutoCloseable {
    public static final int LONG_TEXT = 0;
    public static final int INFORMATION = 1;
    public static final int FOCUS = 2;



    public static final int GAME_EVENT = FOCUS;
    public static final int URGENT = 3;

    public interface Listener {
        void onHealth(AndroidSpeech.State state);







        default void onVoiceRecovered() { }
    }









    public interface CompletionListener {
        void onFinished(String key);
        void onInterrupted(String key);
    }

    private static final int MAX_PENDING = 8;
    private static final String GAME_EVENT_CONTEXT = "__reader_game_event__";



    private final ArrayDeque<Request> orderedGameEvents = new ArrayDeque<>();
    private final ArrayDeque<Request> urgentGameEvents = new ArrayDeque<>();

    private static boolean isOrderedGameEvent(Request request) {
        return request != null && GAME_EVENT_CONTEXT.equals(request.context);
    }




    private final Handler handler = new Handler(Looper.getMainLooper());
    private final ArrayDeque<Request> pending = new ArrayDeque<>();


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



    private int lastChunkIndex;
    private Runnable completionTimeout;
    private boolean closed;
    private boolean healthy;
    private CompletionListener completionListener;




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

            lastChunkIndex = 0;
        }
        if (completionListener != null && key != null) {
            completionListener.onFinished(key);
        }
    }

    private void fireInterrupted(String key) {



        statInterrupted++;
        if (key != null && key.equals(lastKey)) {


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



        say(key, text, FOCUS);
    }

    public void say(String key, String text, int priority) {
        say(key, text, priority, priority == FOCUS ? 2000 : 15000);
    }

    public void say(String key, String text, int priority, long ttlMs) {
        say(key, text, priority, ttlMs, 0);
    }







    public void say(String key, String text, int priority, long ttlMs, int startChunk) {
        if (!isReady() || text == null || text.trim().isEmpty()) {
            return;
        }
        text = ReaderSpokenText.clean(text);
        if (text.trim().isEmpty()) { return; }
        if (key != null && key.startsWith("game-message:")) {



            Request request = new Request(GAME_EVENT_CONTEXT, key, text,
                    priority >= URGENT ? URGENT : GAME_EVENT,
                    365L * 24 * 60 * 60 * 1000, startChunk);
            if (request.priority >= URGENT) {



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


        lastChunkIndex = 0;
        say("repeat:" + key, text, FOCUS);
    }










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




    static boolean isAutodriveProgressText(String text) {
        return text != null && (text.contains("Driving to a reachable area near the selected place.")
                || text.contains("Đang lái đến một khu vực xe có thể tiếp cận gần địa điểm đã chọn.")
                || text.contains("Autodrive found a new route to an area near the selected place."));
    }

    public void discardStaleAutodriveProgress() {
        if (closed) { return; }
        for (Iterator<Request> it = orderedGameEvents.iterator(); it.hasNext();) {
            if (isAutodriveProgressText(it.next().text)) { it.remove(); }
        }
        for (Iterator<Request> it = urgentGameEvents.iterator(); it.hasNext();) {
            if (isAutodriveProgressText(it.next().text)) { it.remove(); }
        }
        for (Iterator<Request> it = pending.iterator(); it.hasNext();) {
            if (isAutodriveProgressText(it.next().text)) { it.remove(); }
        }
        if (active != null && isAutodriveProgressText(active.text)) {
            Request stopped = active;
            active = null;
            activeChunk = null;
            removeCompletionTimeout();
            speech.cancel();
            fireInterrupted(stopped.key);
        }
        advance();
    }

    public void stop() {
        clearRequests();
        speech.cancel();
    }











    public void interruptForUserAction() {
        if (closed) { return; }
        clearRequests();
        speech.cancel();
        lastText = "";
        lastKey = "";
        lastChunkIndex = 0;
    }















    public void clearForLifecycle() {
        cancelInitRetry();
        clearRequests();
        speech.cancel();
        contextKey = "";
        lastText = "";
        lastKey = "";
        lastChunkIndex = 0;



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

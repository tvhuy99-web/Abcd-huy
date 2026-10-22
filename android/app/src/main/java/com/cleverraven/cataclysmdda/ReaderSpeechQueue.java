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
    }

    private static final int MAX_PENDING = 8;
    private final Handler handler = new Handler(Looper.getMainLooper());
    private final ArrayDeque<Request> pending = new ArrayDeque<>();
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

    private static final class Request {
        final String context;
        final String key;
        final String text;
        final int priority;
        final long expires;
        final List<String> chunks;

        Request(String context, String key, String text, int priority) {
            this.context = context;
            this.key = key;
            this.text = text;
            this.priority = priority;
            expires = SystemClock.elapsedRealtime() + (priority == FOCUS ? 2000 : 15000);
            chunks = chunks(text);
        }
    }

    public ReaderSpeechQueue(Context context, Listener listener) {
        this.listener = listener;
        speech = new AndroidSpeech(context, new AndroidSpeech.Listener() {
            @Override
            public void onState(AndroidSpeech.State state) {
                healthy = state == AndroidSpeech.State.READY;
                if (!healthy) {
                    clearRequests();
                }
                listener.onHealth(state);
            }

            @Override
            public void onUtterance(String id, AndroidSpeech.Event event) {
                if (activeChunk == null || !activeChunk.equals(id) || closed) {
                    return;
                }
                if (event == AndroidSpeech.Event.STARTED) {
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
                        active = null;
                        advance();
                    }
                } else {
                    // Never advance through a backlog after a cancellation/error.
                    active = null;
                    pending.clear();
                }
            }
        });
    }

    public void initialize(ReaderPreferences preferences) {
        stop();
        healthy = false;
        speech.initialize(null, new Locale("vi", "VN"), preferences.getRate(),
                !preferences.permitsNetworkVoice());
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
        if (!isReady() || text == null || text.trim().isEmpty()) {
            return;
        }
        if (active != null && active.context.equals(contextKey) && active.key.equals(key)
                && active.text.equals(text)) {
            return;
        }
        Request request = new Request(contextKey, key, text, priority);
        for (Iterator<Request> iterator = pending.iterator(); iterator.hasNext();) {
            Request old = iterator.next();
            if (old.key.equals(key) || (priority == FOCUS && old.priority == FOCUS)) {
                iterator.remove();
            }
        }
        if (active != null && (priority > active.priority ||
                (priority == FOCUS && active.priority == FOCUS))) {
            active = null;
            activeChunk = null;
            removeCompletionTimeout();
            speech.cancel();
        }
        if (pending.size() == MAX_PENDING) {
            pending.removeFirst();
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
            if (!request.context.equals(contextKey) || request.expires < now) {
                iterator.remove();
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

    private void clearRequests() {
        if (focusDebounce != null) {
            handler.removeCallbacks(focusDebounce);
            focusDebounce = null;
        }
        removeCompletionTimeout();
        pending.clear();
        active = null;
        activeChunk = null;
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
        clearRequests();
        speech.close();
    }
}

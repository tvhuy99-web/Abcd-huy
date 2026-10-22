package com.cleverraven.cataclysmdda;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.speech.tts.TextToSpeech;
import android.speech.tts.UtteranceProgressListener;

import java.util.Locale;

/** Android output driver. Gameplay priorities and context ownership belong to
 *  the caller; this class never queues stale text while the engine initializes.
 *  Public operations and TTS callbacks are serialized onto the main looper.
 *  The owner must close this instance when its lifecycle ends.
 */
public final class AndroidSpeech implements AutoCloseable {
    public enum State { NEW, INITIALIZING, READY, MISSING_LANGUAGE, FAILED, CLOSED }
    public enum Event { STARTED, DONE, CANCELLED, REJECTED, ERROR }

    public interface Listener {
        // Callbacks run on the main looper. A native consumer must enqueue them
        // for its game thread, never mutate gameplay from here.
        void onState(State state);
        void onUtterance(String clientId, Event event);
    }

    private static final long INIT_TIMEOUT_MS = 8000;
    private static final long START_TIMEOUT_MS = 6000;
    private final Context context;
    private final Handler handler = new Handler(Looper.getMainLooper());
    private final SpeechSession session = new SpeechSession();
    private Listener listener;
    private TextToSpeech engine;
    private volatile State state = State.NEW;
    private Runnable initTimeout;
    private Runnable startTimeout;

    public AndroidSpeech(Context context, Listener listener) {
        this.context = context.getApplicationContext();
        this.listener = listener;
    }

    public State getState() {
        return state;
    }

    /** Explicit setup/recovery; no silent fallback to a different language.
     *  A null engineName asks Android for the user's default engine.
     */
    public void initialize(String engineName, Locale language, float rate) {
        handler.post(() -> initializeOnMain(engineName, language, rate));
    }

    private void initializeOnMain(String engineName, Locale language, float rate) {
        if (state == State.CLOSED) {
            return;
        }
        cancelOnMain();
        shutdownEngine();
        final long epoch = session.newEngine();
        setState(State.INITIALIZING);
        if (language == null || !Float.isFinite(rate) || rate < 0.2f || rate > 3.0f) {
            fail(State.FAILED);
            return;
        }
        try {
            TextToSpeech.OnInitListener onInit = status -> handler.post(() -> {
                if (state != State.INITIALIZING || !session.isCurrentEngine(epoch)) {
                    return;
                }
                removeInitTimeout();
                if (status != TextToSpeech.SUCCESS || engine == null) {
                    fail(State.FAILED);
                    return;
                }
                try {
                    final int result = engine.setLanguage(language);
                    if (result == TextToSpeech.LANG_MISSING_DATA || result == TextToSpeech.LANG_NOT_SUPPORTED) {
                        fail(State.MISSING_LANGUAGE);
                        return;
                    }
                    if (result < 0 || engine.setSpeechRate(rate) == TextToSpeech.ERROR
                            || engine.setOnUtteranceProgressListener(progress) == TextToSpeech.ERROR) {
                        fail(State.FAILED);
                        return;
                    }
                    if (session.markReady(epoch)) {
                        setState(State.READY);
                    }
                } catch (RuntimeException exception) {
                    fail(State.FAILED);
                }
            });
            engine = engineName == null
                    ? new TextToSpeech(context, onInit)
                    : new TextToSpeech(context, onInit, engineName);
            initTimeout = () -> {
                if (state == State.INITIALIZING && session.isCurrentEngine(epoch)) {
                    fail(State.FAILED);
                }
            };
            handler.postDelayed(initTimeout, INIT_TIMEOUT_MS);
        } catch (RuntimeException exception) {
            fail(State.FAILED);
        }
    }

    /** One chunk at a time; the caller waits for DONE before its next chunk.
     *  Invalid/not-ready requests are rejected, never held for a future screen.
     */
    public void speak(String clientId, String text) {
        handler.post(() -> {
            if (state == State.CLOSED) {
                return;
            }
            if (state != State.READY || engine == null || text == null || text.isEmpty()
                    || text.length() > TextToSpeech.getMaxSpeechInputLength()) {
                emit(clientId, Event.REJECTED);
                return;
            }
            String token = session.begin(clientId);
            if (token == null) {
                emit(clientId, Event.REJECTED);
                return;
            }
            try {
                if (engine.speak(text, TextToSpeech.QUEUE_FLUSH, null, token) == TextToSpeech.ERROR) {
                    finish(token, Event.ERROR);
                    fail(State.FAILED);
                    return;
                }
                startTimeout = () -> {
                    if (session.isActive(token)) {
                        finish(token, Event.ERROR);
                        fail(State.FAILED);
                    }
                };
                handler.postDelayed(startTimeout, START_TIMEOUT_MS);
            } catch (RuntimeException exception) {
                finish(token, Event.ERROR);
                fail(State.FAILED);
            }
        });
    }

    public void cancel() {
        handler.post(() -> {
            if (state != State.CLOSED && !cancelOnMain()) {
                session.newEngine();
                shutdownEngine();
                setState(State.FAILED);
            }
        });
    }

    private boolean cancelOnMain() {
        removeStartTimeout();
        String client = session.cancel();
        boolean stopped = true;
        if (engine != null) {
            try {
                stopped = engine.stop() != TextToSpeech.ERROR;
            } catch (RuntimeException ignored) {
                stopped = false;
            }
        }
        if (client != null) {
            emit(client, Event.CANCELLED);
        }
        return stopped;
    }

    private final UtteranceProgressListener progress = new UtteranceProgressListener() {
        @Override
        public void onStart(String token) {
            handler.post(() -> {
                String client = session.start(token);
                if (state != State.CLOSED && client != null) {
                    removeStartTimeout();
                    emit(client, Event.STARTED);
                }
            });
        }

        @Override
        public void onDone(String token) {
            handler.post(() -> finish(token, Event.DONE));
        }

        @Override
        @SuppressWarnings("deprecation")
        public void onError(String token) {
            handler.post(() -> utteranceFailed(token));
        }

        @Override
        public void onError(String token, int errorCode) {
            handler.post(() -> utteranceFailed(token));
        }

        @Override
        public void onStop(String token, boolean interrupted) {
            handler.post(() -> finish(token, Event.CANCELLED));
        }
    };

    private void utteranceFailed(String token) {
        if (session.isActive(token)) {
            finish(token, Event.ERROR);
            fail(State.FAILED);
        }
    }

    private void finish(String token, Event event) {
        String client = session.complete(token);
        if (client != null) {
            removeStartTimeout();
            emit(client, event);
        }
    }

    private void fail(State failure) {
        removeInitTimeout();
        cancelOnMain();
        session.newEngine();
        shutdownEngine();
        setState(failure);
    }

    private void shutdownEngine() {
        removeInitTimeout();
        if (engine != null) {
            try {
                engine.shutdown();
            } catch (RuntimeException ignored) {
                // The driver will not use this engine again.
            }
            engine = null;
        }
    }

    private void removeInitTimeout() {
        if (initTimeout != null) {
            handler.removeCallbacks(initTimeout);
            initTimeout = null;
        }
    }

    private void removeStartTimeout() {
        if (startTimeout != null) {
            handler.removeCallbacks(startTimeout);
            startTimeout = null;
        }
    }

    private void setState(State value) {
        state = value;
        if (listener != null) {
            listener.onState(value);
        }
    }

    private void emit(String clientId, Event event) {
        if (listener != null) {
            listener.onUtterance(clientId, event);
        }
    }

    @Override
    public void close() {
        handler.post(() -> {
            if (state != State.CLOSED) {
                cancelOnMain();
                session.newEngine();
                shutdownEngine();
                setState(State.CLOSED);
                listener = null;
            }
        });
    }
}

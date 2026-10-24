package com.cleverraven.cataclysmdda;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.speech.tts.TextToSpeech;
import android.speech.tts.UtteranceProgressListener;
import android.speech.tts.Voice;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.Locale;

/** Android output driver. Gameplay priorities and context ownership belong to
 *  the caller; this class never queues stale text while the engine initializes.
 *  Public operations and TTS callbacks are serialized onto the main looper.
 *  The owner must close this instance when its lifecycle ends.
 */
public final class AndroidSpeech implements AutoCloseable {
    public enum State { NEW, INITIALIZING, READY, MISSING_LANGUAGE, MISSING_VOICE, NETWORK_REQUIRED, FAILED, CLOSED }
    public enum Event { STARTED, DONE, CANCELLED, REJECTED, ERROR }

    public interface Listener {
        // Callbacks run on the main looper. A native consumer must enqueue them
        // for its game thread, never mutate gameplay from here.
        void onState(State state);
        void onUtterance(String clientId, Event event);
    }

    /** One installed TTS engine, as reported by a probe instance (R06.1). */
    public static final class EngineOption {
        public final String name;
        public final String label;

        EngineOption(String name, String label) {
            this.name = name;
            this.label = label;
        }
    }

    /**
     * One voice offered by one engine. Only the stable voice name is kept;
     * the live {@link Voice} object is re-resolved inside the owning
     * AndroidSpeech instance at initialize time, so a selection can be
     * persisted and later reported MISSING_VOICE if the voice was removed.
     */
    public static final class VoiceOption {
        public final String engineName;
        public final String voiceName;
        public final Locale locale;
        public final boolean networkRequired;

        VoiceOption(String engineName, Voice voice) {
            this.engineName = engineName;
            this.voiceName = voice.getName();
            this.locale = voice.getLocale();
            this.networkRequired = voice.isNetworkConnectionRequired();
        }
    }

    /** Result of a one-shot engine/voice probe; lists are never null. */
    public interface ProbeListener {
        void onProbe(List<EngineOption> engines, List<VoiceOption> voices, boolean success);
    }

    private static final long PROBE_TIMEOUT_MS = 8000;

    /**
     * Enumerate installed engines and the voices of one engine (null asks
     * for the system default engine) with a short-lived probe instance.
     * The probe is always shut down before the callback and never speaks,
     * so it cannot disturb the caller's own engine (R06.1/R06.2: exactly
     * one speaking owner at a time; a probe is discovery-only).
     */
    public static void probeVoices(Context context, String engineName, ProbeListener listener) {
        final Handler probeHandler = new Handler(Looper.getMainLooper());
        final Context appContext = context.getApplicationContext();
        probeHandler.post(() -> {
            final TextToSpeech[] holder = new TextToSpeech[1];
            final boolean[] finished = new boolean[1];
            final Runnable timeout = () -> {
                if (finished[0]) {
                    return;
                }
                finished[0] = true;
                shutdownProbe(holder[0]);
                listener.onProbe(Collections.emptyList(), Collections.emptyList(), false);
            };
            probeHandler.postDelayed(timeout, PROBE_TIMEOUT_MS);
            try {
                TextToSpeech.OnInitListener onInit = status -> probeHandler.post(() -> {
                    if (finished[0]) {
                        return;
                    }
                    finished[0] = true;
                    probeHandler.removeCallbacks(timeout);
                    List<EngineOption> engines = new ArrayList<>();
                    List<VoiceOption> voices = new ArrayList<>();
                    boolean success = status == TextToSpeech.SUCCESS && holder[0] != null;
                    if (success) {
                        try {
                            for (TextToSpeech.EngineInfo info : holder[0].getEngines()) {
                                engines.add(new EngineOption(info.name, info.label));
                            }
                            List<Voice> found = new ArrayList<>(holder[0].getVoices());
                            Collections.sort(found,
                                    Comparator.comparing((Voice v) -> v.getLocale().toLanguageTag())
                                            .thenComparing(Voice::getName));
                            String owner = engineName;
                            if (owner == null) {
                                TextToSpeech.EngineInfo current = currentEngine(holder[0], engines);
                                owner = current != null ? current.name : "";
                            }
                            for (Voice voice : found) {
                                voices.add(new VoiceOption(owner, voice));
                            }
                        } catch (RuntimeException exception) {
                            success = false;
                            engines.clear();
                            voices.clear();
                        }
                    }
                    shutdownProbe(holder[0]);
                    listener.onProbe(engines, voices, success);
                });
                holder[0] = engineName == null
                        ? new TextToSpeech(appContext, onInit)
                        : new TextToSpeech(appContext, onInit, engineName);
            } catch (RuntimeException exception) {
                if (!finished[0]) {
                    finished[0] = true;
                    probeHandler.removeCallbacks(timeout);
                    listener.onProbe(Collections.emptyList(), Collections.emptyList(), false);
                }
            }
        });
    }

    private static TextToSpeech.EngineInfo currentEngine(TextToSpeech tts,
                                                         List<EngineOption> ignored) {
        try {
            String current = tts.getDefaultEngine();
            for (TextToSpeech.EngineInfo info : tts.getEngines()) {
                if (info.name.equals(current)) {
                    return info;
                }
            }
        } catch (RuntimeException ignoredException) {
            // Fall through: the probe still reports voices without an owner name.
        }
        return null;
    }

    private static void shutdownProbe(TextToSpeech probe) {
        if (probe != null) {
            try {
                probe.shutdown();
            } catch (RuntimeException ignored) {
                // Probe is discarded either way.
            }
        }
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
        initialize(engineName, language, rate, true);
    }

    /** Online voices require an explicit caller choice, never silent fallback. */
    public void initialize(String engineName, Locale language, float rate, boolean offlineOnly) {
        initialize(engineName, language, rate, offlineOnly, null);
    }

    /**
     * Initialize with one explicitly chosen voice (by stable name, R06.1).
     * A named voice that no longer exists fails MISSING_VOICE instead of
     * silently speaking with a different voice; a named network voice
     * under offlineOnly fails NETWORK_REQUIRED, exactly like the default
     * voice path. Passing null keeps the engine-default voice behavior.
     */
    public void initialize(String engineName, Locale language, float rate, boolean offlineOnly,
                           String voiceName) {
        handler.post(() -> initializeOnMain(engineName, language, rate, offlineOnly, voiceName));
    }

    private void initializeOnMain(String engineName, Locale language, float rate, boolean offlineOnly) {
        initializeOnMain(engineName, language, rate, offlineOnly, null);
    }

    private void initializeOnMain(String engineName, Locale language, float rate, boolean offlineOnly,
                                  String voiceName) {
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
                    Voice voice;
                    if (voiceName != null) {
                        voice = findVoice(voiceName);
                        if (voice == null) {
                            fail(State.MISSING_VOICE);
                            return;
                        }
                        if (!language.getLanguage().equals(voice.getLocale().getLanguage())) {
                            fail(State.MISSING_LANGUAGE);
                            return;
                        }
                        if (offlineOnly && voice.isNetworkConnectionRequired()) {
                            fail(State.NETWORK_REQUIRED);
                            return;
                        }
                        if (engine.setVoice(voice) == TextToSpeech.ERROR) {
                            fail(State.FAILED);
                            return;
                        }
                    } else {
                        voice = engine.getVoice();
                        if (voice == null || !language.getLanguage().equals(voice.getLocale().getLanguage())) {
                            fail(State.MISSING_LANGUAGE);
                            return;
                        }
                        if (offlineOnly && voice.isNetworkConnectionRequired()) {
                            fail(State.NETWORK_REQUIRED);
                            return;
                        }
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

    /** Resolve a persisted voice name against this engine's current voices. */
    private Voice findVoice(String voiceName) {
        try {
            for (Voice candidate : engine.getVoices()) {
                if (voiceName.equals(candidate.getName())) {
                    return candidate;
                }
            }
        } catch (RuntimeException ignored) {
            // Treated as missing: the caller recovers by explicit re-selection.
        }
        return null;
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
        if (engine != null && (client != null || state == State.READY)) {
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

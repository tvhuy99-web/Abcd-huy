package android.speech.tts;

import android.content.Context;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Set;

/** Narrow injectable behavior for driver tests, not an Android implementation. */
public final class TextToSpeech {
    public static final int SUCCESS = 0;
    public static final int ERROR = -1;
    public static final int LANG_MISSING_DATA = -1;
    public static final int LANG_NOT_SUPPORTED = -2;
    public static final int QUEUE_FLUSH = 0;
    public static TextToSpeech latest;

    /** Mirror of the real EngineInfo shape used by the voice probe (R06.1). */
    public static final class EngineInfo {
        public final String name;
        public final String label;

        public EngineInfo(String name, String label) {
            this.name = name;
            this.label = label;
        }
    }

    public interface OnInitListener {
        void onInit(int status);
    }

    private final OnInitListener initialized;
    public UtteranceProgressListener progress;
    public int languageResult = SUCCESS;
    public int speakResult = SUCCESS;
    public int speakCalls;
    public boolean stopThrows;
    public boolean closed;
    public String token;
    public boolean networkVoice;
    private Locale locale;

    public TextToSpeech(Context context, OnInitListener listener) {
        initialized = listener;
        latest = this;
    }

    public TextToSpeech(Context context, OnInitListener listener, String engine) {
        this(context, listener);
    }

    public void finishInit(int status) {
        initialized.onInit(status);
    }

    public int setLanguage(Locale locale) {
        this.locale = locale;
        return languageResult;
    }

    private Voice selectedVoice;

    public Voice getVoice() {
        return selectedVoice != null ? selectedVoice : new Voice(locale, networkVoice);
    }

    public int setVoice(Voice voice) {
        selectedVoice = voice;
        return SUCCESS;
    }

    /** One fake engine with the current fake voice, for probe tests. */
    public List<EngineInfo> getEngines() {
        List<EngineInfo> engines = new ArrayList<>();
        engines.add(new EngineInfo("fake.engine", "Fake engine"));
        return engines;
    }

    public String getDefaultEngine() {
        return "fake.engine";
    }

    public Set<Voice> getVoices() {
        Set<Voice> voices = new HashSet<>();
        voices.add(getVoice());
        return voices;
    }

    public int setSpeechRate(float rate) {
        return SUCCESS;
    }

    public int setOnUtteranceProgressListener(UtteranceProgressListener listener) {
        progress = listener;
        return SUCCESS;
    }

    public static int getMaxSpeechInputLength() {
        return 200;
    }

    public int speak(CharSequence text, int mode, Object params, String id) {
        speakCalls++;
        token = id;
        return speakResult;
    }

    public int stop() {
        if (stopThrows) {
            throw new IllegalStateException("Disconnected engine");
        }
        return SUCCESS;
    }

    public void shutdown() {
        closed = true;
    }
}

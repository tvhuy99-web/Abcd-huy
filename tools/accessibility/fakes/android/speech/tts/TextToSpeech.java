package android.speech.tts;

import android.content.Context;
import java.util.Locale;

/** Narrow injectable behavior for driver tests, not an Android implementation. */
public final class TextToSpeech {
    public static final int SUCCESS = 0;
    public static final int ERROR = -1;
    public static final int LANG_MISSING_DATA = -1;
    public static final int LANG_NOT_SUPPORTED = -2;
    public static final int QUEUE_FLUSH = 0;
    public static TextToSpeech latest;

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

    public Voice getVoice() {
        return new Voice(locale, networkVoice);
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

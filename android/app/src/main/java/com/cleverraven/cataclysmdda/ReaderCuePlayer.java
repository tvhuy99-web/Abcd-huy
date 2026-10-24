package com.cleverraven.cataclysmdda;

import android.content.Context;
import android.media.AudioAttributes;
import android.media.AudioFocusRequest;
import android.media.AudioManager;
import android.media.SoundPool;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import android.os.VibrationEffect;
import android.os.Vibrator;

import java.util.HashMap;
import java.util.Map;

/**
 * Optional sound and vibration cues for reader events (R21.2).
 *
 * A cue is never the only carrier of information: every cue name maps
 * to an event the reader also speaks (an urgent game message, a spoken
 * action outcome, a spoken rejection receipt), so turning cues off —
 * the default — loses nothing, and a cue respects the character's
 * knowledge rights by construction because it is fired only from the
 * same engine points that produce the spoken text. A cue also never
 * covers speech: it asks for transient ducking focus and plays a short
 * packaged tone alongside, never instead of, the utterance.
 *
 * The spoken channel is owned by ReaderSpeechQueue; this class owns
 * only the SoundPool and the Vibrator. Playback failure is reported to
 * the listener once per instance so the owner can say so out loud
 * instead of failing silently.
 */
public final class ReaderCuePlayer {
    public static final String BLOCKED = "blocked";
    public static final String DANGER = "danger";
    public static final String PICKUP = "pickup";
    public static final String ACTIVITY_DONE = "activity_done";
    public static final String REJECTED = "rejected";

    public interface Listener {
        /** A cue could not be played (load failure, no vibrator). */
        void onCueUnavailable();
    }

    private static final class Cue {
        final int rawId;
        final long[] pattern;
        final long cooldownMs;

        Cue(int rawId, long[] pattern, long cooldownMs) {
            this.rawId = rawId;
            this.pattern = pattern;
            this.cooldownMs = cooldownMs;
        }
    }

    private static final Map<String, Cue> CUES = new HashMap<>();
    static {
        CUES.put(BLOCKED, new Cue(R.raw.a11y_cue_blocked, new long[]{0, 60, 60, 60}, 700));
        CUES.put(DANGER, new Cue(R.raw.a11y_cue_danger, new long[]{0, 120, 80, 120}, 2500));
        CUES.put(PICKUP, new Cue(R.raw.a11y_cue_pickup, new long[]{0, 40}, 700));
        CUES.put(ACTIVITY_DONE, new Cue(R.raw.a11y_cue_done, new long[]{0, 50, 60, 50}, 1200));
        CUES.put(REJECTED, new Cue(R.raw.a11y_cue_blocked, new long[]{0, 90}, 700));
    }

    private final Context context;
    private final ReaderPreferences preferences;
    private final Listener listener;
    private final Handler handler = new Handler(Looper.getMainLooper());
    private final Map<String, Long> lastPlayed = new HashMap<>();
    /** sampleId → raw resource id, for loads still in flight. */
    private final Map<Integer, Integer> pendingLoads = new HashMap<>();
    private SoundPool pool;
    private AudioManager audioManager;
    private AudioFocusRequest focusRequest;
    /** One shared instance: the legacy abandon call matches by identity. */
    private final AudioManager.OnAudioFocusChangeListener legacyFocusListener = change -> { };
    private Runnable abandonFocus;
    private boolean loadFailed;
    private boolean unavailableReported;
    private boolean closed;

    public ReaderCuePlayer(Context context, ReaderPreferences preferences, Listener listener) {
        this.context = context.getApplicationContext();
        this.preferences = preferences;
        this.listener = listener;
    }

    /** Play the named cue if the user enabled its channel. Unknown names are ignored. */
    public void play(String name) {
        if (closed) {
            return;
        }
        Cue cue = CUES.get(name);
        if (cue == null) {
            return;
        }
        boolean sound = preferences.areSoundCuesEnabled();
        boolean haptics = preferences.areHapticCuesEnabled();
        if (!sound && !haptics) {
            return;
        }
        long now = SystemClock.elapsedRealtime();
        Long last = lastPlayed.get(name);
        if (last != null && now - last < cue.cooldownMs) {
            return;
        }
        lastPlayed.put(name, now);
        if (sound) {
            playSound(cue);
        }
        if (haptics) {
            playHaptic(cue);
        }
    }

    private void playSound(Cue cue) {
        if (loadFailed) {
            reportUnavailable();
            return;
        }
        if (pool == null) {
            AudioAttributes attributes = new AudioAttributes.Builder()
                    .setUsage(AudioAttributes.USAGE_ASSISTANCE_ACCESSIBILITY)
                    .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
                    .build();
            pool = new SoundPool.Builder().setMaxStreams(2)
                    .setAudioAttributes(attributes).build();
            pool.setOnLoadCompleteListener((soundPool, sampleId, status) -> {
                Integer raw = pendingLoads.remove(sampleId);
                if (status != 0) {
                    loadFailed = true;
                    reportUnavailable();
                } else if (raw != null) {
                    loadedSamples.put(sampleId, raw);
                    startSound(sampleId);
                }
            });
        }
        Integer loaded = findLoaded(cue.rawId);
        if (loaded != null) {
            startSound(loaded);
            return;
        }
        int sampleId = pool.load(context, cue.rawId, 1);
        if (sampleId == 0) {
            loadFailed = true;
            reportUnavailable();
            return;
        }
        pendingLoads.put(sampleId, cue.rawId);
    }

    private Integer findLoaded(int rawId) {
        // SoundPool sample ids are per load() call; remember them by raw id.
        for (Map.Entry<Integer, Integer> entry : loadedSamples.entrySet()) {
            if (entry.getValue() == rawId) {
                return entry.getKey();
            }
        }
        return null;
    }

    /** sampleId → raw resource id, for samples that finished loading. */
    private final Map<Integer, Integer> loadedSamples = new HashMap<>();

    private void startSound(int sampleId) {
        if (pool == null || closed) {
            return;
        }
        if (!requestCueFocus()) {
            // Another app holds the audio (for example a call): skip the
            // tone rather than talk over it. The spoken equivalent and
            // the haptic pattern are unaffected.
            ReaderDiagnostics.get().count("cue_focus_denied");
            return;
        }
        int stream = pool.play(sampleId, 0.9f, 0.9f, 1, 0, 1.0f);
        if (stream == 0) {
            reportUnavailable();
        }
        scheduleFocusAbandon();
    }

    private boolean requestCueFocus() {
        if (audioManager == null) {
            audioManager = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
        }
        if (audioManager == null) {
            return false;
        }
        int result;
        if (Build.VERSION.SDK_INT >= 26) {
            if (focusRequest == null) {
                AudioAttributes attributes = new AudioAttributes.Builder()
                        .setUsage(AudioAttributes.USAGE_ASSISTANCE_ACCESSIBILITY)
                        .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
                        .build();
                focusRequest = new AudioFocusRequest.Builder(
                        AudioManager.AUDIOFOCUS_GAIN_TRANSIENT_MAY_DUCK)
                        .setAudioAttributes(attributes)
                        .setOnAudioFocusChangeListener(change -> { }, handler)
                        .build();
            }
            result = audioManager.requestAudioFocus(focusRequest);
        } else {
            result = audioManager.requestAudioFocus(legacyFocusListener,
                    AudioManager.STREAM_MUSIC, AudioManager.AUDIOFOCUS_GAIN_TRANSIENT_MAY_DUCK);
        }
        return result == AudioManager.AUDIOFOCUS_REQUEST_GRANTED;
    }

    private void scheduleFocusAbandon() {
        if (abandonFocus != null) {
            handler.removeCallbacks(abandonFocus);
        }
        abandonFocus = () -> {
            abandonFocus = null;
            abandonCueFocus();
        };
        handler.postDelayed(abandonFocus, 900);
    }

    private void abandonCueFocus() {
        if (audioManager == null) {
            return;
        }
        if (Build.VERSION.SDK_INT >= 26) {
            if (focusRequest != null) {
                audioManager.abandonAudioFocusRequest(focusRequest);
            }
        } else {
            audioManager.abandonAudioFocus(legacyFocusListener);
        }
    }

    private void playHaptic(Cue cue) {
        Vibrator vibrator = (Vibrator) context.getSystemService(Context.VIBRATOR_SERVICE);
        if (vibrator == null || !vibrator.hasVibrator()) {
            reportUnavailable();
            return;
        }
        try {
            if (Build.VERSION.SDK_INT >= 26) {
                vibrator.vibrate(VibrationEffect.createWaveform(cue.pattern, -1));
            } else {
                vibrator.vibrate(cue.pattern, -1);
            }
        } catch (RuntimeException exception) {
            reportUnavailable();
        }
    }

    private void reportUnavailable() {
        ReaderDiagnostics.get().count("cue_unavailable");
        if (!unavailableReported && listener != null) {
            unavailableReported = true;
            listener.onCueUnavailable();
        }
    }

    public void close() {
        closed = true;
        if (abandonFocus != null) {
            handler.removeCallbacks(abandonFocus);
            abandonFocus = null;
        }
        abandonCueFocus();
        if (pool != null) {
            pool.release();
            pool = null;
        }
        pendingLoads.clear();
        loadedSamples.clear();
    }
}

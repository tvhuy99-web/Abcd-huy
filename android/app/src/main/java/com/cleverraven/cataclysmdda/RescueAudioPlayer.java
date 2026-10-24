package com.cleverraven.cataclysmdda;

import android.content.Context;
import android.media.MediaPlayer;

/**
 * Packaged Vietnamese rescue audio (R06.3). These clips are pre-recorded
 * resources, so they play even when every TTS engine on the device is
 * missing, broken or still initializing — the states in which the
 * self-voicing bootstrap would otherwise go completely silent.
 *
 * One player per owning screen; the owner must call {@link #release()}
 * when its lifecycle ends. Playing a new clip replaces the current one.
 * This class never touches TextToSpeech and never opens system screens:
 * the rescue screens pair the clips with ordinary labeled buttons (retry,
 * voice settings, reader setup, back/exit) whose order the clips describe.
 */
public final class RescueAudioPlayer {
    public static final int CLIP_TTS_FAILED = R.raw.a11y_rescue_tts_failed;
    public static final int CLIP_RETRYING = R.raw.a11y_rescue_retrying;
    public static final int CLIP_INSTALL_FAILED = R.raw.a11y_rescue_install_failed;
    public static final int CLIP_NO_SPACE = R.raw.a11y_rescue_no_space;
    public static final int CLIP_WELCOME = R.raw.a11y_rescue_welcome;
    public static final int CLIP_SETUP_FAILED = R.raw.a11y_rescue_setup_failed;

    private final Context context;
    private MediaPlayer player;
    private int playingClip;

    public RescueAudioPlayer(Context context) {
        this.context = context.getApplicationContext();
    }

    /** True while the given clip is the one currently playing. */
    public synchronized boolean isPlaying(int clipResId) {
        return player != null && playingClip == clipResId;
    }

    public synchronized boolean isPlaying() {
        return player != null;
    }

    /**
     * Start a clip, replacing any clip in flight. Returns false when the
     * resource could not be opened; the caller keeps its visual/TalkBack
     * path either way, so a media failure never traps the user.
     */
    public synchronized boolean play(int clipResId) {
        stopLocked();
        MediaPlayer created = null;
        try {
            created = MediaPlayer.create(context, clipResId);
            if (created == null) {
                return false;
            }
            final MediaPlayer owned = created;
            created.setOnCompletionListener(mp -> {
                synchronized (RescueAudioPlayer.this) {
                    if (player == owned) {
                        player = null;
                        playingClip = 0;
                    }
                }
                releaseQuietly(owned);
            });
            created.setOnErrorListener((mp, what, extra) -> {
                synchronized (RescueAudioPlayer.this) {
                    if (player == owned) {
                        player = null;
                        playingClip = 0;
                    }
                }
                releaseQuietly(owned);
                return true;
            });
            player = created;
            playingClip = clipResId;
            created.start();
            return true;
        } catch (RuntimeException exception) {
            if (created != null) {
                releaseQuietly(created);
                if (player == created) {
                    player = null;
                    playingClip = 0;
                }
            }
            return false;
        }
    }

    public synchronized void stop() {
        stopLocked();
    }

    /** Stop playback and free the player; the owner is gone after this. */
    public synchronized void release() {
        stopLocked();
    }

    private void stopLocked() {
        if (player != null) {
            MediaPlayer old = player;
            player = null;
            playingClip = 0;
            try {
                old.stop();
            } catch (RuntimeException ignored) {
                // Already stopped or never started; release below still runs.
            }
            releaseQuietly(old);
        }
    }

    private static void releaseQuietly(MediaPlayer target) {
        try {
            target.release();
        } catch (RuntimeException ignored) {
            // Nothing else can be done with a broken player instance.
        }
    }
}

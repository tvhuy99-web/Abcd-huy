package com.cleverraven.cataclysmdda;

import android.content.Context;
import android.media.AudioAttributes;
import android.media.SoundPool;
import android.os.Handler;
import android.os.Looper;

/**
 * One gentle sonar ping per second when a seen item, monster or NPC is
 * cardinally aligned with the player. This never interrupts screen-reader
 * speech or changes the native game's audio settings.
 *
 * All methods run on the Android main looper. No polling of game data happens
 * here: the native game thread publishes only a visibility-filtered distance.
 */
final class AlignmentSonarPlayer implements AutoCloseable {
    private static final long PING_INTERVAL_MS = 1000L;
    private static final int MAX_DISTANCE = 30;

    private final Handler handler = new Handler(Looper.getMainLooper());
    private final SoundPool pool;
    // SoundPool loads asynchronously. Its listener is installed before load().
    private int soundId;
    private int playingStream;
    private final Runnable tick = new Runnable() {
        @Override
        public void run() {
            scheduled = false;
            if (!closed && worldActive && nearestDistance > 0) {
                if (loaded) {
                    final float gain = volumeAt(nearestDistance);
                    playingStream = pool.play(soundId, gain, gain, 1, 0, 1.0f);
                }
                scheduled = true;
                handler.postDelayed(this, PING_INTERVAL_MS);
            }
        }
    };
    private boolean loaded;
    private boolean closed;
    private boolean scheduled;
    private boolean worldActive;
    private int nearestDistance = -1;

    AlignmentSonarPlayer(Context context) {
        AudioAttributes attributes = new AudioAttributes.Builder()
                .setUsage(AudioAttributes.USAGE_ASSISTANCE_ACCESSIBILITY)
                .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
                .build();
        pool = new SoundPool.Builder().setMaxStreams(1)
                .setAudioAttributes(attributes).build();
        pool.setOnLoadCompleteListener((soundPool, sampleId, status) -> {
            if (!closed && sampleId == soundId && status == 0) {
                loaded = true;
            }
        });
        soundId = pool.load(context.getApplicationContext(),
                R.raw.a11y_alignment_sonar, 1);
    }

    /** -1 means nothing aligned; otherwise nearest distance in [1, 30]. */
    void updateDistance(int distance) {
        if (closed) {
            return;
        }
        nearestDistance = distance >= 1 && distance <= MAX_DISTANCE ? distance : -1;
        refresh();
    }

    /** A menu, paused/hidden Activity or disabled reader stops the sonar. */
    void setWorldActive(boolean active) {
        if (closed) {
            return;
        }
        worldActive = active;
        if (!active) {
            nearestDistance = -1;
        }
        refresh();
    }

    private void refresh() {
        if (!worldActive || nearestDistance < 1) {
            handler.removeCallbacks(tick);
            scheduled = false;
            if (playingStream != 0) {
                pool.stop(playingStream);
                playingStream = 0;
            }
        } else if (!scheduled) {
            scheduled = true;
            handler.post(tick);
        }
    }

    /** 30 tiles = soft; 1 tile = loud; never louder than 90%. */
    static float volumeAt(int distance) {
        final int safe = Math.max(1, Math.min(MAX_DISTANCE, distance));
        return 0.08f + (MAX_DISTANCE - safe) * (0.82f / (MAX_DISTANCE - 1));
    }

    @Override
    public void close() {
        closed = true;
        handler.removeCallbacks(tick);
        nearestDistance = -1;
        worldActive = false;
        if (playingStream != 0) {
            pool.stop(playingStream);
            playingStream = 0;
        }
        pool.release();
    }
}

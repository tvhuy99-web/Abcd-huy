package com.cleverraven.cataclysmdda;

import android.content.Context;
import android.media.AudioAttributes;
import android.media.SoundPool;
import android.os.Handler;
import android.os.Looper;
import java.util.Arrays;

/**
 * Optional, independent item and creature sonar across eight compass rays.
 * Each direction gets a separate time slot; stereo pan and north/south pitch
 * distinguish the headings without interrupting TTS or changing game audio.
 */
final class AlignmentSonarPlayer implements AutoCloseable {
    static final int MAX_DISTANCE = 16;
    static final int DIRECTIONS = 8;
    private static final long DIRECTION_SLOT_MS = 160L;
    private final Handler handler = new Handler(Looper.getMainLooper());
    private final SoundPool pool;
    private final int[] creatures = new int[DIRECTIONS];
    private final int[] items = new int[DIRECTIONS];
    private final int[] streams = new int[DIRECTIONS * 2];
    private int creatureSound;
    private int itemSound;
    private boolean creatureLoaded;
    private boolean itemLoaded;
    private boolean closed;
    private boolean enabled;
    private boolean worldActive;
    private boolean scheduled;
    private int direction;

    private final Runnable tick = new Runnable() {
        @Override public void run() {
            scheduled = false;
            if (closed || !enabled || !worldActive || !hasTargets()) {
                return;
            }
            final int heading = direction;
            direction = (direction + 1) % DIRECTIONS;
            // Both categories play independently, including on one ray.
            if (creatureLoaded && creatures[heading] > 0) {
                play(creatureSound, creatures[heading], heading, 0);
            }
            if (itemLoaded && items[heading] > 0) {
                play(itemSound, items[heading], heading, 1);
            }
            scheduled = true;
            handler.postDelayed(this, DIRECTION_SLOT_MS);
        }
    };

    AlignmentSonarPlayer(Context context) {
        Arrays.fill(creatures, -1);
        Arrays.fill(items, -1);
        AudioAttributes attributes = new AudioAttributes.Builder()
                .setUsage(AudioAttributes.USAGE_ASSISTANCE_ACCESSIBILITY)
                .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION).build();
        pool = new SoundPool.Builder().setMaxStreams(DIRECTIONS * 2)
                .setAudioAttributes(attributes).build();
        pool.setOnLoadCompleteListener((soundPool, sampleId, status) -> {
            if (closed || status != 0) { return; }
            if (sampleId == creatureSound) { creatureLoaded = true; }
            if (sampleId == itemSound) { itemLoaded = true; }
        });
        Context app = context.getApplicationContext();
        creatureSound = pool.load(app, R.raw.a11y_alignment_sonar, 1);
        itemSound = pool.load(app, R.raw.a11y_item_beep, 1);
    }

    void setEnabled(boolean on) {
        if (closed || enabled == on) { return; }
        enabled = on;
        if (!on) { clearTargets(); }
        refresh();
    }

    /** Menus, paused/hidden Activities and disabled readers stop the sonar. */
    void setWorldActive(boolean active) {
        if (closed || worldActive == active) { return; }
        worldActive = active;
        if (!active) { clearTargets(); }
        refresh();
    }

    /** Nearest visible distance per type per compass ray, as reported by C++. */
    void updateTargets(int[] nearbyCreatures, int[] nearbyItems) {
        if (closed || !enabled || nearbyCreatures == null || nearbyItems == null
                || nearbyCreatures.length != DIRECTIONS || nearbyItems.length != DIRECTIONS) {
            return;
        }
        for (int i = 0; i < DIRECTIONS; ++i) {
            creatures[i] = clamp(nearbyCreatures[i]);
            items[i] = clamp(nearbyItems[i]);
        }
        refresh();
    }

    private static int clamp(int d) {
        return d >= 1 && d <= MAX_DISTANCE ? d : -1;
    }

    private boolean hasTargets() {
        for (int i = 0; i < DIRECTIONS; ++i) {
            if (creatures[i] > 0 || items[i] > 0) { return true; }
        }
        return false;
    }

    private void clearTargets() {
        Arrays.fill(creatures, -1);
        Arrays.fill(items, -1);
    }

    private void play(int sample, int distance, int heading, int category) {
        final float gain = volumeAt(distance);
        // N is bright/center, S low/center, East and West pan.
        // Diagonal headings blend pan with pitch.
        float left = gain;
        float right = gain;
        switch (heading) {
            case 1: case 3: left *= 0.30f; break;
            case 2: left *= 0.10f; break;
            case 5: case 7: right *= 0.30f; break;
            case 6: right *= 0.10f; break;
            default: break;
        }
        final float rate = heading == 0 ? 1.35f
                : heading == 1 || heading == 7 ? 1.20f
                : heading == 4 ? 0.70f
                : heading == 3 || heading == 5 ? 0.82f : 1.0f;
        streams[heading * 2 + category] = pool.play(sample, left, right, 1, 0, rate);
    }

    /** 16 tiles = soft; 1 tile = loud (maximum 90% sample gain). */
    static float volumeAt(int distance) {
        final int d = Math.max(1, Math.min(MAX_DISTANCE, distance));
        return 0.10f + (MAX_DISTANCE - d) * (0.80f / (MAX_DISTANCE - 1));
    }

    private void refresh() {
        if (!enabled || !worldActive || !hasTargets()) {
            handler.removeCallbacks(tick);
            scheduled = false;
            for (int i = 0; i < streams.length; ++i) {
                if (streams[i] != 0) { pool.stop(streams[i]); streams[i] = 0; }
            }
        } else if (!scheduled) {
            direction = 0;
            scheduled = true;
            handler.post(tick);
        }
    }

    @Override public void close() {
        if (closed) { return; }
        closed = true;
        enabled = false;
        worldActive = false;
        clearTargets();
        handler.removeCallbacks(tick);
        scheduled = false;
        for (int i = 0; i < streams.length; ++i) {
            if (streams[i] != 0) { pool.stop(streams[i]); streams[i] = 0; }
        }
        pool.release();
    }
}

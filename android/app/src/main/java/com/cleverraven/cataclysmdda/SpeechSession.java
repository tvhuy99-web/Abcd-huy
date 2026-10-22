package com.cleverraven.cataclysmdda;

/** Serial-thread token gate shared by the Android driver and host-side tests.
 *  No callbacks from a replaced engine or cancelled utterance may complete a
 *  newer request, including one with the same caller-provided label.
 */
final class SpeechSession {
    private long generation;
    private long sequence;
    private boolean ready;
    private String activeToken;
    private String activeClient;
    private boolean started;

    long newEngine() {
        generation++;
        ready = false;
        cancel();
        return generation;
    }

    boolean isCurrentEngine(long epoch) {
        return epoch == generation;
    }

    boolean markReady(long epoch) {
        if (!isCurrentEngine(epoch)) {
            return false;
        }
        ready = true;
        return true;
    }

    String begin(String clientId) {
        if (!ready || clientId == null || clientId.isEmpty() || activeToken != null) {
            return null;
        }
        activeClient = clientId;
        started = false;
        activeToken = generation + ":" + (++sequence);
        return activeToken;
    }

    boolean isActive(String token) {
        return token != null && token.equals(activeToken);
    }

    String start(String token) {
        if (!isActive(token) || started) {
            return null;
        }
        started = true;
        return activeClient;
    }

    String complete(String token) {
        return isActive(token) ? cancel() : null;
    }

    String cancel() {
        String client = activeClient;
        activeClient = null;
        activeToken = null;
        started = false;
        return client;
    }
}

package com.cleverraven.cataclysmdda;

/** JVM-only diagnostic sink: never included in the Android application. */
public final class SessionDebugLog {
    private SessionDebugLog() {}
    public static void event(String category, String detail) {
        // Speech correctness is tested independently of Android file storage.
    }
}

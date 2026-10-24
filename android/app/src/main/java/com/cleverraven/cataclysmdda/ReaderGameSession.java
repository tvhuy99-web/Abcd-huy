package com.cleverraven.cataclysmdda;

/** Process-wide exclusion between launching the engine and replacing player data. */
final class ReaderGameSession {
    private static boolean gameStarted;
    private static boolean restoring;

    private ReaderGameSession() { }

    static synchronized boolean reserveGameStart() {
        if (restoring) { return false; }
        gameStarted = true;
        return true;
    }

    static synchronized boolean canRestore() { return !gameStarted && !restoring; }

    static synchronized boolean beginRestore() {
        if (!canRestore()) { return false; }
        restoring = true;
        return true;
    }

    static synchronized void endRestore() { restoring = false; }
    // gameStarted intentionally survives Activity destruction: SDL may still
    // hold a world in memory. A fresh process is required before restoring.
}

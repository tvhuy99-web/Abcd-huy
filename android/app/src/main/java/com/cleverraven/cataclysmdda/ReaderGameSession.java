package com.cleverraven.cataclysmdda;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;


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























    static final String BACKUP_STAGING_DIR = ".reader_backup_staging";
    private static final String DEFERRED_RESTORE_MARKER = "deferred_restore.txt";

    private static File deferredRestoreMarker(File liveRoot) {
        return new File(new File(liveRoot, BACKUP_STAGING_DIR), DEFERRED_RESTORE_MARKER);
    }


    static synchronized boolean hasDeferredRestore(File liveRoot) {
        return liveRoot != null && deferredRestoreMarker(liveRoot).isFile();
    }







    static synchronized void markDeferredRestore(File liveRoot) throws IOException {
        if (liveRoot == null) {
            throw new IOException("no live root for deferred restore");
        }
        File staging = new File(liveRoot, BACKUP_STAGING_DIR);
        if (!staging.isDirectory()) {
            throw new IOException("no staged import to confirm");
        }
        File marker = deferredRestoreMarker(liveRoot);
        try (FileOutputStream out = new FileOutputStream(marker)) {
            out.write("deferred\n".getBytes("UTF-8"));
            out.getFD().sync();
        }
    }
}

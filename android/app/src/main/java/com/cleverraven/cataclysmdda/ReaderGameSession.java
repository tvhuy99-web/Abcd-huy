package com.cleverraven.cataclysmdda;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

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

    // ---- Deferred restore ----
    //
    // The only way into BackupRestoreActivity is in-game (Main Menu ->
    // Settings), where the engine has already started and canRestore() is
    // false, so a restore could never run. Instead of restoring, the
    // Confirm action records itself here: a marker file inside the
    // staging directory. The marker is the durable form of the user's
    // explicit confirmation, and it is the only thing that ever makes a
    // staged import apply without a fresh confirmation.
    //
    // The marker lives inside the staging directory on purpose, so its
    // lifetime is exactly the staged import's lifetime, enforced by
    // ReaderBackupArchive itself with no extra cleanup paths:
    // stageImport() deletes staging (a new stage voids an old
    // confirmation), cancelStagedImport() deletes it (Cancel voids it),
    // and restoreStagedImport() deletes it on success (the confirmation
    // is spent). A merely staged, unconfirmed import has no marker and
    // is never applied. STAGING_DIR below must match
    // ReaderBackupArchive's private staging directory name; if that name
    // ever changes, update this one with it.
    static final String BACKUP_STAGING_DIR = ".reader_backup_staging";
    private static final String DEFERRED_RESTORE_MARKER = "deferred_restore.txt";

    private static File deferredRestoreMarker(File liveRoot) {
        return new File(new File(liveRoot, BACKUP_STAGING_DIR), DEFERRED_RESTORE_MARKER);
    }

    /** True while an explicitly confirmed restore is waiting for the next launch. */
    static synchronized boolean hasDeferredRestore(File liveRoot) {
        return liveRoot != null && deferredRestoreMarker(liveRoot).isFile();
    }

    /**
     * Record an explicit Confirm as a deferred restore. The staging
     * directory must already exist (the import was staged); the marker is
     * fsynced so it survives the user closing the app straight after
     * confirming. Nothing live is touched.
     */
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

package com.cleverraven.cataclysmdda;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.attribute.PosixFilePermissions;
import java.util.Arrays;

/** Filesystem fault tests against production transaction/archive classes. */
public final class ReaderStorageTest {
    private static int assertions;
    private static void check(boolean value, String message) {
        assertions++;
        if (!value) { throw new AssertionError(message); }
    }
    private static void put(Path path, String value) throws IOException {
        Files.createDirectories(path.getParent());
        Files.writeString(path, value);
    }
    private static ReaderFileTransaction tx(Path base) {
        return new ReaderFileTransaction(base.resolve("live").toFile(),
                base.resolve("rollback").toFile(), base.resolve("journal").toFile(),
                "BACKUP_", new String[] {"save", "config"});
    }

    public static void main(String[] args) throws Exception {
        Path base = Files.createTempDirectory("reader-storage-");
        try {
            Path happy = base.resolve("happy");
            put(happy.resolve("live/save/old"), "original");
            put(happy.resolve("stage/save/new"), "new");
            tx(happy).replace(happy.resolve("stage").toFile(), Arrays.asList("save"), null);
            check(Files.readString(happy.resolve("live/save/new")).equals("new"), "commit new data");
            check(!Files.exists(happy.resolve("journal")), "commit clears journal");
            check(!Files.exists(happy.resolve("rollback")), "commit clears rollback");

            Path failed = base.resolve("failed");
            put(failed.resolve("live/save/old"), "original");
            put(failed.resolve("stage/save/new"), "new");
            put(failed.resolve("stage/config/new"), "new config");
            try {
                tx(failed).replace(failed.resolve("stage").toFile(), Arrays.asList("save", "config"),
                        () -> { throw new IOException("injected before commit"); });
                throw new AssertionError("failure must propagate");
            } catch (IOException expected) {
                check(Files.readString(failed.resolve("live/save/old")).equals("original"), "restored original");
                check(!Files.exists(failed.resolve("live/config")), "remove root absent before swap");
            }

            Path blocked = base.resolve("blocked");
            put(blocked.resolve("rollback/save/old"), "original");
            put(blocked.resolve("live/save/partial"), "partial");
            put(blocked.resolve("journal"), "BACKUP_SWAPPING\nroots=save\noriginals=save\n");
            Files.setPosixFilePermissions(blocked.resolve("live"), PosixFilePermissions.fromString("r-xr-xr-x"));
            boolean permissionEnforced = !Files.isWritable(blocked.resolve("live"));
            if (permissionEnforced) {
                try { tx(blocked).recover(); throw new AssertionError("recovery must stop"); }
                catch (IOException expected) {
                    check(Files.exists(blocked.resolve("rollback/save/old")), "failed rollback retains original");
                    check(Files.exists(blocked.resolve("journal")), "failed rollback retains journal");
                }
            } else {
                System.out.println("SKIP read-only-directory fault: process bypasses filesystem permissions");
            }
            Files.setPosixFilePermissions(blocked.resolve("live"), PosixFilePermissions.fromString("rwxr-xr-x"));
            tx(blocked).recover();
            check(Files.readString(blocked.resolve("live/save/old")).equals("original"), "retry restores original");
            tx(blocked).recover();
            check(!Files.exists(blocked.resolve("journal")), "recovery is idempotent");

            Path unknown = base.resolve("unknown");
            put(unknown.resolve("rollback/save/old"), "original");
            put(unknown.resolve("journal"), "UNKNOWN\n");
            try { tx(unknown).recover(); throw new AssertionError("unknown journal must stop"); }
            catch (IOException expected) {
                check(Files.exists(unknown.resolve("rollback/save/old")), "unknown state retains original");
            }

            Path noJournal = base.resolve("no-journal");
            put(noJournal.resolve("rollback/save/old"), "original");
            try { tx(noJournal).recover(); throw new AssertionError("untracked rollback must stop"); }
            catch (IOException expected) {
                check(Files.exists(noJournal.resolve("rollback/save/old")), "missing journal retains original");
            }

            Path cannotJournal = base.resolve("cannot-journal");
            put(cannotJournal.resolve("live/save/old"), "original");
            put(cannotJournal.resolve("stage/save/new"), "new");
            Files.createDirectories(cannotJournal.resolve("journal"));
            try {
                tx(cannotJournal).replace(cannotJournal.resolve("stage").toFile(), Arrays.asList("save"), null);
                throw new AssertionError("invalid journal must prevent swap");
            } catch (IOException expected) {
                check(Files.readString(cannotJournal.resolve("live/save/old")).equals("original"), "journal failure leaves original");
            }

            Path archiveBase = base.resolve("archive");
            put(archiveBase.resolve("save/character.sav"), "save one");
            ReaderBackupArchive archive = new ReaderBackupArchive(archiveBase.toFile(), base.resolve("archive-journal").toFile());
            ByteArrayOutputStream bytes = new ByteArrayOutputStream();
            archive.exportTo(bytes);
            put(archiveBase.resolve("save/character.sav"), "save two");
            archive.stageImport(new ByteArrayInputStream(bytes.toByteArray()));
            archive.restoreStagedImport();
            check(Files.readString(archiveBase.resolve("save/character.sav")).equals("save one"), "archive round trip");

            check(ReaderGameSession.beginRestore(), "restore lease available before engine");
            check(!ReaderGameSession.reserveGameStart(), "restore excludes game launch");
            ReaderGameSession.endRestore();
            check(ReaderGameSession.reserveGameStart(), "game starts after restore");
            check(!ReaderGameSession.canRestore(), "engine lifetime excludes restore");
            try { archive.restoreStagedImport(); throw new AssertionError("engine must block restore"); }
            catch (ReaderBackupArchive.BackupException expected) {
                check(ReaderBackupArchive.ERR_GAME_ACTIVE.equals(expected.code), "storage layer enforces session gate");
            }
            System.out.println("Reader storage: " + assertions + " assertions passed");
        } finally {
            ReaderFileTransaction.deleteChecked(base.toFile());
        }
    }
}

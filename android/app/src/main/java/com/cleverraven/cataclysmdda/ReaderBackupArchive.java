package com.cleverraven.cataclysmdda;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;
import java.util.zip.ZipOutputStream;

/**
 * Accessible backup of the player's own data trees (R20.2), kept separate
 * from the game-data installer in SplashScreen.
 *
 * The game keeps every player tree under one live root (the Android
 * external files directory, which C++ uses as both base path and user
 * directory): {@code save/} and {@code config/} are the core of a backup,
 * and {@code memorial/}, {@code achievements/} and {@code templates/} are
 * player data of the same kind, so they travel with it. Shipped trees
 * ({@code data/}, {@code gfx/}, {@code lang/}) are never part of a backup
 * and an import can never replace them: entry names outside the backup
 * trees are rejected during verification.
 *
 * Export only reads the live trees. Import is staged and journaled with
 * the same discipline as the R06 install swap: the chosen archive is
 * copied into a staging directory on the same filesystem, fully verified
 * (readable zip, backup manifest, exact file/byte counts, at least one
 * save or config file, no path escaping the staging tree, enough free
 * space) before anything live is touched, and the swap itself parks the
 * current trees in a rollback directory and renames the staged trees in.
 * A journal in internal storage records the swap phase, so a process
 * death mid-swap is recovered by {@link #recoverInterruptedSwap()}
 * instead of leaving half-restored data that looks valid. A staged but
 * unconfirmed import is never restored automatically: it lives only in
 * the staging directory and the owning screen discards it when a fresh
 * screen instance starts, so no restore confirmation can survive an
 * Activity recreate or a process death (R20.3).
 *
 * This class has no Android dependencies: the owner passes the live root
 * and the journal file, which keeps the archive logic host-testable.
 */
public final class ReaderBackupArchive {
    public static final String ERR_NO_SPACE = "no_space";
    public static final String ERR_IO = "io";
    public static final String ERR_VERIFY = "verify";
    public static final String ERR_INVALID = "invalid";
    public static final String ERR_EMPTY = "empty";
    public static final String ERR_SWAP = "swap";

    private static final String MANIFEST_NAME = "cdda-reader-backup.txt";
    private static final String MANIFEST_MARKER = "cdda-reader-backup";
    private static final String STAGING_DIR = ".reader_backup_staging";
    private static final String ROLLBACK_DIR = ".reader_backup_rollback";
    private static final String STAGED_ROOT = "extracted";
    private static final String STAGED_ARCHIVE = "backup.zip";
    private static final String STAGED_SUMMARY = "staging_summary.txt";
    private static final String JOURNAL_SWAPPING = "BACKUP_SWAPPING";
    private static final String JOURNAL_COMMITTED = "BACKUP_COMMITTED";
    private static final long RESTORE_RESERVE_BYTES = 32L * 1024L * 1024L;
    private static final String[] BACKUP_TREES = {
            "save", "config", "memorial", "achievements", "templates" };
    private static final String[] CORE_TREES = { "save", "config" };

    /** A classified backup failure; the code is spoken by the owner. */
    public static final class BackupException extends Exception {
        public final String code;

        BackupException(String code, String detail) {
            super(detail);
            this.code = code;
        }
    }

    /** Counts and trees of one measured, exported or staged backup. */
    public static final class Summary {
        public final int files;
        public final long bytes;
        public final List<String> trees;

        Summary(int files, long bytes, List<String> trees) {
            this.files = files;
            this.bytes = bytes;
            this.trees = trees;
        }
    }

    private final File liveRoot;
    private final File journalFile;
    private final byte[] buffer = new byte[256 * 1024];

    public ReaderBackupArchive(File liveRoot, File journalFile) {
        this.liveRoot = liveRoot;
        this.journalFile = journalFile;
    }

    private File stagingDir() {
        return new File(liveRoot, STAGING_DIR);
    }

    private File rollbackDir() {
        return new File(liveRoot, ROLLBACK_DIR);
    }

    private File stagedRoot() {
        return new File(stagingDir(), STAGED_ROOT);
    }

    // ------------------------------------------------------------------
    // Export
    // ------------------------------------------------------------------

    /**
     * Write the live backup trees to {@code out} as one zip archive with a
     * manifest entry carrying exact counts. Read-only with respect to the
     * live trees: a failure (including a file changing size while it is
     * copied, which fails verification) leaves the player's data exactly
     * as it was.
     */
    public Summary exportTo(OutputStream out) throws BackupException {
        recoverInterruptedSwap();
        List<String> relativeFiles = new ArrayList<>();
        List<String> trees = new ArrayList<>();
        long totalBytes = 0;
        int coreFiles = 0;
        for (String tree : BACKUP_TREES) {
            File root = new File(liveRoot, tree);
            if (!root.isDirectory()) {
                continue;
            }
            List<String> treeFiles = new ArrayList<>();
            collectFiles(liveRoot, root, treeFiles);
            if (!treeFiles.isEmpty()) {
                trees.add(tree);
                relativeFiles.addAll(treeFiles);
                for (String relative : treeFiles) {
                    totalBytes += new File(liveRoot, relative).length();
                    if (isCoreTree(tree)) {
                        coreFiles++;
                    }
                }
            }
        }
        if (coreFiles == 0) {
            throw new BackupException(ERR_EMPTY,
                    "no save or config files under " + liveRoot);
        }
        Collections.sort(relativeFiles);
        int writtenFiles = 0;
        long writtenBytes = 0;
        try (ZipOutputStream zip = new ZipOutputStream(out)) {
            StringBuilder manifest = new StringBuilder();
            manifest.append(MANIFEST_MARKER).append('\n');
            manifest.append("format=1\n");
            manifest.append("files=").append(relativeFiles.size()).append('\n');
            manifest.append("bytes=").append(totalBytes).append('\n');
            manifest.append("trees=").append(join(trees)).append('\n');
            byte[] manifestBytes = manifest.toString().getBytes("UTF-8");
            zip.putNextEntry(new ZipEntry(MANIFEST_NAME));
            zip.write(manifestBytes);
            zip.closeEntry();
            for (String relative : relativeFiles) {
                zip.putNextEntry(new ZipEntry(relative));
                try (InputStream in = new FileInputStream(new File(liveRoot, relative))) {
                    int read;
                    while ((read = in.read(buffer)) != -1) {
                        zip.write(buffer, 0, read);
                        writtenBytes += read;
                    }
                }
                zip.closeEntry();
                writtenFiles++;
            }
            zip.finish();
        } catch (IOException e) {
            throw new BackupException(ERR_IO, String.valueOf(e.getMessage()));
        }
        if (writtenFiles != relativeFiles.size() || writtenBytes != totalBytes) {
            // A save written by the game while the export streamed makes
            // the archive inconsistent with its own manifest; report it
            // instead of handing back a backup that would fail restore.
            throw new BackupException(ERR_VERIFY,
                    "files=" + writtenFiles + "/" + relativeFiles.size()
                            + " bytes=" + writtenBytes + "/" + totalBytes);
        }
        return new Summary(writtenFiles, writtenBytes, trees);
    }

    // ------------------------------------------------------------------
    // Import: stage and verify, then an explicitly confirmed swap
    // ------------------------------------------------------------------

    /**
     * Copy the chosen archive into staging and verify it completely.
     * Nothing live is read for writing and nothing live changes here; on
     * any failure the staging directory is removed again and the current
     * saves and settings stay exactly as they were.
     */
    public Summary stageImport(InputStream in) throws BackupException {
        recoverInterruptedSwap();
        File staging = stagingDir();
        deletePlain(staging);
        if (!staging.mkdirs() && !staging.isDirectory()) {
            throw new BackupException(ERR_IO, "cannot create staging directory");
        }
        try {
            File archiveFile = new File(staging, STAGED_ARCHIVE);
            long archiveBytes = 0;
            try (OutputStream out = new FileOutputStream(archiveFile)) {
                int read;
                while ((read = in.read(buffer)) != -1) {
                    out.write(buffer, 0, read);
                    archiveBytes += read;
                    long usable = liveRoot.getUsableSpace();
                    if (usable > 0 && archiveBytes > usable) {
                        throw new BackupException(ERR_NO_SPACE,
                                "archive larger than free space");
                    }
                }
            } catch (IOException e) {
                throw new BackupException(ERR_IO, String.valueOf(e.getMessage()));
            }
            Summary summary = verifyAndExtract(archiveFile);
            writeStagedSummary(summary);
            return summary;
        } catch (BackupException failure) {
            deletePlain(staging);
            throw failure;
        }
    }

    /** True while a verified import is staged and awaiting confirmation. */
    public boolean hasStagedImport() {
        return readStagedSummary() != null;
    }

    /** Drop a staged import without touching the live trees. */
    public void cancelStagedImport() {
        deletePlain(stagingDir());
    }

    /**
     * Replace the live trees named by the staged backup with the staged
     * copies. Only trees present in the backup are replaced; live trees
     * the backup does not contain are left alone. The caller must have
     * obtained an explicit overwrite confirmation for this exact staged
     * backup immediately before calling: this method never asks and never
     * defaults to overwriting.
     */
    public Summary restoreStagedImport() throws BackupException {
        Summary expected = readStagedSummary();
        if (expected == null) {
            throw new BackupException(ERR_INVALID, "no staged import");
        }
        // The summary lives on disk in the staging directory, which the
        // DocumentsProvider also exposes: re-validate its tree list here
        // so only genuine backup trees can ever be swapped, whatever the
        // summary file claims (staging verification already enforces the
        // same rule for archives).
        for (String tree : expected.trees) {
            if (!isBackupTree(tree)) {
                throw new BackupException(ERR_INVALID, "staged tree not a backup tree: " + tree);
            }
        }
        // Re-measure what is actually on disk in staging; a summary file
        // alone is never trusted at swap time.
        File extracted = stagedRoot();
        List<String> stagedFiles = new ArrayList<>();
        for (String tree : expected.trees) {
            File root = new File(extracted, tree);
            if (root.isDirectory()) {
                collectFiles(extracted, root, stagedFiles);
            }
        }
        long stagedBytes = 0;
        int stagedCore = 0;
        for (String relative : stagedFiles) {
            stagedBytes += new File(extracted, relative).length();
            String top = relative.substring(0, relative.indexOf('/'));
            if (isCoreTree(top)) {
                stagedCore++;
            }
        }
        if (stagedFiles.size() != expected.files || stagedBytes != expected.bytes
                || stagedCore == 0) {
            throw new BackupException(ERR_VERIFY,
                    "staged files=" + stagedFiles.size() + "/" + expected.files
                            + " bytes=" + stagedBytes + "/" + expected.bytes);
        }

        File rollback = rollbackDir();
        writeJournal(JOURNAL_SWAPPING);
        deletePlain(rollback);
        if (!rollback.mkdirs() && !rollback.isDirectory()) {
            writeJournal(null);
            throw new BackupException(ERR_SWAP, "cannot create rollback directory");
        }
        try {
            for (String tree : expected.trees) {
                moveDirectory(new File(liveRoot, tree), new File(rollback, tree));
            }
            for (String tree : expected.trees) {
                moveDirectory(new File(extracted, tree), new File(liveRoot, tree));
            }
        } catch (IOException e) {
            // Best-effort immediate rollback; if it cannot complete, the
            // journal stays at SWAPPING and the next launch finishes the
            // restore of the parked trees in recoverInterruptedSwap().
            if (rollbackSwap(expected.trees)) {
                writeJournal(null);
                deletePlain(rollback);
            }
            deletePlain(stagingDir());
            throw new BackupException(ERR_SWAP, String.valueOf(e.getMessage()));
        }
        writeJournal(JOURNAL_COMMITTED);
        deletePlain(rollback);
        deletePlain(stagingDir());
        writeJournal(null);
        return expected;
    }

    /**
     * Undo or finish whatever a previous process left behind. SWAPPING
     * means the live trees may be half-restored, so the parked rollback
     * copies go back before anything else trusts the live data; COMMITTED
     * leftovers only need cleaning. With no journal, a leftover rollback
     * directory cannot be part of a committed swap and is discarded;
     * staging is the caller's decision (a fresh owner discards it, which
     * is what makes a staged import die with its screen instance).
     */
    public void recoverInterruptedSwap() {
        if (liveRoot == null) {
            return;
        }
        String state = readJournal();
        File rollback = rollbackDir();
        if (JOURNAL_SWAPPING.equals(state) && rollback.isDirectory()) {
            rollbackSwap(allTreeNames());
        }
        if (state != null) {
            deletePlain(stagingDir());
            writeJournal(null);
        }
        deletePlain(rollback);
    }

    // ------------------------------------------------------------------
    // Verification and extraction
    // ------------------------------------------------------------------

    private Summary verifyAndExtract(File archiveFile) throws BackupException {
        long manifestFiles = -1;
        long manifestBytes = -1;
        List<String> manifestTrees = null;
        long declaredBytes = 0;
        int fileEntries = 0;
        int coreFiles = 0;
        Set<String> presentTrees = new HashSet<>();
        Set<String> seen = new HashSet<>();
        List<ZipEntry> toExtract = new ArrayList<>();
        try (ZipFile zip = new ZipFile(archiveFile)) {
            java.util.Enumeration<? extends ZipEntry> entries = zip.entries();
            while (entries.hasMoreElements()) {
                ZipEntry entry = entries.nextElement();
                String name = entry.getName();
                if (entry.isDirectory()) {
                    if (!isBackupTreePath(name)) {
                        throw invalidEntry(name);
                    }
                    continue;
                }
                if (MANIFEST_NAME.equals(name)) {
                    try (BufferedReader reader = new BufferedReader(
                            new java.io.InputStreamReader(zip.getInputStream(entry), "UTF-8"))) {
                        String marker = reader.readLine();
                        if (!MANIFEST_MARKER.equals(marker)) {
                            throw new BackupException(ERR_INVALID, "bad manifest marker");
                        }
                        String line;
                        while ((line = reader.readLine()) != null) {
                            if (line.startsWith("format=") && !"format=1".equals(line)) {
                                throw new BackupException(ERR_INVALID, "unsupported format");
                            }
                            if (line.startsWith("files=")) {
                                manifestFiles = parseManifestLong(line.substring(6));
                            } else if (line.startsWith("bytes=")) {
                                manifestBytes = parseManifestLong(line.substring(6));
                            } else if (line.startsWith("trees=")) {
                                manifestTrees = new ArrayList<>();
                                for (String tree : line.substring(6).split(",")) {
                                    if (!tree.isEmpty()) {
                                        manifestTrees.add(tree);
                                    }
                                }
                            }
                        }
                    }
                    continue;
                }
                if (!isBackupFilePath(name) || !seen.add(name)) {
                    throw invalidEntry(name);
                }
                if (entry.getSize() < 0) {
                    throw new BackupException(ERR_INVALID, "unknown entry size: " + name);
                }
                declaredBytes += entry.getSize();
                fileEntries++;
                String top = name.substring(0, name.indexOf('/'));
                presentTrees.add(top);
                if (isCoreTree(top)) {
                    coreFiles++;
                }
                toExtract.add(entry);
            }
            if (manifestFiles < 0 || manifestBytes < 0 || manifestTrees == null) {
                throw new BackupException(ERR_INVALID, "missing backup manifest");
            }
            if (fileEntries != manifestFiles || declaredBytes != manifestBytes
                    || coreFiles == 0) {
                throw new BackupException(ERR_INVALID,
                        "manifest files=" + manifestFiles + " entries=" + fileEntries
                                + " bytes=" + manifestBytes + "/" + declaredBytes
                                + " core=" + coreFiles);
            }
            // The manifest's tree list drives the swap, so it must name
            // exactly the backup trees that actually carry files in this
            // archive. A crafted list naming a shipped tree (or any tree
            // without files) must never reach the swap, where a listed
            // tree's live copy is parked before its staged copy moves in.
            Set<String> manifestTreeSet = new HashSet<>(manifestTrees);
            if (manifestTrees.isEmpty() || manifestTreeSet.size() != manifestTrees.size()
                    || !manifestTreeSet.equals(presentTrees)) {
                throw new BackupException(ERR_INVALID,
                        "manifest trees do not match archived trees");
            }
            for (String tree : manifestTrees) {
                if (!isBackupTree(tree)) {
                    throw invalidEntry(tree);
                }
            }
            // Free space for the extraction itself plus a reserve, checked
            // before a single staged file is written; the swap after that
            // is renames on this filesystem and needs no further space.
            long usable = liveRoot.getUsableSpace();
            if (usable > 0 && usable < declaredBytes + RESTORE_RESERVE_BYTES) {
                throw new BackupException(ERR_NO_SPACE,
                        "usable=" + usable + " needed=" + declaredBytes);
            }
            File extracted = stagedRoot();
            if (!extracted.mkdirs() && !extracted.isDirectory()) {
                throw new BackupException(ERR_IO, "cannot create staged root");
            }
            String canonicalRoot = extracted.getCanonicalPath();
            long extractedBytes = 0;
            for (ZipEntry entry : toExtract) {
                File target = new File(extracted, entry.getName());
                if (!target.getCanonicalPath().startsWith(canonicalRoot)) {
                    throw invalidEntry(entry.getName());
                }
                File parent = target.getParentFile();
                if (parent != null) {
                    parent.mkdirs();
                }
                long entryBytes = 0;
                try (InputStream in = zip.getInputStream(entry);
                     OutputStream out = new FileOutputStream(target)) {
                    int read;
                    while ((read = in.read(buffer)) != -1) {
                        out.write(buffer, 0, read);
                        entryBytes += read;
                        extractedBytes += read;
                        if (extractedBytes > declaredBytes) {
                            throw new BackupException(ERR_INVALID,
                                    "entry expands past declared size");
                        }
                    }
                }
                if (entryBytes != entry.getSize()) {
                    throw new BackupException(ERR_VERIFY,
                            "entry bytes=" + entryBytes + "/" + entry.getSize());
                }
            }
            if (extractedBytes != declaredBytes) {
                throw new BackupException(ERR_VERIFY,
                        "extracted=" + extractedBytes + "/" + declaredBytes);
            }
            return new Summary(fileEntries, declaredBytes, manifestTrees);
        } catch (IOException e) {
            // An unreadable or truncated archive is a verification
            // failure from the user's point of view, not a live-data IO
            // failure: either way the current data stays untouched.
            // (BackupExceptions thrown above are not IOExceptions and
            // already propagate with their own codes.)
            throw new BackupException(ERR_INVALID, String.valueOf(e.getMessage()));
        }
    }

    private BackupException invalidEntry(String name) {
        return new BackupException(ERR_INVALID, "entry outside backup trees: " + name);
    }

    private static long parseManifestLong(String value) throws BackupException {
        try {
            return Long.parseLong(value.trim());
        } catch (NumberFormatException e) {
            throw new BackupException(ERR_INVALID, "bad manifest number: " + value);
        }
    }

    /** Directory path inside the archive: a backup tree, or below one. */
    private static boolean isBackupTreePath(String name) {
        String path = name.endsWith("/") ? name.substring(0, name.length() - 1) : name;
        if (path.isEmpty() || path.contains("\\") || path.contains("..")) {
            return false;
        }
        int slash = path.indexOf('/');
        return isBackupTree(slash < 0 ? path : path.substring(0, slash));
    }

    /** File path inside the archive: strictly below one backup tree. */
    private static boolean isBackupFilePath(String name) {
        if (name.startsWith("/") || name.contains("\\")) {
            return false;
        }
        for (String segment : name.split("/")) {
            if (segment.isEmpty() || ".".equals(segment) || "..".equals(segment)) {
                return false;
            }
        }
        int slash = name.indexOf('/');
        return slash > 0 && isBackupTree(name.substring(0, slash));
    }

    private static boolean isBackupTree(String name) {
        for (String tree : BACKUP_TREES) {
            if (tree.equals(name)) {
                return true;
            }
        }
        return false;
    }

    private static boolean isCoreTree(String name) {
        for (String tree : CORE_TREES) {
            if (tree.equals(name)) {
                return true;
            }
        }
        return false;
    }

    private static List<String> allTreeNames() {
        List<String> result = new ArrayList<>();
        Collections.addAll(result, BACKUP_TREES);
        return result;
    }

    // ------------------------------------------------------------------
    // Staged summary and journal (same fsync discipline as R06 install)
    // ------------------------------------------------------------------

    private void writeStagedSummary(Summary summary) throws BackupException {
        File file = new File(stagingDir(), STAGED_SUMMARY);
        try (FileOutputStream out = new FileOutputStream(file)) {
            String text = "files=" + summary.files + "\nbytes=" + summary.bytes
                    + "\ntrees=" + join(summary.trees) + "\n";
            out.write(text.getBytes("UTF-8"));
            out.getFD().sync();
        } catch (IOException e) {
            throw new BackupException(ERR_IO, String.valueOf(e.getMessage()));
        }
    }

    private Summary readStagedSummary() {
        File file = new File(stagingDir(), STAGED_SUMMARY);
        if (!file.isFile()) {
            return null;
        }
        try (BufferedReader reader = new BufferedReader(new FileReader(file))) {
            long files = -1;
            long bytes = -1;
            List<String> trees = null;
            String line;
            while ((line = reader.readLine()) != null) {
                if (line.startsWith("files=")) {
                    files = Long.parseLong(line.substring(6).trim());
                } else if (line.startsWith("bytes=")) {
                    bytes = Long.parseLong(line.substring(6).trim());
                } else if (line.startsWith("trees=")) {
                    trees = new ArrayList<>();
                    for (String tree : line.substring(6).split(",")) {
                        if (!tree.isEmpty()) {
                            trees.add(tree);
                        }
                    }
                }
            }
            if (files < 0 || files > Integer.MAX_VALUE || bytes < 0 || trees == null
                    || trees.isEmpty()) {
                return null;
            }
            return new Summary((int) files, bytes, trees);
        } catch (IOException | NumberFormatException e) {
            return null;
        }
    }

    private void writeJournal(String state) {
        if (journalFile == null) {
            return;
        }
        if (state == null) {
            journalFile.delete();
            return;
        }
        try (FileOutputStream out = new FileOutputStream(journalFile)) {
            out.write((state + "\n").getBytes("UTF-8"));
            out.getFD().sync();
        } catch (IOException ignored) {
            // Same contract as the install journal: an unwritable journal
            // leaves leftovers untrusted, never treated as restored data.
        }
    }

    private String readJournal() {
        if (journalFile == null || !journalFile.isFile()) {
            return null;
        }
        try (BufferedReader reader = new BufferedReader(new FileReader(journalFile))) {
            String line = reader.readLine();
            return line == null ? null : line.trim();
        } catch (IOException e) {
            return null;
        }
    }

    // ------------------------------------------------------------------
    // File helpers (mirroring the SplashScreen install helpers)
    // ------------------------------------------------------------------

    /** Parked rollback trees go back over whatever the swap left live. */
    private boolean rollbackSwap(List<String> trees) {
        boolean complete = true;
        File rollback = rollbackDir();
        for (String tree : trees) {
            File parked = new File(rollback, tree);
            if (parked.exists()) {
                deletePlain(new File(liveRoot, tree));
                if (!parked.renameTo(new File(liveRoot, tree))) {
                    complete = false;
                }
            }
        }
        return complete;
    }

    private void moveDirectory(File from, File to) throws IOException {
        if (!from.exists()) {
            return;
        }
        if (to.exists()) {
            throw new IOException("swap target already exists: " + to.getPath());
        }
        File parent = to.getParentFile();
        if (parent != null) {
            parent.mkdirs();
        }
        if (!from.renameTo(to)) {
            throw new IOException("cannot move " + from.getPath() + " to " + to.getPath());
        }
    }

    private static void collectFiles(File base, File current, List<String> out) {
        File[] children = current.listFiles();
        if (children == null) {
            return;
        }
        for (File child : children) {
            if (child.isDirectory()) {
                collectFiles(base, child, out);
            } else {
                out.add(child.getPath().substring(base.getPath().length() + 1)
                        .replace(File.separatorChar, '/'));
            }
        }
    }

    private static void deletePlain(File fileOrDirectory) {
        if (fileOrDirectory == null || !fileOrDirectory.exists()) {
            return;
        }
        if (fileOrDirectory.isDirectory()) {
            File[] children = fileOrDirectory.listFiles();
            if (children != null) {
                for (File child : children) {
                    deletePlain(child);
                }
            }
        }
        fileOrDirectory.delete();
    }

    private static String join(List<String> values) {
        StringBuilder result = new StringBuilder();
        for (String value : values) {
            if (result.length() > 0) {
                result.append(',');
            }
            result.append(value);
        }
        return result.toString();
    }
}

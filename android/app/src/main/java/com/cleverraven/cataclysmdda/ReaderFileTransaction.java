package com.cleverraven.cataclysmdda;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/** Same-filesystem directory replacement. A failed recovery retains its journal and originals. */
final class ReaderFileTransaction {
    interface BeforeCommit { void run() throws IOException; }

    private final File live;
    private final File rollback;
    private final File journal;
    private final List<String> allowed;
    private final String prefix;

    ReaderFileTransaction(File live, File rollback, File journal, String prefix, String[] roots) {
        this.live = live;
        this.rollback = rollback;
        this.journal = journal;
        this.prefix = prefix;
        this.allowed = Arrays.asList(roots);
    }

    void replace(File staged, List<String> roots, BeforeCommit beforeCommit) throws IOException {
        recover();
        validateRoots(roots);
        List<String> originals = new ArrayList<>();
        for (String root : roots) {
            if (new File(live, root).exists()) { originals.add(root); }
        }
        // Persist intent before the first destructive rename. Failure is fatal.
        write("SWAPPING", roots, originals);
        try {
            if (!rollback.mkdir()) { throw new IOException("Cannot create rollback directory"); }
            for (String root : originals) {
                move(new File(live, root), new File(rollback, root));
            }
            for (String root : roots) {
                move(new File(staged, root), new File(live, root));
            }
            if (beforeCommit != null) { beforeCommit.run(); }
            write("COMMITTED", roots, originals);
        } catch (IOException failure) {
            try { recover(); } catch (IOException recoveryFailure) {
                failure.addSuppressed(recoveryFailure);
            }
            throw failure;
        }
        // COMMITTED is durable before cleanup. Retry cleanup at the next launch if needed.
        cleanupCommitted();
    }

    void recover() throws IOException {
        if (!journal.exists()) {
            if (rollback.exists()) {
                throw new IOException("Unjournaled rollback retained for recovery");
            }
            return;
        }
        String state;
        List<String> roots = null;
        List<String> originals = null;
        try (BufferedReader in = new BufferedReader(new InputStreamReader(
                new FileInputStream(journal), "UTF-8"))) {
            state = in.readLine();
            String line;
            while ((line = in.readLine()) != null) {
                if (line.startsWith("roots=")) { roots = split(line.substring(6)); }
                if (line.startsWith("originals=")) { originals = split(line.substring(10)); }
            }
        }
        if ((prefix + "COMMITTED").equals(state)) {
            cleanupCommitted();
            return;
        }
        if ((prefix + "STAGING").equals(state) || (prefix + "STAGED").equals(state)) {
            if (rollback.exists()) { throw new IOException("Unexpected rollback retained"); }
            removeJournal();
            return;
        }
        if (!(prefix + "SWAPPING").equals(state)) {
            throw new IOException("Unknown transaction state; originals retained");
        }
        // Read the pre-fix journal format conservatively. Do not delete a live
        // root whose original existence was not recorded by that old format.
        if (roots == null || originals == null) {
            roots = new ArrayList<>(allowed);
            originals = new ArrayList<>(allowed);
        }
        validateRoots(roots);
        validateRoots(originals);
        if (!roots.containsAll(originals)) { throw new IOException("Invalid original roots"); }
        for (String root : roots) {
            File parked = new File(rollback, root);
            File target = new File(live, root);
            if (parked.exists()) {
                deleteChecked(target);
                move(parked, target);
            } else if (!originals.contains(root)) {
                deleteChecked(target);
            }
            // Missing parked original means it was never parked, or has
            // already been restored by an earlier interrupted recovery.
        }
        // Reached only after every restoration succeeded. Never erase the
        // only good copy or clear the journal after a failed rename.
        deleteChecked(rollback);
        removeJournal();
    }

    private void cleanupCommitted() throws IOException {
        deleteChecked(rollback);
        removeJournal();
    }

    private void removeJournal() throws IOException {
        if (journal.exists() && !journal.delete()) { throw new IOException("Cannot clear journal"); }
    }

    private void write(String state, List<String> roots, List<String> originals) throws IOException {
        File parent = journal.getParentFile();
        if (parent == null || (!parent.isDirectory() && !parent.mkdirs())) {
            throw new IOException("Journal directory unavailable");
        }
        File temporary = new File(parent, journal.getName() + ".new");
        try (FileOutputStream out = new FileOutputStream(temporary)) {
            String text = prefix + state + "\nroots=" + join(roots)
                    + "\noriginals=" + join(originals) + "\n";
            out.write(text.getBytes("UTF-8"));
            out.getFD().sync();
        }
        if (!temporary.renameTo(journal)) {
            throw new IOException("Cannot publish transaction journal");
        }
    }

    private void validateRoots(List<String> roots) throws IOException {
        Set<String> seen = new HashSet<>();
        for (String root : roots) {
            if (!allowed.contains(root) || !seen.add(root)) {
                throw new IOException("Invalid transaction root");
            }
        }
    }

    private static List<String> split(String text) {
        return text.isEmpty() ? new ArrayList<>() : new ArrayList<>(Arrays.asList(text.split(",")));
    }

    private static String join(List<String> values) {
        StringBuilder text = new StringBuilder();
        for (String value : values) {
            if (text.length() != 0) { text.append(','); }
            text.append(value);
        }
        return text.toString();
    }

    private static void move(File from, File to) throws IOException {
        if (!from.exists()) { throw new IOException("Missing transaction source: " + from); }
        if (to.exists() || !from.renameTo(to)) {
            throw new IOException("Cannot restore/move directory: " + from + " -> " + to);
        }
    }

    static void deleteChecked(File path) throws IOException {
        if (!path.exists()) { return; }
        File parent = path.getParentFile();
        File expected = parent == null ? path.getAbsoluteFile()
                : new File(parent.getCanonicalFile(), path.getName());
        if (!path.getCanonicalFile().equals(expected)) {
            throw new IOException("Refusing transaction symlink: " + path);
        }
        if (path.isDirectory()) {
            File[] children = path.listFiles();
            if (children == null) { throw new IOException("Cannot list directory: " + path); }
            for (File child : children) { deleteChecked(child); }
        }
        if (!path.delete()) { throw new IOException("Cannot remove transaction path: " + path); }
    }
}

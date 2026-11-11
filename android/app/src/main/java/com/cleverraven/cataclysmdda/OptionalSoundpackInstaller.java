package com.cleverraven.cataclysmdda;

import android.os.SystemClock;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FilterInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.HashSet;
import java.util.Set;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

/**
 * Installs the pinned Bl@ck Edition soundpack once, outside the APK.
 *
 * The archive is pinned to one audited Git commit; no untrusted URL,
 * in-place game data edits, or automatic soundpack replacement are allowed.
 * Downloading and extraction are both streaming, bounded, and cancellable.
 * The repository-root soundpack files are copied into data/sound.
 */
public final class OptionalSoundpackInstaller {
    public static final String PACK_DIRECTORY = "Bl@ck Edition soundpack";
    public static final String PACK_ID = "Bl@ck Edition soundpack";
    public static final String SOURCE_COMMIT = "061d784e5e98ad4e1fc847f22b5fd8a389e2954d";
    private static final String ARCHIVE_URL =
            "https://codeload.github.com/chronicpayne/Bl-ck-Edition-Soundpack/zip/" + SOURCE_COMMIT;
    private static final String ARCHIVE_FOLDER = "Bl-ck-Edition-Soundpack-";
    private static final long MIB = 1024L * 1024L;
    // Pinned tree is ~274 MB uncompressed; reserve extra storage for installation.
    private static final long MIN_FREE_BYTES = 600L * MIB;
    private static final long MIN_REMAINING_BYTES = 160L * MIB;
    private static final long MAX_COMPRESSED_BYTES = 1000L * MIB;
    private static final long MAX_EXTRACTED_BYTES = 1500L * MIB;
    private static final long MAX_FILE_BYTES = 256L * MIB;
    private static final int MAX_FILES = 3500;
    private static final int MAX_ARCHIVE_ENTRIES = 5000;
    private static final int MIN_AUDIO_FILES = 1000;

    public interface Listener {
        void onProgress(long receivedBytes, long extractedBytes, int files);
    }

    private volatile boolean cancelled;

    public void cancel() {
        cancelled = true;
    }

    public static File destination(File gameRoot) {
        return new File(gameRoot, "data/sound/" + PACK_DIRECTORY);
    }

    public static boolean isInstalled(File gameRoot) {
        if (gameRoot == null) { return false; }
        File target = destination(gameRoot);
        // Respect a user-installed copy of this soundpack as well as our download.
        File definition = new File(target, "soundpack.txt");
        File effects = new File(target, "effects.json");
        File player = new File(target, "player.json");
        if (!definition.isFile() || !effects.isFile() || !player.isFile()) {
            return false;
        }
        try (InputStream in = new FileInputStream(definition)) {
            byte[] bytes = new byte[256];
            int read = in.read(bytes);
            return read > 0 &&
                    new String(bytes, 0, read, StandardCharsets.UTF_8)
                            .contains("NAME: " + PACK_ID);
        } catch (IOException error) {
            return false;
        }
    }

    public void downloadAndInstall(File gameRoot, Listener listener) throws IOException {
        if (gameRoot == null) { throw new IOException("Missing game files directory"); }
        File soundRoot = new File(gameRoot, "data/sound");
        if (!soundRoot.isDirectory() && !soundRoot.mkdirs()) {
            throw new IOException("Could not create the sound directory");
        }
        if (isInstalled(gameRoot)) { return; }
        File destination = destination(gameRoot);
        if (destination.exists()) {
            // A user-owned partial/custom installation must never be deleted.
            throw new IOException("Soundpack directory already exists but is incomplete; "
                    + "please rename that directory before retrying");
        }
        if (soundRoot.getUsableSpace() < MIN_FREE_BYTES) {
            throw new IOException("Not enough storage space; at least 600 MB free is required");
        }
        File staging = new File(soundRoot, ".reader_black_edition_staging");
        if (staging.exists()) { removeStaging(staging); }
        if (!staging.mkdirs()) {
            throw new IOException("Could not prepare temporary Otopack directory");
        }
        boolean committed = false;
        HttpURLConnection connection = null;
        long compressedBytes = 0;
        long extractedBytes = 0;
        int totalFiles = 0;
        int audioFiles = 0;
        int archiveEntries = 0;
        long lastReportAt = 0;
        Set<String> seen = new HashSet<>();
        try {
            connection = (HttpURLConnection) new URL(ARCHIVE_URL).openConnection();
            connection.setConnectTimeout(25000);
            connection.setReadTimeout(45000);
            connection.setRequestProperty("Accept-Encoding", "identity");
            connection.setRequestProperty("User-Agent", "CDDA-Android-Reader-Otopack-Installer");
            connection.setInstanceFollowRedirects(true);
            if (!"https".equalsIgnoreCase(connection.getURL().getProtocol())
                    || connection.getResponseCode() != HttpURLConnection.HTTP_OK) {
                throw new IOException("Unable to download soundpack from GitHub");
            }
            long advertised = connection.getContentLengthLong();
            if (advertised > MAX_COMPRESSED_BYTES) {
                throw new IOException("Soundpack download exceeds size limit");
            }
            // GitHub codeload wraps the soundpack in a repository-root directory.
            // Extract its children only, never directly into the game root.
            try (InputStream network = new BufferedInputStream(connection.getInputStream());
                 InputStream raw = new FilterInputStream(network) {
                     private long bytes;
                     @Override
                     public int read() throws IOException {
                         int value = super.read();
                         if (value != -1 && ++bytes > MAX_COMPRESSED_BYTES) {
                             throw new IOException("Soundpack archive exceeds size limit");
                         }
                         return value;
                     }
                     @Override
                     public int read(byte[] data, int off, int len) throws IOException {
                         int count = in.read(data, off, len);
                         if (count > 0 && (bytes += count) > MAX_COMPRESSED_BYTES) {
                             throw new IOException("Soundpack archive exceeds size limit");
                         }
                         return count;
                     }
                 };
                 ZipInputStream zip = new ZipInputStream(raw)) {
                ZipEntry entry;
                byte[] buffer = new byte[64 * 1024];
                while ((entry = zip.getNextEntry()) != null) {
                    ensureRunning();
                    if (++archiveEntries > MAX_ARCHIVE_ENTRIES) {
                        throw new IOException("Too many archive entries");
                    }
                    String name = entry.getName();
                    String relative = childPath(name);
                    if (relative == null || relative.isEmpty()) {
                        zip.closeEntry();
                        continue;
                    }
                    if (!safePath(relative)) {
                        throw new IOException("Unsafe path in soundpack archive");
                    }
                    File output = new File(staging, relative);
                    String base = staging.getCanonicalPath() + File.separator;
                    if (!output.getCanonicalPath().startsWith(base)) {
                        throw new IOException("Soundpack archive attempted to escape its folder");
                    }
                    if (entry.isDirectory()) {
                        if (!output.isDirectory() && !output.mkdirs()) {
                            throw new IOException("Cannot create soundpack directory");
                        }
                        zip.closeEntry();
                        continue;
                    }
                    if (!seen.add(relative) || ++totalFiles > MAX_FILES) {
                        throw new IOException("Duplicate or excessive soundpack files");
                    }
                    if (!output.getParentFile().isDirectory() && !output.getParentFile().mkdirs()) {
                        throw new IOException("Cannot create soundpack folder");
                    }
                    long oneFile = 0;
                    try (FileOutputStream rawFile = new FileOutputStream(output);
                         BufferedOutputStream dest = new BufferedOutputStream(rawFile)) {
                        int n;
                        while ((n = zip.read(buffer)) != -1) {
                            ensureRunning();
                            oneFile += n;
                            extractedBytes += n;
                            if (oneFile > MAX_FILE_BYTES || extractedBytes > MAX_EXTRACTED_BYTES) {
                                throw new IOException("Soundpack content exceeds safety limit");
                            }
                            dest.write(buffer, 0, n);
                        }
                    }
                    if (relative.endsWith(".ogg") || relative.endsWith(".mp3")
                            || relative.endsWith(".wav")) { audioFiles++; }
                    zip.closeEntry(); // validates each entry's CRC
                    long now = SystemClock.elapsedRealtime();
                    if (now - lastReportAt >= 15000 || totalFiles == 1) {
                        compressedBytes = connection.getContentLengthLong();
                        if (listener != null) { listener.onProgress(compressedBytes, extractedBytes, totalFiles); }
                        lastReportAt = now;
                    }
                    if (soundRoot.getUsableSpace() < MIN_REMAINING_BYTES) {
                        throw new IOException("Storage too low to finish installing soundpack");
                    }
                }
            }
            if (audioFiles < MIN_AUDIO_FILES || totalFiles < MIN_AUDIO_FILES
                    || !isValidStaging(staging)) {
                throw new IOException("Incomplete Bl@ck Edition download; nothing was installed");
            }
            File marker = new File(staging, ".reader_install_source.txt");
            try (FileOutputStream out = new FileOutputStream(marker)) {
                out.write((SOURCE_COMMIT + "\n" + audioFiles + " audio files\n")
                        .getBytes(StandardCharsets.UTF_8));
                out.getFD().sync();
            }
            ensureRunning();
            if (destination.exists() || !staging.renameTo(destination)) {
                throw new IOException("Unable to activate soundpack; existing data was not changed");
            }
            committed = true;
        } finally {
            if (connection != null) { connection.disconnect(); }
            if (!committed) { removeStaging(staging); }
        }
    }

    private void ensureRunning() throws IOException {
        if (cancelled || Thread.currentThread().isInterrupted()) {
            throw new IOException("Soundpack download cancelled");
        }
    }

    private static boolean isValidStaging(File staging) {
        return new File(staging, "soundpack.txt").isFile()
                && new File(staging, "effects.json").isFile()
                && new File(staging, "player.json").isFile();
    }

    private static String childPath(String archiveName) {
        if (!archiveName.startsWith(ARCHIVE_FOLDER)) { return null; }
        int rootEnd = archiveName.indexOf('/');
        if (rootEnd < 0) { return null; }
        return archiveName.substring(rootEnd + 1);
    }

    private static boolean safePath(String relative) {
        if (relative.startsWith("/") || relative.indexOf('\\') >= 0
                || relative.indexOf(':') >= 0 || relative.indexOf((char) 0) >= 0) {
            return false;
        }
        for (String segment : relative.split("/")) {
            if ("..".equals(segment) || ".".equals(segment) || segment.isEmpty()) {
                return false;
            }
        }
        return true;
    }

    private static void removeStaging(File location) throws IOException {
        if (!location.exists()) { return; }
        if (!".reader_black_edition_staging".equals(location.getName())) {
            throw new IOException("Refusing to remove a non-staging path");
        }
        removeChildren(location);
    }

    private static void removeChildren(File file) throws IOException {
        if (file.isDirectory()) {
            // This tree was created by the installer; prevent symlink traversal.
            File parent = file.getParentFile();
            if (parent != null && !file.getCanonicalFile().equals(
                    new File(parent.getCanonicalFile(), file.getName()))) {
                throw new IOException("Unexpected redirected staging path");
            }
            File[] children = file.listFiles();
            if (children == null) { throw new IOException("Cannot clean staging directory"); }
            for (File child : children) { removeChildren(child); }
        }
        if (!file.delete()) { throw new IOException("Cannot clean staging file"); }
    }
}

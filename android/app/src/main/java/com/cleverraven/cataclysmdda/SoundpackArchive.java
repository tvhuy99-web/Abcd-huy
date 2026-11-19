package com.cleverraven.cataclysmdda;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FilterInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Set;
import java.util.UUID;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;
import java.util.zip.ZipOutputStream;

/**
 * Bounded, streaming SAF ZIP importer/exporter for Cataclysm soundpacks.
 * It never edits a bundled soundpack or overwrites any installed pack.
 *
 * Soundpacks are identified by NAME: in soundpack.txt, not by ZIP filenames.
 * Only files under USER_DIR/sound are created during import.
 */
final class SoundpackArchive {
    static final long MAX_COMPRESSED = 1024L * 1024 * 1024;
    static final long MAX_EXPANDED = 1536L * 1024 * 1024;
    static final long MAX_SINGLE = 256L * 1024 * 1024;
    static final int MAX_ENTRIES = 5000;
    private static final int BUFFER = 64 * 1024;
    private static final int MAX_META = 64 * 1024;
    private static final String STAGING_PREFIX = ".reader-sound-import-";

    static final class Pack {
        final String id;
        final String display;
        final File folder;
        final boolean bundled;

        Pack(String id, String display, File folder, boolean bundled) {
            this.id = id;
            this.display = display;
            this.folder = folder;
            this.bundled = bundled;
        }
    }

    private static final class BoundedInput extends FilterInputStream {
        long consumed;
        BoundedInput(InputStream input) { super(input); }

        @Override public int read() throws IOException {
            int value = in.read();
            if (value >= 0 && ++consumed > MAX_COMPRESSED) {
                throw new IOException("Tệp ZIP âm thanh vượt giới hạn 1 GB");
            }
            return value;
        }

        @Override public int read(byte[] out, int offset, int length) throws IOException {
            int count = in.read(out, offset, length);
            if (count > 0 && (consumed += count) > MAX_COMPRESSED) {
                throw new IOException("Tệp ZIP âm thanh vượt giới hạn 1 GB");
            }
            return count;
        }
    }

    private SoundpackArchive() { }

    private static boolean safeRelative(String name) {
        if (name == null || name.isEmpty() || name.length() > 1024
                || name.startsWith("/") || name.indexOf('\\') >= 0
                || name.indexOf(':') >= 0) {
            return false;
        }
        String[] components = name.split("/", -1);
        for (int i = 0; i < components.length; i++) {
            String part = components[i];
            if (part.isEmpty()) {
                if (i != components.length - 1) { return false; }
                continue;
            }
            if (".".equals(part) || "..".equals(part)) { return false; }
            for (int j = 0; j < part.length(); j++) {
                if (part.charAt(j) < 32 || part.charAt(j) == 127) { return false; }
            }
        }
        return true;
    }

    private static void inside(File base, File child) throws IOException {
        String prefix = base.getCanonicalPath() + File.separator;
        if (!child.getCanonicalPath().startsWith(prefix)) {
            throw new IOException("Đường dẫn âm thanh không an toàn");
        }
    }

    private static String readMetadata(File definition, String key) throws IOException {
        if (!definition.isFile() || definition.length() > MAX_META) {
            throw new IOException("Thiếu hoặc quá lớn: soundpack.txt");
        }
        try (java.io.BufferedReader reader = new java.io.BufferedReader(
                new java.io.InputStreamReader(new FileInputStream(definition),
                        StandardCharsets.UTF_8))) {
            String line;
            while ((line = reader.readLine()) != null) {
                String trimmed = line.trim();
                if (trimmed.startsWith(key + ":")) {
                    return trimmed.substring(key.length() + 1).trim();
                }
            }
        }
        return "";
    }

    private static Pack inspect(File folder, boolean bundled) throws IOException {
        String id = readMetadata(new File(folder, "soundpack.txt"), "NAME");
        String display = readMetadata(new File(folder, "soundpack.txt"), "VIEW");
        if (id.isEmpty() || id.length() > 120 || id.indexOf('/') >= 0
                || id.indexOf('\\') >= 0 || id.indexOf('\n') >= 0
                || id.indexOf('\r') >= 0 || id.equals(".") || id.equals("..")) {
            throw new IOException("NAME không hợp lệ trong soundpack.txt");
        }
        if (display.isEmpty()) { display = id; }
        if (display.length() > 160) { display = display.substring(0, 160); }
        return new Pack(id, display, folder.getCanonicalFile(), bundled);
    }

    private static void findPacks(File root, File dir, int depth, boolean bundled,
                                  List<Pack> result, Set<String> ids) throws IOException {
        if (!dir.isDirectory() || depth > 4 || result.size() >= 100) { return; }
        if (!dir.getCanonicalPath().equals(root.getCanonicalPath())) {
            inside(root, dir);
        }
        File definition = new File(dir, "soundpack.txt");
        if (definition.isFile()) {
            try {
                Pack pack = inspect(dir, bundled);
                if (ids.add(pack.id)) { result.add(pack); }
            } catch (IOException invalid) {
                // Ignore incomplete downloads and staged packs in the chooser.
            }
            return;
        }
        File[] children = dir.listFiles();
        if (children == null) { return; }
        for (File child : children) {
            if (child.getName().startsWith(".")) { continue; }
            if (child.isDirectory()) {
                findPacks(root, child, depth + 1, bundled, result, ids);
            }
        }
    }

    static List<Pack> list(File userSound, File bundledSound) throws IOException {
        List<Pack> found = new ArrayList<>();
        Set<String> ids = new HashSet<>();
        if (userSound.isDirectory()) {
            findPacks(userSound.getCanonicalFile(), userSound, 0, false, found, ids);
        }
        if (bundledSound.isDirectory()) {
            findPacks(bundledSound.getCanonicalFile(), bundledSound, 0, true, found, ids);
        }
        Collections.sort(found, Comparator.comparing(
                a -> a.display.toLowerCase(Locale.ROOT)));
        return found;
    }

    private static File createStaging(File userSound) throws IOException {
        if (!userSound.isDirectory() && !userSound.mkdirs()) {
            throw new IOException("Không tạo được thư mục âm thanh riêng");
        }
        File stage = new File(userSound,
                STAGING_PREFIX + UUID.randomUUID()).getCanonicalFile();
        inside(userSound, stage);
        if (!stage.mkdir()) {
            throw new IOException("Không tạo được thư mục tạm cho gói âm thanh");
        }
        return stage;
    }

    private static void cleanup(File root, File file, int depth) throws IOException {
        if (!file.exists()) { return; }
        if (depth > 40) { throw new IOException("Cây thư mục âm thanh quá sâu"); }
        if (!root.getCanonicalPath().equals(file.getCanonicalPath())) {
            inside(root, file);
        }
        if (file.isDirectory()) {
            File[] children = file.listFiles();
            if (children == null) { throw new IOException("Không đọc được thư mục tạm"); }
            for (File child : children) { cleanup(root, child, depth + 1); }
        }
        if (!file.delete()) { throw new IOException("Không xóa được dữ liệu tạm"); }
    }

    private static String stableFolder(String id) throws IOException {
        try {
            byte[] hashed = MessageDigest.getInstance("SHA-256").digest(
                    id.getBytes(StandardCharsets.UTF_8));
            StringBuilder result = new StringBuilder("imported-");
            for (int i = 0; i < 16; i++) {
                result.append(String.format(Locale.ROOT, "%02x", hashed[i] & 0xff));
            }
            return result.toString();
        } catch (NoSuchAlgorithmException unavailable) {
            throw new IOException("Không thể tạo tên thư mục an toàn", unavailable);
        }
    }

    private static File packRoot(File staging) throws IOException {
        File root = new File(staging, "soundpack.txt");
        if (root.isFile()) { return staging; }
        File[] children = staging.listFiles();
        if (children == null) { throw new IOException("Không đọc được ZIP âm thanh"); }
        File selected = null;
        for (File child : children) {
            if (child.isDirectory() && new File(child, "soundpack.txt").isFile()) {
                if (selected != null) {
                    throw new IOException("ZIP chứa nhiều hơn một bộ âm thanh");
                }
                selected = child;
            }
        }
        if (selected == null) { throw new IOException("ZIP không có soundpack.txt ở thư mục gốc"); }
        return selected;
    }

    private static boolean audioName(String name) {
        String lower = name.toLowerCase(Locale.ROOT);
        return lower.endsWith(".ogg") || lower.endsWith(".wav")
                || lower.endsWith(".mp3") || lower.endsWith(".flac")
                || lower.endsWith(".opus") || lower.endsWith(".aiff");
    }

    private static int[] verifyContents(File root, File here, int depth) throws IOException {
        if (depth > 32) { throw new IOException("Thư mục âm thanh quá sâu"); }
        inside(root, here);
        int audio = 0;
        int json = 0;
        File[] children = here.listFiles();
        if (children == null) { throw new IOException("Không đọc được bộ âm thanh"); }
        for (File child : children) {
            inside(root, child);
            if (child.isDirectory()) {
                int[] nested = verifyContents(root, child, depth + 1);
                audio += nested[0];
                json += nested[1];
            } else {
                if (audioName(child.getName())) { audio++; }
                if (child.getName().endsWith(".json")) { json++; }
            }
        }
        return new int[] {audio, json};
    }

    static Pack importZip(InputStream input, File userSound, File bundledSound)
            throws IOException {
        if (input == null) { throw new IOException("Không mở được tệp ZIP"); }
        File userRoot = userSound.getCanonicalFile();
        File stage = createStaging(userRoot);
        boolean moved = false;
        try {
            long total = 0;
            int entries = 0;
            Set<String> names = new HashSet<>();
            BoundedInput limited = new BoundedInput(new BufferedInputStream(input, BUFFER));
            try (ZipInputStream zip = new ZipInputStream(limited)) {
                byte[] buffer = new byte[BUFFER];
                ZipEntry entry;
                while ((entry = zip.getNextEntry()) != null) {
                    if (++entries > MAX_ENTRIES) {
                        throw new IOException("ZIP có quá nhiều tệp");
                    }
                    String name = entry.getName();
                    if (!safeRelative(name) || !names.add(name)) {
                        throw new IOException("ZIP có đường dẫn hoặc tên tệp trùng không an toàn");
                    }
                    File target = new File(stage, name);
                    inside(stage, target);
                    if (entry.isDirectory()) {
                        if (!target.isDirectory() && !target.mkdirs()) {
                            throw new IOException("Không tạo được thư mục âm thanh");
                        }
                        zip.closeEntry();
                        continue;
                    }
                    File parent = target.getParentFile();
                    if (!parent.isDirectory() && !parent.mkdirs()) {
                        throw new IOException("Không tạo được thư mục chứa tệp");
                    }
                    if (target.exists()) {
                        throw new IOException("ZIP có tên tệp trùng với thư mục");
                    }
                    long perFile = 0;
                    try (OutputStream output = new BufferedOutputStream(
                            new FileOutputStream(target), BUFFER)) {
                        int n;
                        while ((n = zip.read(buffer)) >= 0) {
                            if (n == 0) { continue; }
                            total += n;
                            perFile += n;
                            if (total > MAX_EXPANDED || perFile > MAX_SINGLE) {
                                throw new IOException("Bộ âm thanh vượt giới hạn dung lượng");
                            }
                            output.write(buffer, 0, n);
                        }
                    }
                    zip.closeEntry();
                }
            }
            File source = packRoot(stage);
            Pack pack = inspect(source, false);
            int[] assets = verifyContents(source, source, 0);
            if (assets[0] == 0 || assets[1] == 0) {
                throw new IOException("Bộ âm thanh cần ít nhất một tệp âm thanh và cấu hình JSON");
            }
            for (Pack installed : list(userSound, bundledSound)) {
                if (installed.id.equals(pack.id)) {
                    throw new IOException("Đã có bộ âm thanh tên " + pack.id);
                }
            }
            File destination = new File(userRoot, stableFolder(pack.id)).getCanonicalFile();
            inside(userRoot, destination);
            if (destination.exists()) {
                throw new IOException("Tên thư mục âm thanh đã tồn tại, không ghi đè");
            }
            if (!source.renameTo(destination)) {
                throw new IOException("Không chuyển được bộ âm thanh sang thư mục chính");
            }
            moved = true;
            // A wrapped archive leaves its now-empty parent stage in place.
            if (stage.exists()) { cleanup(stage, stage, 0); }
            return new Pack(pack.id, pack.display, destination, false);
        } finally {
            if (stage.exists()) {
                // Keep any installed destination intact even if staging cleanup fails.
                try { cleanup(stage, stage, 0); }
                catch (IOException cleanupError) {
                    if (!moved) { throw cleanupError; }
                }
            }
        }
    }

    private static long zipFiles(File root, File item, ZipOutputStream zip,
                                 byte[] buffer, int depth, int[] entries,
                                 long[] expanded) throws IOException {
        if (depth > 32 || ++entries[0] > MAX_ENTRIES) {
            throw new IOException("Bộ âm thanh có quá nhiều tệp hoặc thư mục");
        }
        inside(root, item);
        String relative = root.toURI().relativize(item.toURI()).getPath();
        if (!safeRelative(relative)) {
            throw new IOException("Tên tệp trong gói âm thanh không an toàn");
        }
        if (item.isDirectory()) {
            File[] children = item.listFiles();
            if (children == null) { throw new IOException("Không đọc được thư mục âm thanh"); }
            for (File child : children) {
                zipFiles(root, child, zip, buffer, depth + 1, entries, expanded);
            }
            return expanded[0];
        }
        if (!item.isFile() || item.length() > MAX_SINGLE) {
            throw new IOException("Không hỗ trợ tệp âm thanh quá lớn");
        }
        expanded[0] += item.length();
        if (expanded[0] > MAX_EXPANDED) {
            throw new IOException("Bộ âm thanh vượt giới hạn dung lượng xuất");
        }
        zip.putNextEntry(new ZipEntry(relative));
        try (InputStream in = new BufferedInputStream(new FileInputStream(item), BUFFER)) {
            int n;
            while ((n = in.read(buffer)) != -1) { zip.write(buffer, 0, n); }
        }
        zip.closeEntry();
        return expanded[0];
    }

    static void exportZip(OutputStream output, File userSound, File bundledSound, String id)
            throws IOException {
        if (output == null) { throw new IOException("Không mở được tệp xuất"); }
        Pack selected = null;
        for (Pack item : list(userSound, bundledSound)) {
            if (item.id.equals(id)) { selected = item; break; }
        }
        if (selected == null) { throw new IOException("Bộ âm thanh đã chọn không còn tồn tại"); }
        if (!selected.folder.isDirectory()
                || !new File(selected.folder, "soundpack.txt").isFile()) {
            throw new IOException("Bộ âm thanh không còn đầy đủ");
        }
        try (ZipOutputStream zip = new ZipOutputStream(
                new BufferedOutputStream(output, BUFFER))) {
            int[] entries = {0};
            long[] total = {0};
            byte[] bytes = new byte[BUFFER];
            File[] files = selected.folder.listFiles();
            if (files == null) { throw new IOException("Không đọc được thư mục âm thanh"); }
            for (File file : files) {
                zipFiles(selected.folder, file, zip, bytes, 0, entries, total);
            }
        }
    }
}

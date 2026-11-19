package com.cleverraven.cataclysmdda;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.Locale;
import java.util.UUID;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;
import java.util.zip.ZipOutputStream;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

/**
 * ZIP-only import/export for app-private USER_DIR/mods/manual/<mod-id>.
 *
 * Never writes game data or saved worlds, never follows archive paths outside
 * staging, and never replaces an installed mod. Call on a worker thread.
 */
final class ManualModArchive {
    static final int MAX_FILES = 3000;
    static final long MAX_ZIP = 48L * 1024 * 1024;
    static final long MAX_EXPANDED = 100L * 1024 * 1024;
    static final long MAX_FILE = 20L * 1024 * 1024;
    private static final int MAX_MODINFO = 256 * 1024;

    private ManualModArchive() { }

    private static final class LimitedInput extends java.io.FilterInputStream {
        private long used;
        LimitedInput(InputStream stream) { super(stream); }

        @Override public int read() throws IOException {
            int value = super.read();
            if (value >= 0 && ++used > MAX_ZIP) {
                throw new IOException("ZIP vượt giới hạn kích thước");
            }
            return value;
        }

        @Override public int read(byte[] bytes, int offset, int length) throws IOException {
            int count = in.read(bytes, offset, length);
            if (count > 0 && (used += count) > MAX_ZIP) {
                throw new IOException("ZIP vượt giới hạn kích thước");
            }
            return count;
        }
    }

    private static void validId(String id) throws IOException {
        if (id == null || !id.matches("[A-Za-z0-9][A-Za-z0-9_.+\\-]{0,79}")
                || id.equals(".") || id.equals("..")) {
            throw new IOException("Mã mod không hợp lệ");
        }
    }

    private static File privateRoot(String userModsPath) throws IOException {
        if (userModsPath == null || userModsPath.isEmpty()) {
            throw new IOException("Không tìm thấy thư mục mod của trò chơi");
        }
        File mods = new File(userModsPath).getCanonicalFile();
        File root = new File(mods, "manual").getCanonicalFile();
        if (!root.getPath().startsWith(mods.getPath() + File.separator) || !root.isDirectory() && !root.mkdirs()) {
            throw new IOException("Không thể tạo thư mục nhập mod");
        }
        return root;
    }

    private static void within(File parent, File child) throws IOException {
        File base = parent.getCanonicalFile();
        File file = child.getCanonicalFile();
        if (!file.getPath().startsWith(base.getPath() + File.separator)) {
            throw new IOException("Đường dẫn tệp không an toàn");
        }
    }

    private static void removeTree(File root) throws IOException {
        if (!root.exists()) { return; }
        File[] children = root.listFiles();
        if (root.isDirectory() && children != null) {
            for (File file : children) {
                within(root, file);
                removeTree(file);
            }
        }
        if (!root.delete()) {
            throw new IOException("Không thể xóa tệp");
        }
    }

    private static void collectModinfo(File root, ArrayList<File> found) throws IOException {
        File[] files = root.listFiles();
        if (files == null) { return; }
        for (File file : files) {
            within(root, file);
            if (file.isDirectory()) {
                collectModinfo(file, found);
            } else if (file.getName().equals("modinfo.json")) {
                found.add(file);
            }
        }
    }

    private static String parseModId(File file) throws IOException {
        if (file.length() > MAX_MODINFO) {
            throw new IOException("modinfo.json quá lớn");
        }
        ByteArrayOutputStream output = new ByteArrayOutputStream();
        try (InputStream stream = new FileInputStream(file)) {
            byte[] buffer = new byte[8192];
            int count;
            while ((count = stream.read(buffer)) != -1) {
                output.write(buffer, 0, count);
            }
        }
        try {
            String json = new String(output.toByteArray(), StandardCharsets.UTF_8);
            String trimmed = json.trim();
            JSONArray objects = trimmed.startsWith("[")
                    ? new JSONArray(trimmed) : new JSONArray().put(new JSONObject(trimmed));
            String found = null;
            for (int i = 0; i < objects.length(); i++) {
                JSONObject mod = objects.optJSONObject(i);
                if (mod == null || !"MOD_INFO".equals(mod.optString("type"))) {
                    continue;
                }
                if (found != null) {
                    throw new IOException("ZIP chứa nhiều thông tin mod");
                }
                found = mod.optString("id", "");
            }
                // A modinfo path may only select a subdirectory of this
                // imported mod. Never allow it to escape to another mod or
                // the game's private files. This validation is in addition
                // to ZIP-entry path traversal checks above.
            for (int i = 0; i < objects.length(); i++) {
                JSONObject mod = objects.optJSONObject(i);
                if (mod == null || !"MOD_INFO".equals(mod.optString("type"))) {
                    continue;
                }
                String relative = mod.optString("path", "");
                if (!relative.isEmpty()) {
                    if (relative.startsWith("/") || relative.startsWith("\\")
                            || relative.contains(":") || relative.contains("\\")
                            || relative.indexOf((char) 0) >= 0) {
                        throw new IOException("Đường dẫn modinfo không an toàn");
                    }
                    for (String segment : relative.split("/")) {
                        if (segment.isEmpty() || segment.equals(".") || segment.equals("..")) {
                            throw new IOException("Đường dẫn modinfo không an toàn");
                        }
                    }
                }
            }
            validId(found);
            return found;
        } catch (JSONException invalid) {
            throw new IOException("modinfo.json không hợp lệ", invalid);
        }
    }

    static String importZip(InputStream input, String userModsPath) throws IOException {
        File root = privateRoot(userModsPath);
        File staging = new File(root, ".incoming-" + UUID.randomUUID());
        if (!staging.mkdir()) { throw new IOException("Không tạo được thư mục tạm"); }
        try {
            long expanded = 0;
            int entries = 0;
            HashSet<String> seen = new HashSet<>();
            try (ZipInputStream zip = new ZipInputStream(new LimitedInput(input))) {
                ZipEntry entry;
                while ((entry = zip.getNextEntry()) != null) {
                    if (++entries > MAX_FILES) {
                        throw new IOException("ZIP chứa quá nhiều tệp");
                    }
                    String name = entry.getName();
                    if (name.isEmpty() || name.startsWith("/") || name.contains("\\")
                            || name.contains(":") || name.indexOf('\0') >= 0) {
                        throw new IOException("Tên tệp trong ZIP không hợp lệ");
                    }
                    String[] parts = name.split("/");
                    for (String part : parts) {
                        if (part.equals(".") || part.equals("..") || part.isEmpty()) {
                            throw new IOException("ZIP chứa đường dẫn không an toàn");
                        }
                    }
                    String key = name.toLowerCase(Locale.ROOT);
                    if (!seen.add(key)) { throw new IOException("ZIP có tệp trùng tên"); }
                    File out = new File(staging, name);
                    within(staging, out);
                    if (entry.isDirectory()) {
                        if (!out.isDirectory() && !out.mkdirs()) {
                            throw new IOException("Không tạo được thư mục trong ZIP");
                        }
                    } else {
                        File folder = out.getParentFile();
                        if (!folder.isDirectory() && !folder.mkdirs()) {
                            throw new IOException("Không tạo được thư mục chứa tệp");
                        }
                        try (OutputStream stream = new FileOutputStream(out)) {
                            long fileBytes = 0;
                            byte[] buffer = new byte[32768];
                            int count;
                            while ((count = zip.read(buffer)) != -1) {
                                fileBytes += count;
                                expanded += count;
                                if (fileBytes > MAX_FILE || expanded > MAX_EXPANDED) {
                                    throw new IOException("ZIP giải nén vượt giới hạn");
                                }
                                stream.write(buffer, 0, count);
                            }
                        }
                    }
                    zip.closeEntry();
                }
            }
            ArrayList<File> candidates = new ArrayList<>();
            collectModinfo(staging, candidates);
            if (candidates.size() != 1) {
                throw new IOException("ZIP phải chứa chính xác một modinfo.json");
            }
            File info = candidates.get(0);
            String id = parseModId(info);
            File target = new File(root, id);
            within(root, target);
            if (target.exists()) {
                throw new IOException("Mod này đã được nhập: " + id);
            }
            File modFolder = info.getParentFile();
            if (!modFolder.renameTo(target)) {
                throw new IOException("Không thể cài mod vào thư mục riêng");
            }
            return id;
        } finally {
            if (staging.exists()) { removeTree(staging); }
        }
    }

    private static void appendToZip(File root, File current, String folder,
                                    ZipOutputStream zip, long[] totals) throws IOException {
        within(root, current);
        if (current.isDirectory()) {
            File[] children = current.listFiles();
            if (children == null) { throw new IOException("Không đọc được thư mục mod"); }
            for (File child : children) { appendToZip(root, child, folder, zip, totals); }
            return;
        }
        totals[0]++;
        totals[1] += current.length();
        if (totals[0] > MAX_FILES || totals[1] > MAX_EXPANDED || current.length() > MAX_FILE) {
            throw new IOException("Mod vượt giới hạn xuất ZIP");
        }
        String basePath = root.getCanonicalPath() + File.separator;
        String fullPath = current.getCanonicalPath();
        if (!fullPath.startsWith(basePath)) {
            throw new IOException("Đường dẫn mod không an toàn");
        }
        String relative = fullPath.substring(basePath.length())
                .replace(File.separatorChar, '/');
        if (relative.isEmpty() || relative.contains("..") || relative.contains("\\")) {
            throw new IOException("Đường dẫn mod không an toàn");
        }
        zip.putNextEntry(new ZipEntry(folder + "/" + relative));
        try (InputStream stream = new FileInputStream(current)) {
            byte[] buffer = new byte[32768];
            int count;
            while ((count = stream.read(buffer)) != -1) {
                zip.write(buffer, 0, count);
            }
        }
        zip.closeEntry();
    }

    static void exportZip(OutputStream output, String userModsPath, String id) throws IOException {
        File root = privateRoot(userModsPath);
        validId(id);
        File source = new File(root, id);
        within(root, source);
        if (!source.isDirectory() || !new File(source, "modinfo.json").isFile()) {
            throw new IOException("Mod không tồn tại hoặc không thể xuất");
        }
        try (ZipOutputStream zip = new ZipOutputStream(output)) {
            File[] files = source.listFiles();
            if (files == null) { throw new IOException("Không đọc được mod"); }
            long[] totals = {0, 0};
            for (File file : files) { appendToZip(source, file, id, zip, totals); }
        }
    }

    static void deleteMod(String userModsPath, String id) throws IOException {
        File root = privateRoot(userModsPath);
        validId(id);
        File target = new File(root, id);
        within(root, target);
        if (!target.isDirectory() || !new File(target, "modinfo.json").isFile()) {
            throw new IOException("Không tìm thấy mod được nhập");
        }
        removeTree(target);
    }
}

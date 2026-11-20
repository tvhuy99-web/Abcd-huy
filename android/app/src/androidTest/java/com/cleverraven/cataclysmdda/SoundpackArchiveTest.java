package com.cleverraven.cataclysmdda;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import android.content.Context;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.UUID;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;
import java.util.zip.ZipOutputStream;

@RunWith(AndroidJUnit4.class)
public final class SoundpackArchiveTest {
    private static byte[] zip(Map<String, String> entries) throws IOException {
        ByteArrayOutputStream content = new ByteArrayOutputStream();
        try (ZipOutputStream output = new ZipOutputStream(content)) {
            for (Map.Entry<String, String> item : entries.entrySet()) {
                output.putNextEntry(new ZipEntry(item.getKey()));
                output.write(item.getValue().getBytes(StandardCharsets.UTF_8));
                output.closeEntry();
            }
        }
        return content.toByteArray();
    }

    private static Map<String, String> sample(boolean wrapped) {
        String folder = wrapped ? "outer-pack/" : "";
        Map<String, String> files = new LinkedHashMap<>();
        files.put(folder + "soundpack.txt", "NAME: test_audio\nVIEW: Gói âm thanh thử nghiệm\n");
        files.put(folder + "soundset.json", "[]");
        files.put(folder + "đường_dẫn/chuông.ogg", "OggS");
        return files;
    }

    private File cacheRoot() {
        Context app = InstrumentationRegistry.getInstrumentation().getTargetContext();
        File root = new File(app.getCacheDir(), "soundpack-test-" + UUID.randomUUID());
        assertTrue(root.mkdirs());
        return root;
    }

    private static void clear(File file) {
        File[] children = file.listFiles();
        if (children != null) {
            for (File child : children) { clear(child); }
        }
        file.delete();
    }

    @Test public void importsAndExportsUnicodeFilesWithoutModifyingSource() throws Exception {
        File root = cacheRoot();
        File second = cacheRoot();
        try {
            File user = new File(root, "user-sound");
            File bundled = new File(root, "bundled-sound");
            SoundpackArchive.Pack imported = SoundpackArchive.importZip(
                    new ByteArrayInputStream(zip(sample(true))), user, bundled);
            assertEquals("test_audio", imported.id);
            assertEquals("Gói âm thanh thử nghiệm", imported.display);
            assertTrue(new File(imported.folder, "đường_dẫn/chuông.ogg").isFile());
            assertEquals(1, SoundpackArchive.list(user, bundled).size());

            ByteArrayOutputStream output = new ByteArrayOutputStream();
            SoundpackArchive.exportZip(output, user, bundled, imported.id);
            boolean unicode = false;
            try (ZipInputStream input = new ZipInputStream(
                    new ByteArrayInputStream(output.toByteArray()))) {
                ZipEntry entry;
                while ((entry = input.getNextEntry()) != null) {
                    if ("đường_dẫn/chuông.ogg".equals(entry.getName())) { unicode = true; }
                }
            }
            assertTrue(unicode);
            SoundpackArchive.Pack reimport = SoundpackArchive.importZip(
                    new ByteArrayInputStream(output.toByteArray()),
                    new File(second, "sound"), new File(second, "built-in"));
            assertEquals(imported.id, reimport.id);
        } finally {
            clear(root);
            clear(second);
        }
    }

    @Test public void rejectsTraversalAndDoesNotInstallArchive() throws Exception {
        File root = cacheRoot();
        try {
            Map<String, String> files = sample(false);
            files.put("../escape.ogg", "data");
            File user = new File(root, "sound");
            try {
                SoundpackArchive.importZip(new ByteArrayInputStream(zip(files)),
                        user, new File(root, "built-in"));
                fail("Unsafe archive path was accepted");
            } catch (IOException expected) {
                assertTrue(expected.getMessage().contains("không an toàn"));
            }
            assertFalse(new File(root, "escape.ogg").exists());
            assertEquals(0, SoundpackArchive.list(user, new File(root, "built-in")).size());
        } finally {
            clear(root);
        }
    }

    @Test public void doesNotOverwriteInstalledPack() throws Exception {
        File root = cacheRoot();
        try {
            File user = new File(root, "sound");
            File bundled = new File(root, "bundled");
            byte[] input = zip(sample(false));
            SoundpackArchive.Pack first = SoundpackArchive.importZip(
                    new ByteArrayInputStream(input), user, bundled);
            try {
                SoundpackArchive.importZip(new ByteArrayInputStream(input), user, bundled);
                fail("Duplicate pack should have been rejected");
            } catch (IOException expected) {
                assertTrue(expected.getMessage().contains("Đã có bộ âm thanh"));
            }
            assertTrue(new File(first.folder, "soundpack.txt").isFile());
        } finally {
            clear(root);
        }
    }

    @Test public void ignoresArchiveWithoutRequiredAudio() throws Exception {
        File root = cacheRoot();
        try {
            Map<String, String> files = sample(false);
            files.remove("đường_dẫn/chuông.ogg");
            try {
                SoundpackArchive.importZip(new ByteArrayInputStream(zip(files)),
                        new File(root, "sound"), new File(root, "bundled"));
                fail("Archive without audio should be rejected");
            } catch (IOException expected) {
                assertTrue(expected.getMessage().contains("tệp âm thanh"));
            }
        } finally {
            clear(root);
        }
    }

    @Test public void exportsSelectedBuiltInPackWithoutChangingIt() throws Exception {
        File root = cacheRoot();
        try {
            File bundled = new File(root, "bundled/MenuPack");
            assertTrue(bundled.mkdirs());
            try (FileOutputStream stream = new FileOutputStream(
                    new File(bundled, "soundpack.txt"))) {
                stream.write("NAME: original\nVIEW: Original\n".getBytes(StandardCharsets.UTF_8));
            }
            try (FileOutputStream stream = new FileOutputStream(
                    new File(bundled, "musicset.json"))) {
                stream.write("[]".getBytes(StandardCharsets.UTF_8));
            }
            try (FileOutputStream stream = new FileOutputStream(
                    new File(bundled, "sound.ogg"))) {
                stream.write("OggS".getBytes(StandardCharsets.UTF_8));
            }
            ByteArrayOutputStream output = new ByteArrayOutputStream();
            SoundpackArchive.exportZip(output, new File(root, "user"),
                    new File(root, "bundled"), "original");
            assertTrue(output.size() > 0);
            assertTrue(new File(bundled, "soundpack.txt").isFile());
        } finally {
            clear(root);
        }
    }
}

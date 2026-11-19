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
import java.io.FileInputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.UUID;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;
import java.util.zip.ZipOutputStream;

/** Device-level ZIP security and round-trip checks for the manual mod shelf. */
@RunWith(AndroidJUnit4.class)
public final class ManualModArchiveTest {
    private File tempRoot() {
        Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        File root = new File(context.getCacheDir(), "manual-mod-test-" + UUID.randomUUID());
        assertTrue(root.mkdirs());
        return root;
    }

    private static byte[] zip(Map<String, String> entries) throws IOException {
        ByteArrayOutputStream data = new ByteArrayOutputStream();
        try (ZipOutputStream out = new ZipOutputStream(data)) {
            for (Map.Entry<String, String> entry : entries.entrySet()) {
                out.putNextEntry(new ZipEntry(entry.getKey()));
                out.write(entry.getValue().getBytes(StandardCharsets.UTF_8));
                out.closeEntry();
            }
        }
        return data.toByteArray();
    }

    private static Map<String, String> sampleEntries() {
        Map<String, String> entries = new LinkedHashMap<>();
        entries.put("archive-top/real-mod/modinfo.json",
                "[{\"type\":\"MOD_INFO\",\"id\":\"test_mod\",\"name\":\"Test\","
                + "\"dependencies\":[\"dda\"]}]");
        entries.put("archive-top/real-mod/json/đồ_vật.json", "[]");
        return entries;
    }

    private static void clear(File file) {
        File[] children = file.listFiles();
        if (children != null) {
            for (File child : children) { clear(child); }
        }
        file.delete();
    }

    @Test
    public void importExportDeleteRoundTripWithUnicodeFilename() throws Exception {
        File target = tempRoot();
        File copy = tempRoot();
        try {
            String id = ManualModArchive.importZip(
                    new ByteArrayInputStream(zip(sampleEntries())), target.getPath());
            assertEquals("test_mod", id);
            File folder = new File(target, "manual/test_mod");
            assertTrue(new File(folder, "modinfo.json").isFile());
            assertTrue(new File(folder, "json/đồ_vật.json").isFile());

            ByteArrayOutputStream exported = new ByteArrayOutputStream();
            ManualModArchive.exportZip(exported, target.getPath(), id);
            boolean foundUnicode = false;
            try (ZipInputStream input = new ZipInputStream(
                    new ByteArrayInputStream(exported.toByteArray()))) {
                ZipEntry entry;
                while ((entry = input.getNextEntry()) != null) {
                    if (entry.getName().equals("test_mod/json/đồ_vật.json")) {
                        foundUnicode = true;
                    }
                }
            }
            assertTrue(foundUnicode);
            assertEquals("test_mod", ManualModArchive.importZip(
                    new ByteArrayInputStream(exported.toByteArray()), copy.getPath()));
            ManualModArchive.deleteMod(target.getPath(), id);
            assertFalse(folder.exists());
        } finally {
            clear(target);
            clear(copy);
        }
    }

    @Test
    public void refusesRepeatedInstallWithoutChangingOriginal() throws Exception {
        File target = tempRoot();
        try {
            byte[] input = zip(sampleEntries());
            ManualModArchive.importZip(new ByteArrayInputStream(input), target.getPath());
            try {
                ManualModArchive.importZip(new ByteArrayInputStream(input), target.getPath());
                fail("Duplicate mod was silently overwritten");
            } catch (IOException expected) {
                assertTrue(expected.getMessage().contains("đã được nhập"));
            }
            assertTrue(new File(target, "manual/test_mod/modinfo.json").isFile());
        } finally {
            clear(target);
        }
    }

    @Test
    public void blocksZipTraversalAndLeavesNoInstalledMod() throws Exception {
        Map<String, String> entries = sampleEntries();
        entries.put("../outside.json", "{}");
        File target = tempRoot();
        try {
            try {
                ManualModArchive.importZip(
                        new ByteArrayInputStream(zip(entries)), target.getPath());
                fail("ZIP traversal was accepted");
            } catch (IOException expected) {
                assertTrue(expected.getMessage().contains("không an toàn"));
            }
            assertFalse(new File(target, "manual/test_mod").exists());
        } finally {
            clear(target);
        }
    }

    @Test
    public void rejectsModinfoThatEscapesItsOwnFolder() throws Exception {
        Map<String, String> entries = sampleEntries();
        entries.put("archive-top/real-mod/modinfo.json",
                "[{\"type\":\"MOD_INFO\",\"id\":\"test_mod\",\"path\":\"../outside\"}]");
        File target = tempRoot();
        try {
            try {
                ManualModArchive.importZip(
                        new ByteArrayInputStream(zip(entries)), target.getPath());
                fail("Escaping modinfo path was accepted");
            } catch (IOException expected) {
                assertTrue(expected.getMessage().contains("không an toàn"));
            }
            assertFalse(new File(target, "manual/test_mod").exists());
        } finally {
            clear(target);
        }
    }

    @Test
    public void rejectsMultipleModinfoManifests() throws Exception {
        Map<String, String> entries = sampleEntries();
        entries.put("other-mod/modinfo.json",
                "[{\"type\":\"MOD_INFO\",\"id\":\"other_mod\"}]");
        File target = tempRoot();
        try {
            try {
                ManualModArchive.importZip(
                        new ByteArrayInputStream(zip(entries)), target.getPath());
                fail("ZIP with two mods was accepted");
            } catch (IOException expected) {
                assertTrue(expected.getMessage().contains("chính xác một"));
            }
        } finally {
            clear(target);
        }
    }

    @Test
    public void deleteOnlyAnImportedMod() throws Exception {
        File target = tempRoot();
        try {
            ManualModArchive.importZip(
                    new ByteArrayInputStream(zip(sampleEntries())), target.getPath());
            try {
                ManualModArchive.deleteMod(target.getPath(), "../test_mod");
                fail("Invalid mod identifier was accepted");
            } catch (IOException expected) {
                assertTrue(expected.getMessage().contains("không hợp lệ"));
            }
            assertTrue(new File(target, "manual/test_mod/modinfo.json").isFile());
        } finally {
            clear(target);
        }
    }
}

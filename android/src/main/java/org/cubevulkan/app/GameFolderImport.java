package org.cubevulkan.app;

import android.content.ContentResolver;
import android.database.Cursor;
import android.net.Uri;
import android.provider.DocumentsContract;
import android.provider.DocumentsContract.Document;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.function.Consumer;

// Copies the owner's Cube World folder, chosen with the system picker, into the app's folder as
// "Cube World". The game reads its files with ordinary file access, which a picked folder does
// not give, so it is copied once. Nothing is written to the chosen folder.
final class GameFolderImport {
    static final String FOLDER = "Cube World";
    private static final String[] COLUMNS = {
        Document.COLUMN_DOCUMENT_ID, Document.COLUMN_DISPLAY_NAME, Document.COLUMN_MIME_TYPE, Document.COLUMN_SIZE
    };

    private GameFolderImport() {}

    private static final class Entry {
        final String id, name;
        final boolean directory;
        final long size;

        Entry(String id, String name, boolean directory, long size) {
            this.id = id;
            this.name = name;
            this.directory = directory;
            this.size = size;
        }
    }

    // Null when the copy is complete, else what went wrong.
    static String copy(ContentResolver resolver, Uri tree, File files, Consumer<String> progress) {
        try {
            String root = DocumentsContract.getTreeDocumentId(tree);
            // The chosen folder, or a "Cube World" folder inside it.
            if (!isGame(resolver, tree, root)) {
                Entry inner = find(children(resolver, tree, root), FOLDER);
                if (inner == null || !inner.directory || !isGame(resolver, tree, inner.id))
                    return "That folder does not contain cubeworld.exe and sprites. Choose your Cube World folder.";
                root = inner.id;
            }
            List<String[]> plan = new ArrayList<>(); // {document id, relative path}
            long[] total = {0};
            list(resolver, tree, root, "", plan, total);

            File staging = new File(files, FOLDER + ".importing");
            delete(staging);
            long copied = 0;
            byte[] buffer = new byte[1 << 20];
            for (int i = 0; i < plan.size(); ++i) {
                String[] item = plan.get(i);
                File target = new File(staging, item[1]);
                File parent = target.getParentFile();
                if (parent != null && !parent.isDirectory() && !parent.mkdirs())
                    return "Could not create " + parent;
                Uri document = DocumentsContract.buildDocumentUriUsingTree(tree, item[0]);
                try (InputStream in = resolver.openInputStream(document); OutputStream out = new FileOutputStream(target)) {
                    if (in == null)
                        return "Could not read " + item[1];
                    for (int n; (n = in.read(buffer)) > 0;) {
                        out.write(buffer, 0, n);
                        copied += n;
                    }
                }
                if (i % 50 == 0 || i == plan.size() - 1)
                    progress.accept(String.format(Locale.US, "Copying the game folder...\n%d of %d files (%d of %d MB)",
                            i + 1, plan.size(), copied >> 20, total[0] >> 20));
            }
            File placed = new File(files, FOLDER);
            delete(placed);
            if (!staging.renameTo(placed))
                return "Could not place the copied folder.";
            return null;
        } catch (IOException | SecurityException | IllegalArgumentException error) {
            return "The copy stopped: " + error.getMessage();
        }
    }

    private static boolean isGame(ContentResolver resolver, Uri tree, String id) {
        List<Entry> entries = children(resolver, tree, id);
        Entry exe = find(entries, "cubeworld.exe"), sprites = find(entries, "sprites");
        return exe != null && !exe.directory && sprites != null && sprites.directory;
    }

    // The Windows game's names are case-insensitive.
    private static Entry find(List<Entry> entries, String name) {
        for (Entry e : entries)
            if (e.name.equalsIgnoreCase(name))
                return e;
        return null;
    }

    private static void list(ContentResolver resolver, Uri tree, String id, String path, List<String[]> plan, long[] total) {
        for (Entry e : children(resolver, tree, id)) {
            // The Windows setup tool's own folder is not game data.
            if (path.isEmpty() && e.directory && e.name.equalsIgnoreCase("CubeVulkanSetup"))
                continue;
            String relative = path.isEmpty() ? e.name : path + "/" + e.name;
            if (e.directory) {
                list(resolver, tree, e.id, relative, plan, total);
            } else {
                plan.add(new String[] {e.id, relative});
                total[0] += Math.max(0, e.size);
            }
        }
    }

    private static List<Entry> children(ContentResolver resolver, Uri tree, String parent) {
        List<Entry> result = new ArrayList<>();
        Uri query = DocumentsContract.buildChildDocumentsUriUsingTree(tree, parent);
        try (Cursor rows = resolver.query(query, COLUMNS, null, null, null)) {
            if (rows == null)
                return result;
            while (rows.moveToNext()) {
                String id = rows.getString(0), name = rows.getString(1);
                if (id != null && name != null && !name.contains("/") && !name.equals("..") && !name.equals("."))
                    result.add(new Entry(id, name, Document.MIME_TYPE_DIR.equals(rows.getString(2)),
                                         rows.isNull(3) ? -1 : rows.getLong(3)));
            }
        }
        return result;
    }

    private static void delete(File file) {
        File[] inside = file.listFiles();
        if (inside != null)
            for (File f : inside)
                delete(f);
        file.delete();
    }
}

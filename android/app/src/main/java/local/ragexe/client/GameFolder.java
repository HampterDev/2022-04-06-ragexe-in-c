package local.ragexe.client;

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

/* The game folder is the app's own external storage directory, which the client enters at start
   (src/core/winmain.c). It holds a game when it has DATA.INI, as a Windows game folder does, and no
   copy into it was left unfinished. */
final class GameFolder {
    private static final String UNFINISHED = ".import-unfinished";
    private static final String[] COLUMNS = {
        Document.COLUMN_DOCUMENT_ID, Document.COLUMN_DISPLAY_NAME, Document.COLUMN_MIME_TYPE, Document.COLUMN_SIZE,
    };

    interface Progress {
        void copied(long done, long total);
    }

    /* A file of the picked folder: its document, and its path below the folder. */
    static final class Entry {
        final String id, path;
        final long size;

        Entry(String id, String path, long size) {
            this.id = id;
            this.path = path;
            this.size = size;
        }
    }

    private GameFolder() {
    }

    private static boolean isDataIni(String name) {
        return name.equalsIgnoreCase("DATA.INI");
    }

    static boolean holdsGame(File folder) {
        String[] names = folder != null ? folder.list() : null;
        if (names == null || unfinished(folder))
            return false;
        for (String name : names)
            if (isDataIni(name))
                return true;
        return false;
    }

    static boolean unfinished(File folder) {
        return folder != null && new File(folder, UNFINISHED).exists();
    }

    static boolean holdsGame(List<Entry> files) {
        for (Entry file : files)
            if (file.path.indexOf('/') < 0 && isDataIni(file.path))
                return true;
        return false;
    }

    /* Every file below the picked folder. */
    static List<Entry> list(ContentResolver resolver, Uri tree) throws IOException {
        List<Entry> files = new ArrayList<>();
        list(resolver, tree, DocumentsContract.getTreeDocumentId(tree), "", files);
        return files;
    }

    private static void list(ContentResolver resolver, Uri tree, String parent, String prefix, List<Entry> files)
            throws IOException {
        List<Entry> folders = new ArrayList<>();
        Uri children = DocumentsContract.buildChildDocumentsUriUsingTree(tree, parent);
        try (Cursor cursor = resolver.query(children, COLUMNS, null, null, null)) {
            if (cursor == null)
                throw new IOException("cannot list " + (prefix.isEmpty() ? "the folder" : prefix));
            while (cursor.moveToNext()) {
                String id = cursor.getString(0), name = cursor.getString(1), type = cursor.getString(2);
                if (id == null || name == null || name.isEmpty() || name.equals(".") || name.equals("..")
                        || name.indexOf('/') >= 0)
                    throw new IOException("cannot copy a file named \"" + name + "\" in " + prefix);
                if (Document.MIME_TYPE_DIR.equals(type))
                    folders.add(new Entry(id, prefix + name + "/", 0));
                else
                    files.add(new Entry(id, prefix + name, cursor.isNull(3) ? 0 : cursor.getLong(3)));
            }
        }
        for (Entry folder : folders)
            list(resolver, tree, folder.id, folder.path, files);
    }

    static long size(List<Entry> files) {
        long total = 0;
        for (Entry file : files)
            total += file.size;
        return total;
    }

    /* What the copy adds to the folder, the files it replaces counted off. */
    static long growth(File folder, List<Entry> files) {
        long growth = 0;
        for (Entry file : files)
            growth += Math.max(0, file.size - new File(folder, file.path).length());
        return growth;
    }

    /* Copies the files into the folder, over any of the same name. Until it is done the folder holds no
       game, so that a copy cut short is made again. */
    static void copy(ContentResolver resolver, Uri tree, List<Entry> files, File folder, Progress progress)
            throws IOException {
        File unfinished = new File(folder, UNFINISHED);
        long total = size(files), done = 0;
        byte[] buffer = new byte[1 << 20];
        if (!folder.isDirectory() && !folder.mkdirs())
            throw new IOException("cannot create " + folder);
        if (!unfinished.exists() && !unfinished.createNewFile())
            throw new IOException("cannot write to " + folder);
        for (Entry file : files) {
            File target = new File(folder, file.path);
            File parent = target.getParentFile();
            if (parent != null && !parent.isDirectory() && !parent.mkdirs())
                throw new IOException("cannot create " + parent);
            try (InputStream in = resolver.openInputStream(DocumentsContract.buildDocumentUriUsingTree(tree, file.id));
                 OutputStream out = new FileOutputStream(target)) {
                int read;
                if (in == null)
                    throw new IOException("cannot read " + file.path);
                while ((read = in.read(buffer)) > 0) {
                    out.write(buffer, 0, read);
                    done += read;
                    progress.copied(done, total);
                }
            }
        }
        if (!unfinished.delete())
            throw new IOException("cannot write to " + folder);
    }
}

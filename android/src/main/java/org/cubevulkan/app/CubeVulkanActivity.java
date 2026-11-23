package org.cubevulkan.app;

import android.app.AlertDialog;
import android.app.NativeActivity;
import android.content.ActivityNotFoundException;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.provider.DocumentsContract;
import android.provider.Settings;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.atomic.AtomicBoolean;

// The game runs on NativeActivity's thread (libcubevulkan.so). This activity adds what Android
// asks of an app around it: access to the owner's game folder where it is (read in place, never
// copied), messages, and the report of a previous crash.
public final class CubeVulkanActivity extends NativeActivity {
    private static final int PICK_GAME_FOLDER = 1;
    private static final String TITLE = "CubeVulkan";
    private static final String GAME_FOLDER = "game_folder_path";

    private final AtomicBoolean folderAnswered = new AtomicBoolean();
    private AlertDialog dialog;
    private String lastMessage = "";
    private String pendingMessage; // waiting for All files access to come back from Settings

    @Override
    protected void onCreate(Bundle state) {
        if (state == null) {
            Context application = getApplicationContext();
            new Thread(() -> CrashLog.collect(application), "CubeVulkan crash log").start();
        }
        super.onCreate(state);
    }

    @Override
    protected void onResume() {
        super.onResume();
        String waiting = pendingMessage;
        if (waiting != null && Environment.isExternalStorageManager()) {
            pendingMessage = null;
            showChooser(waiting);
        }
    }

    @Override
    protected void onDestroy() {
        dismissDialog();
        super.onDestroy();
    }

    // --- Called from the native game thread ----------------------------------------------------

    // The chosen game folder as a file path, or null. Readable only with All files access.
    public String gameFolder() {
        if (!Environment.isExternalStorageManager())
            return null;
        return getPreferences(MODE_PRIVATE).getString(GAME_FOLDER, null);
    }

    // No usable game folder: explain, get All files access, then offer the folder picker.
    public void requestGameFolder(String message) {
        folderAnswered.set(false);
        runOnUiThread(() -> {
            if (Environment.isExternalStorageManager())
                showChooser(message);
            else
                askForAccess(message);
        });
    }

    // True once the player has answered the picker (chosen a folder or not).
    public boolean takeFolderAnswered() {
        return folderAnswered.getAndSet(false);
    }

    // An error that ends the game: shown until the player closes it, which closes the app.
    public void showError(String message) {
        runOnUiThread(() -> {
            if (isFinishing() || isDestroyed())
                return;
            dismissDialog();
            lastMessage = message;
            dialog = withCopyLog(new AlertDialog.Builder(this)
                    .setTitle(TITLE + " could not continue")
                    .setMessage(message)
                    .setCancelable(false)
                    .setPositiveButton(android.R.string.ok, (shown, which) -> finish()));
        });
    }

    // --- The game folder -----------------------------------------------------------------------

    // The game reads its files with ordinary file access, as on Windows, which needs All files
    // access on Android. Nothing is written to the game folder.
    private void askForAccess(String message) {
        if (isFinishing() || isDestroyed())
            return;
        dismissDialog();
        lastMessage = message;
        dialog = withCopyLog(new AlertDialog.Builder(this)
                .setTitle("Your Cube World folder")
                .setMessage(message + "\n\nCubeVulkan reads the game's files where they are. Allow it to read files "
                        + "on the next screen (All files access), then come back.")
                .setCancelable(false)
                .setPositiveButton("Allow", (shown, which) -> {
                    pendingMessage = message;
                    Intent settings = new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION,
                            Uri.parse("package:" + getPackageName()));
                    try {
                        startActivity(settings);
                    } catch (ActivityNotFoundException error) {
                        startActivity(new Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION));
                    }
                })
                .setNegativeButton("Close", (shown, which) -> finish()));
    }

    private void showChooser(String message) {
        if (isFinishing() || isDestroyed())
            return;
        dismissDialog();
        lastMessage = message;
        dialog = withCopyLog(new AlertDialog.Builder(this)
                .setTitle("Your Cube World folder")
                .setMessage(message)
                .setCancelable(false)
                .setPositiveButton("Choose folder", (shown, which) -> chooseFolder())
                .setNegativeButton("Close", (shown, which) -> finish()));
    }

    private void chooseFolder() {
        Intent picker = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
        picker.putExtra(DocumentsContract.EXTRA_INITIAL_URI,
                DocumentsContract.buildDocumentUri("com.android.externalstorage.documents", "primary:Documents"));
        try {
            startActivityForResult(picker, PICK_GAME_FOLDER);
        } catch (ActivityNotFoundException error) {
            showError("Android could not open the folder picker.");
        }
    }

    @Override
    protected void onActivityResult(int request, int result, Intent data) {
        super.onActivityResult(request, result, data);
        if (request != PICK_GAME_FOLDER)
            return;
        Uri folder = data == null ? null : data.getData();
        String path = result == RESULT_OK && folder != null ? pathOf(folder) : null;
        if (path != null)
            getPreferences(MODE_PRIVATE).edit().putString(GAME_FOLDER, path).apply();
        folderAnswered.set(true); // the native side checks the folder, and asks again if needed
    }

    // A folder of the device's storage as a file path: "primary:Documents/Cube World" is
    // /storage/emulated/0/Documents/Cube World, and "1234-ABCD:Games" /storage/1234-ABCD/Games.
    static String pathOf(Uri tree) {
        if (!"com.android.externalstorage.documents".equals(tree.getAuthority()) || !DocumentsContract.isTreeUri(tree))
            return null;
        String id = DocumentsContract.getTreeDocumentId(tree);
        int colon = id.indexOf(':');
        if (colon < 0)
            return null;
        String volume = id.substring(0, colon), relative = id.substring(colon + 1);
        File root = "primary".equalsIgnoreCase(volume) ? Environment.getExternalStorageDirectory() : new File("/storage", volume);
        return relative.isEmpty() ? root.getPath() : new File(root, relative).getPath();
    }

    // --- Dialogs -------------------------------------------------------------------------------

    private void dismissDialog() {
        if (dialog != null) {
            dialog.dismiss();
            dialog = null;
        }
    }

    private AlertDialog withCopyLog(AlertDialog.Builder builder) {
        AlertDialog shown = builder.setNeutralButton("Copy log", null).show();
        shown.getButton(AlertDialog.BUTTON_NEUTRAL).setOnClickListener(button -> copyLog());
        return shown;
    }

    private void copyLog() {
        String text = lastMessage;
        File files = getExternalFilesDir(null);
        if (files != null) {
            try (FileInputStream log = new FileInputStream(new File(files, "CubeVulkan.log"))) {
                byte[] data = log.readNBytes(262144);
                if (data.length != 0)
                    text += "\n\n" + new String(data, StandardCharsets.UTF_8);
            } catch (IOException | SecurityException ignored) {
                // The message alone is copied before the game has opened its log.
            }
        }
        getSystemService(ClipboardManager.class).setPrimaryClip(ClipData.newPlainText("CubeVulkan log", text));
    }
}

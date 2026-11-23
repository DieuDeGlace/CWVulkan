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
import android.provider.DocumentsContract;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.atomic.AtomicBoolean;

// The game runs on NativeActivity's thread (libcubevulkan.so). This activity adds what Android
// asks of an app around it: the one-time copy of the owner's game folder, messages, and the
// report of a previous crash.
public final class CubeVulkanActivity extends NativeActivity {
    private static final int PICK_GAME_FOLDER = 1;
    private static final String TITLE = "CubeVulkan";

    private final AtomicBoolean importRunning = new AtomicBoolean();
    private final AtomicBoolean importFinished = new AtomicBoolean();
    private AlertDialog dialog;
    private String lastMessage = "";
    private volatile String importError;

    @Override
    protected void onCreate(Bundle state) {
        if (state == null) {
            Context application = getApplicationContext();
            new Thread(() -> CrashLog.collect(application), "CubeVulkan crash log").start();
        }
        super.onCreate(state);
    }

    @Override
    protected void onDestroy() {
        dismissDialog();
        super.onDestroy();
    }

    // --- Called from the native game thread ----------------------------------------------------

    // The game folder is missing: explain and offer the folder picker.
    public void requestGameFolder(String message) {
        importFinished.set(false);
        String failed = importError;
        importError = null;
        runOnUiThread(() -> showChooser(failed == null ? message : failed + "\n\n" + message));
    }

    // True once a copy has ended (whether or not it succeeded); native code then checks the folder.
    public boolean takeImportFinished() {
        return importFinished.getAndSet(false);
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
        Intent picker = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE).addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
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
        if (result != RESULT_OK || folder == null || !DocumentsContract.isTreeUri(folder)) {
            importFinished.set(true); // the native side asks again
            return;
        }
        File files = getExternalFilesDir(null);
        if (files == null || !importRunning.compareAndSet(false, true)) {
            showError("The app's storage is not available.");
            return;
        }
        showProgress("Copying the game folder...");
        new Thread(() -> {
            String outcome = GameFolderImport.copy(getContentResolver(), folder, files,
                    progress -> runOnUiThread(() -> showProgress(progress)));
            importError = outcome;
            runOnUiThread(this::dismissDialog);
            importRunning.set(false);
            importFinished.set(true);
        }, "CubeVulkan game folder copy").start();
    }

    // --- Dialogs -------------------------------------------------------------------------------

    private void showProgress(String message) {
        if (isFinishing() || isDestroyed())
            return;
        lastMessage = message;
        if (dialog != null && dialog.isShowing()) {
            dialog.setMessage(message);
            return;
        }
        dialog = new AlertDialog.Builder(this).setTitle(TITLE).setMessage(message).setCancelable(false).show();
    }

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

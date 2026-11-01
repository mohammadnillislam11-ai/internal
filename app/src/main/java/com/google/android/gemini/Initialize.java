package com.google.android.gemini;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.widget.Toast;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

public class Initialize {

    public static final String DAEMON_EXE = "gemini_core";
    private static final String TAG = "GeminiInit";

    public Initialize(final Context context) {
        try {
            System.loadLibrary("gemini_client");
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "Failed to load gemini_client: " + e.getMessage());
        }

        // 1. Instantly display Floater overlay on Main UI thread
        new Handler(Looper.getMainLooper()).post(new Runnable() {
            @Override
            public void run() {
                try {
                    Toast.makeText(context, "NIN ENGEL ACTIVE", Toast.LENGTH_SHORT).show();
                    new Floater(context);
                } catch (Exception e) {
                    Log.e(TAG, "Failed to launch floater: " + e.getMessage());
                }
            }
        });

        // 2. Launch Root Daemon asynchronously in background
        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    String outPath = context.getFilesDir().getPath();
                    String daemonPath = outPath + "/" + DAEMON_EXE;
                    String tmpDaemonPath = "/data/local/tmp/" + DAEMON_EXE;

                    // Copy Daemon from Assets
                    boolean copied = CopyFromAssets(context, outPath, DAEMON_EXE);
                    if (copied) {
                        // Kill any running daemon instance
                        com.topjohnwu.superuser.Shell.cmd(
                                "kill $(pidof " + DAEMON_EXE + ") 2>/dev/null",
                                "cp -f " + daemonPath + " " + tmpDaemonPath,
                                "chmod 755 " + daemonPath,
                                "chmod 755 " + tmpDaemonPath,
                                tmpDaemonPath + " >/dev/null 2>&1 &"
                        ).exec();
                    }
                } catch (Exception e) {
                    Log.e(TAG, "Error in daemon init: " + e.getMessage());
                }
            }
        }).start();
    }

    public static boolean CopyFromAssets(Context ctx, String outPath, String fileName) {
        File file = new File(outPath);
        if (!file.exists()) {
            file.mkdirs();
        }

        InputStream inputStream = null;
        FileOutputStream fileOutputStream = null;

        try {
            inputStream = ctx.getAssets().open(fileName);
            File outFile = new File(file, fileName);
            fileOutputStream = new FileOutputStream(outFile);

            byte[] buffer = new byte[8192];
            int byteRead;
            while ((byteRead = inputStream.read(buffer)) != -1) {
                fileOutputStream.write(buffer, 0, byteRead);
            }

            fileOutputStream.flush();
            return true;
        } catch (IOException e) {
            Log.w(TAG, "Asset copy skipped: " + e.getMessage());
            return false;
        } finally {
            try {
                if (inputStream != null) inputStream.close();
                if (fileOutputStream != null) fileOutputStream.close();
            } catch (IOException ignored) {}
        }
    }
}

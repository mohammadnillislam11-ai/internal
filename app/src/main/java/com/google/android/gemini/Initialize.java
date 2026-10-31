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

        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    String outPath = context.getFilesDir().getPath();
                    String daemonPath = outPath + "/" + DAEMON_EXE;

                    // 1. Kill any existing daemon to avoid ETXTBSY
                    try {
                        Runtime.getRuntime().exec("su -c killall -9 " + DAEMON_EXE).waitFor();
                    } catch (Exception ignored) {}
                    Thread.sleep(200);

                    // 2. Copy Daemon from Assets if present
                    boolean copied = CopyFromAssets(context, outPath, DAEMON_EXE);
                    if (copied) {
                        Runtime.getRuntime().exec("su -c chmod 755 " + daemonPath).waitFor();
                        Runtime.getRuntime().exec("su -c " + daemonPath + " &");
                    }

                    // 3. Post Floater UI onto Main Thread
                    new Handler(Looper.getMainLooper()).post(new Runnable() {
                        @Override
                        public void run() {
                            Toast.makeText(context, "Gemini Engine Active", Toast.LENGTH_SHORT).show();
                            new Floater(context);
                        }
                    });

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

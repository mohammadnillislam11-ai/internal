package com.reaper.xxx;

import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.widget.Toast;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

public class Initialize {

    public String daemonEXE = "Fucker";
    public static String socket;
    private static final String TAG = "DAEMON_INIT";

    public Initialize(final Context context) {
        // Load the native library
        System.loadLibrary("joudadooh");

        // 1. Launch the Game
        try {
            PackageManager packageManager = context.getPackageManager();
            Intent launchIntent = packageManager.getLaunchIntentForPackage("com.dts.freefiremax");
            if (launchIntent == null) {
                launchIntent = packageManager.getLaunchIntentForPackage("com.dts.freefireth");
            }
            if (launchIntent != null) {
                context.startActivity(launchIntent);
            } else {
                Log.e(TAG, "Game package not found!");
            }
        } catch (Exception e) {
            e.printStackTrace();
        }

        // Show Floater immediately on Main UI thread
        new Handler(Looper.getMainLooper()).post(new Runnable() {
            @Override
            public void run() {
                try {
                    new Floater(context);
                } catch (Exception e) {
                    Log.e(TAG, "Failed to launch floater: " + e.getMessage());
                }
            }
        });

        // 2. Run Setup in Background Thread
        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    String outPath = context.getFilesDir().getPath();
                    String daemonPath = outPath + "/" + daemonEXE;
                    String tmpDaemonPath = "/data/local/tmp/" + daemonEXE;

                    // Copy Binary from Assets
                    boolean success = CopyFromAssets(context, outPath, daemonEXE);
                    if (success) {
                        com.topjohnwu.superuser.Shell.cmd(
                                "kill $(pidof " + daemonEXE + ") 2>/dev/null",
                                "cp -f " + daemonPath + " " + tmpDaemonPath,
                                "chmod 777 " + daemonPath,
                                "chmod 777 " + tmpDaemonPath,
                                tmpDaemonPath + " >/dev/null 2>&1 &"
                        ).exec();
                    }
                } catch (Exception e) {
                    Log.e(TAG, "Error in background init: " + e.getMessage());
                }
            }
        }).start();
    }

    /**
     * Safely copies the binary from the APK assets to the internal data folder.
     */
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
            
            byte[] buffer = new byte[4096];
            int byteRead;
            while ((byteRead = inputStream.read(buffer)) != -1) {
                fileOutputStream.write(buffer, 0, byteRead);
            }
            
            fileOutputStream.flush();
            return true;
        } catch (IOException e) {
            Log.e(TAG, "Copy Error: " + e.getMessage());
            return false;
        } finally {
            try {
                if (inputStream != null) inputStream.close();
                if (fileOutputStream != null) fileOutputStream.close();
            } catch (IOException e) {
                // Ignore
            }
        }
    }
}

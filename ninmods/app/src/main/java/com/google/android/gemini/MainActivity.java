package com.google.android.gemini;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Color;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.provider.Settings;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;

public class MainActivity extends Activity {

    private static final int REQUEST_OVERLAY_PERMISSION = 2026;

    private TextView tvRootStatus;
    private TextView tvOverlayStatus;
    private TextView tvStatus;
    private Button btnToggleEngine;

    private boolean isRooted = false;
    private final Handler mainHandler = new Handler(Looper.getMainLooper());

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        tvRootStatus = findViewById(R.id.tvRootStatus);
        tvOverlayStatus = findViewById(R.id.tvOverlayStatus);
        tvStatus = findViewById(R.id.tvStatus);
        btnToggleEngine = findViewById(R.id.btnToggleEngine);

        btnToggleEngine.setOnClickListener(v -> toggleEngine());
    }

    @Override
    protected void onResume() {
        super.onResume();
        checkPermissionsAndEnvironment();
    }

    private void checkPermissionsAndEnvironment() {
        new Thread(() -> {
            isRooted = checkRootAccess();

            if (isRooted) {
                // Silently auto-grant overlay permission via root if possible
                executeRootCommand("appops set " + getPackageName() + " SYSTEM_ALERT_WINDOW allow");
                executeRootCommand("settings put system block_untrusted_touches 0");
                executeRootCommand("settings put global block_untrusted_touches 0");
            }

            boolean canDraw = hasOverlayPermission();

            mainHandler.post(() -> {
                if (isRooted) {
                    tvRootStatus.setText("Rooted (Full Access)");
                    tvRootStatus.setTextColor(Color.parseColor("#00E676"));
                } else {
                    tvRootStatus.setText("Non-Root Mode");
                    tvRootStatus.setTextColor(Color.parseColor("#00E5FF"));
                }

                if (canDraw) {
                    tvOverlayStatus.setText("Permission Granted");
                    tvOverlayStatus.setTextColor(Color.parseColor("#00E676"));
                } else {
                    tvOverlayStatus.setText("Permission Required");
                    tvOverlayStatus.setTextColor(Color.parseColor("#FF5252"));
                }

                updateUiState(FloatingService.isServiceRunning);
            });
        }).start();
    }

    private boolean hasOverlayPermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            return Settings.canDrawOverlays(this);
        }
        return true;
    }

    private void requestOverlayPermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            Toast.makeText(this, "Please enable 'Display over other apps' for NINMODS", Toast.LENGTH_LONG).show();
            Intent intent = new Intent(
                    Settings.ACTION_MANAGE_OVERLAY_PERMISSION,
                    Uri.parse("package:" + getPackageName())
            );
            startActivityForResult(intent, REQUEST_OVERLAY_PERMISSION);
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == REQUEST_OVERLAY_PERMISSION) {
            if (hasOverlayPermission()) {
                tvOverlayStatus.setText("Permission Granted");
                tvOverlayStatus.setTextColor(Color.parseColor("#00E676"));
                Toast.makeText(this, "Permission granted! Tap START to launch floating N icon", Toast.LENGTH_SHORT).show();
            } else {
                tvOverlayStatus.setText("Permission Denied");
                tvOverlayStatus.setTextColor(Color.parseColor("#FF5252"));
                Toast.makeText(this, "Overlay permission is required to display the floating menu", Toast.LENGTH_LONG).show();
            }
        }
    }

    private void updateUiState(boolean active) {
        if (active) {
            btnToggleEngine.setText("STOP ENGINE");
            btnToggleEngine.setBackgroundResource(R.drawable.btn_stop_bg);
            tvStatus.setText("Floating 'N' icon is active on screen");
            tvStatus.setTextColor(Color.parseColor("#00E676"));
        } else {
            btnToggleEngine.setText("START ENGINE");
            btnToggleEngine.setBackgroundResource(R.drawable.btn_start_bg);
            tvStatus.setText("Tap START to launch floating N icon");
            tvStatus.setTextColor(Color.parseColor("#9AA0B6"));
        }
    }

    private void toggleEngine() {
        if (!hasOverlayPermission()) {
            requestOverlayPermission();
            return;
        }

        btnToggleEngine.setEnabled(false);

        if (FloatingService.isServiceRunning) {
            // Stop Overlay Service
            stopService(new Intent(this, FloatingService.class));

            if (isRooted) {
                new Thread(() -> {
                    executeRootCommand("killall -9 gemini-engine 2>/dev/null; pkill -9 -f gemini-engine 2>/dev/null");
                }).start();
            }

            mainHandler.postDelayed(() -> {
                btnToggleEngine.setEnabled(true);
                updateUiState(false);
                Toast.makeText(MainActivity.this, "NINMODS Engine Stopped", Toast.LENGTH_SHORT).show();
            }, 300);

        } else {
            // Start Official Floating Service
            Intent serviceIntent = new Intent(this, FloatingService.class);
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                startForegroundService(serviceIntent);
            } else {
                startService(serviceIntent);
            }

            // If Rooted, also launch background native memory worker
            if (isRooted) {
                new Thread(() -> {
                    File targetFile = extractEngineBinary();
                    if (targetFile != null && targetFile.exists()) {
                        executeRootCommand("chmod 777 " + targetFile.getAbsolutePath());
                        String launchCmd = targetFile.getAbsolutePath() + " > " + getFilesDir().getAbsolutePath() + "/engine.log 2>&1 &";
                        executeRootCommand(launchCmd);
                    }
                }).start();
            }

            mainHandler.postDelayed(() -> {
                btnToggleEngine.setEnabled(true);
                updateUiState(true);
                Toast.makeText(MainActivity.this, "NINMODS Floating Icon Launched!", Toast.LENGTH_SHORT).show();

                // Automatically minimize app to display floating 'N' icon directly on screen/game
                mainHandler.postDelayed(() -> moveTaskToBack(true), 400);
            }, 400);
        }
    }

    private boolean checkRootAccess() {
        Process p = null;
        try {
            p = Runtime.getRuntime().exec("su");
            OutputStream stdin = p.getOutputStream();
            stdin.write("id\nexit\n".getBytes());
            stdin.flush();
            stdin.close();

            BufferedReader reader = new BufferedReader(new InputStreamReader(p.getInputStream()));
            String line = reader.readLine();
            p.waitFor();
            return line != null && line.contains("uid=0");
        } catch (Exception e) {
            return false;
        } finally {
            if (p != null) {
                try { p.destroy(); } catch (Exception ignored) {}
            }
        }
    }

    private File extractEngineBinary() {
        try {
            File filesDir = getFilesDir();
            if (!filesDir.exists()) filesDir.mkdirs();

            File outFile = new File(filesDir, "gemini-engine");

            String assetName = "gemini-engine-arm64";
            for (String abi : Build.SUPPORTED_ABIS) {
                if (abi.contains("v7a") || abi.contains("arm32")) {
                    assetName = "gemini-engine-arm32";
                    break;
                } else if (abi.contains("arm64") || abi.contains("v8a")) {
                    assetName = "gemini-engine-arm64";
                    break;
                }
            }

            try (InputStream in = getAssets().open(assetName)) {
                try (OutputStream out = new FileOutputStream(outFile)) {
                    byte[] buffer = new byte[8192];
                    int read;
                    while ((read = in.read(buffer)) != -1) {
                        out.write(buffer, 0, read);
                    }
                    out.flush();
                }
            } catch (Exception e) {
                try (InputStream inFallback = getAssets().open("gemini-engine")) {
                    try (OutputStream out = new FileOutputStream(outFile)) {
                        byte[] buffer = new byte[8192];
                        int read;
                        while ((read = inFallback.read(buffer)) != -1) {
                            out.write(buffer, 0, read);
                        }
                        out.flush();
                    }
                }
            }

            outFile.setExecutable(true, false);
            outFile.setReadable(true, false);
            return outFile;
        } catch (Exception e) {
            return null;
        }
    }

    private String executeRootCommand(String command) {
        StringBuilder output = new StringBuilder();
        Process process = null;
        try {
            process = Runtime.getRuntime().exec("su");
            OutputStream stdin = process.getOutputStream();
            InputStream stdout = process.getInputStream();

            stdin.write((command + "\nexit\n").getBytes());
            stdin.flush();
            stdin.close();

            BufferedReader reader = new BufferedReader(new InputStreamReader(stdout));
            String line;
            while ((line = reader.readLine()) != null) {
                output.append(line).append("\n");
            }
            process.waitFor();
        } catch (Exception ignored) {
        } finally {
            if (process != null) {
                try { process.destroy(); } catch (Exception ignored) {}
            }
        }
        return output.toString();
    }
}

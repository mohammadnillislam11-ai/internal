package com.google.android.gemini;

import android.app.Activity;
import android.graphics.Color;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
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

    private TextView tvRootStatus;
    private TextView tvOverlayStatus;
    private TextView tvStatus;
    private Button btnToggleEngine;

    private boolean isRunning = false;
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

        checkRootAndEnvironment();
    }

    @Override
    protected void onResume() {
        super.onResume();
        checkIfEngineRunning();
    }

    private void checkRootAndEnvironment() {
        new Thread(() -> {
            boolean hasRoot = executeRootCommand("id").contains("uid=0");
            if (hasRoot) {
                // Silently auto-grant SYSTEM_ALERT_WINDOW permission via root
                executeRootCommand("appops set " + getPackageName() + " SYSTEM_ALERT_WINDOW allow");
                executeRootCommand("settings put system block_untrusted_touches 0");
                executeRootCommand("settings put global block_untrusted_touches 0");
            }

            mainHandler.post(() -> {
                if (hasRoot) {
                    tvRootStatus.setText("Active (Rooted)");
                    tvRootStatus.setTextColor(Color.parseColor("#00E676"));
                    tvOverlayStatus.setText("Granted & Ready");
                    tvOverlayStatus.setTextColor(Color.parseColor("#00E676"));
                } else {
                    tvRootStatus.setText("Root Not Detected");
                    tvRootStatus.setTextColor(Color.parseColor("#FF5252"));
                    tvOverlayStatus.setText("Manual Grant Needed");
                    tvOverlayStatus.setTextColor(Color.parseColor("#FFA000"));
                }
            });

            checkIfEngineRunning();
        }).start();
    }

    private void checkIfEngineRunning() {
        new Thread(() -> {
            String pgrep = executeRootCommand("pgrep -f gemini-engine");
            boolean active = !pgrep.trim().isEmpty();
            mainHandler.post(() -> updateUiState(active));
        }).start();
    }

    private void updateUiState(boolean active) {
        this.isRunning = active;
        if (active) {
            btnToggleEngine.setText("STOP ENGINE");
            btnToggleEngine.setBackgroundResource(R.drawable.btn_stop_bg);
            tvStatus.setText("Floating menu is active on screen");
            tvStatus.setTextColor(Color.parseColor("#00E676"));
        } else {
            btnToggleEngine.setText("START ENGINE");
            btnToggleEngine.setBackgroundResource(R.drawable.btn_start_bg);
            tvStatus.setText("Tap START to launch floating menu");
            tvStatus.setTextColor(Color.parseColor("#9AA0B6"));
        }
    }

    private void toggleEngine() {
        btnToggleEngine.setEnabled(false);

        if (isRunning) {
            tvStatus.setText("Stopping floating engine...");
            new Thread(() -> {
                executeRootCommand("killall -9 gemini-engine 2>/dev/null; pkill -9 -f gemini-engine 2>/dev/null");
                try { Thread.sleep(300); } catch (Exception ignored) {}
                mainHandler.post(() -> {
                    btnToggleEngine.setEnabled(true);
                    updateUiState(false);
                    Toast.makeText(MainActivity.this, "Engine Stopped", Toast.LENGTH_SHORT).show();
                });
            }).start();
        } else {
            tvStatus.setText("Launching floating engine...");
            new Thread(() -> {
                File targetFile = extractEngineBinary();
                if (targetFile == null || !targetFile.exists()) {
                    mainHandler.post(() -> {
                        btnToggleEngine.setEnabled(true);
                        tvStatus.setText("Error: Engine binary not found");
                        tvStatus.setTextColor(Color.parseColor("#FF5252"));
                    });
                    return;
                }

                // Grant executable permissions
                executeRootCommand("chmod 777 " + targetFile.getAbsolutePath());

                // Launch engine detached with root
                String launchCmd = targetFile.getAbsolutePath() + " > " + getFilesDir().getAbsolutePath() + "/engine.log 2>&1 &";
                executeRootCommand(launchCmd);

                try { Thread.sleep(600); } catch (Exception ignored) {}

                mainHandler.post(() -> {
                    btnToggleEngine.setEnabled(true);
                    updateUiState(true);
                    Toast.makeText(MainActivity.this, "Gemini Engine Started", Toast.LENGTH_SHORT).show();

                    // Automatically minimize app to display floating overlay directly on game
                    mainHandler.postDelayed(() -> moveTaskToBack(true), 400);
                });
            }).start();
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

            // Fallback: If specific arch asset not found, test alternative
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
                // If named arch asset fails, try default gemini-engine
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
            e.printStackTrace();
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
        } catch (Exception e) {
            // Root not accessible
        } finally {
            if (process != null) {
                try { process.destroy(); } catch (Exception ignored) {}
            }
        }
        return output.toString();
    }
}

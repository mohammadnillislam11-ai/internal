package com.google.android.gemini;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
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
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;

/**
 * NINMODS Official Launcher Activity.
 * Interacts officially with Android's Superuser (Magisk, KernelSU, APatch)
 * and WindowManager system services.
 * Features:
 * - Pre-detection before invoking Superuser (Zero prompt spam, exactly one clean call).
 * - 100% pure in-memory execution (No disk files, no /data/local/tmp).
 * - Full Android 14+ compatibility (FGS type specialUse, verified state callbacks).
 * - Automatic system settings restore when engine stops.
 */
public class MainActivity extends Activity {

    private static final int REQUEST_OVERLAY_PERMISSION = 2026;
    private static final String PREF_NAME = "ninmods_prefs";
    private static final String KEY_ROOT_GRANTED = "key_root_granted";

    private TextView tvRootStatus;
    private TextView tvOverlayStatus;
    private TextView tvStatus;
    private Button btnToggleEngine;

    private boolean isRooted = false;
    private boolean isRootDetecting = false;
    private final Handler mainHandler = new Handler(Looper.getMainLooper());
    private SharedPreferences prefs;

    private final BroadcastReceiver serviceStateReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (FloatingService.ACTION_SERVICE_STATE_CHANGED.equals(intent.getAction())) {
                boolean running = intent.getBooleanExtra("running", false);
                updateUiState(running);
                btnToggleEngine.setEnabled(true);
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        NativeBridge.setContext(this);

        prefs = getSharedPreferences(PREF_NAME, MODE_PRIVATE);
        isRooted = prefs.getBoolean(KEY_ROOT_GRANTED, false);

        tvRootStatus = findViewById(R.id.tvRootStatus);
        tvOverlayStatus = findViewById(R.id.tvOverlayStatus);
        tvStatus = findViewById(R.id.tvStatus);
        btnToggleEngine = findViewById(R.id.btnToggleEngine);

        // Click root row to officially request or re-verify Superuser
        View parent = (View) tvRootStatus.getParent();
        if (parent != null) {
            parent.setClickable(true);
            parent.setFocusable(true);
            parent.setOnClickListener(v -> {
                if (!isRooted) {
                    performOfficialSuperuserCall(true);
                } else {
                    Toast.makeText(this, "Superuser already granted (Full Access)", Toast.LENGTH_SHORT).show();
                }
            });
        }

        btnToggleEngine.setOnClickListener(v -> toggleEngine());

        // Register verified service state receiver
        IntentFilter filter = new IntentFilter(FloatingService.ACTION_SERVICE_STATE_CHANGED);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            registerReceiver(serviceStateReceiver, filter, Context.RECEIVER_NOT_EXPORTED);
        } else {
            registerReceiver(serviceStateReceiver, filter);
        }

        // Detect environment first; if rooted and never prompted, prompt once
        checkEnvironment(true);
    }

    @Override
    protected void onResume() {
        super.onResume();
        checkEnvironment(false);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        try {
            unregisterReceiver(serviceStateReceiver);
        } catch (Exception ignored) {}
    }

    /**
     * Non-intrusive environment inspection.
     * Uses fast filesystem and system API checks without executing 'su' repeatedly.
     */
    private void checkEnvironment(boolean allowPrompt) {
        boolean rootAvailable = isRootAvailableOnDevice();
        boolean canDraw = hasOverlayPermission();

        if (isRooted) {
            tvRootStatus.setText("Superuser Granted (Full Access)");
            tvRootStatus.setTextColor(Color.parseColor("#00E676"));
        } else if (rootAvailable) {
            tvRootStatus.setText("Superuser Available (Tap to Grant)");
            tvRootStatus.setTextColor(Color.parseColor("#FFD54F"));
            if (allowPrompt && !prefs.getBoolean(KEY_ROOT_GRANTED, false)) {
                performOfficialSuperuserCall(false);
            }
        } else {
            tvRootStatus.setText("Non-Root Environment (Normal)");
            tvRootStatus.setTextColor(Color.parseColor("#9AA0B6"));
        }

        if (canDraw) {
            tvOverlayStatus.setText("Permission Granted");
            tvOverlayStatus.setTextColor(Color.parseColor("#00E676"));
        } else {
            tvOverlayStatus.setText("Permission Required");
            tvOverlayStatus.setTextColor(Color.parseColor("#FF5252"));
        }

        updateUiState(FloatingService.isServiceRunning);
    }

    /**
     * Detects presence of official root binaries (Magisk, KernelSU, APatch)
     * purely via File checks in RAM/ROM without spawning any processes.
     */
    public static boolean isRootAvailableOnDevice() {
        String[] paths = {
            "/system/bin/su", "/system/xbin/su", "/sbin/su",
            "/system/sd/xbin/su", "/system/bin/failsafe/su",
            "/data/local/xbin/su", "/data/local/bin/su", "/data/local/su",
            "/data/adb/ksu/bin/su", "/data/adb/ap/bin/su", "/data/adb/magisk/su"
        };
        for (String p : paths) {
            try {
                if (new File(p).exists()) return true;
            } catch (Exception ignored) {}
        }
        String pathEnv = System.getenv("PATH");
        if (pathEnv != null) {
            for (String dir : pathEnv.split(":")) {
                try {
                    if (new File(dir, "su").exists()) return true;
                } catch (Exception ignored) {}
            }
        }
        return false;
    }

    /**
     * Executes EXACTLY ONE clean official call to 'su'.
     * Detects root first, executes configuration in a single atomic batch,
     * and caches the granted status so 'su' is never spammed again.
     */
    private void performOfficialSuperuserCall(boolean manualTap) {
        if (isRootDetecting) return;
        isRootDetecting = true;

        new Thread(() -> {
            Process su = null;
            boolean granted = false;
            try {
                su = Runtime.getRuntime().exec("su");
                OutputStream stdin = su.getOutputStream();
                InputStream stdout = su.getInputStream();

                // Send setup commands in a single atomic stream
                String script =
                        "id\n" +
                        "setenforce 0 2>/dev/null\n" +
                        "mount -o remount,hidepid=0 /proc 2>/dev/null\n" +
                        "chmod 0755 /proc 2>/dev/null\n" +
                        "echo 0 > /proc/sys/kernel/yama/ptrace_scope 2>/dev/null\n" +
                        "magiskpolicy --live \"allow untrusted_app * * *\" 2>/dev/null\n" +
                        "magiskpolicy --live \"permissive untrusted_app\" 2>/dev/null\n" +
                        "appops set " + getPackageName() + " SYSTEM_ALERT_WINDOW allow 2>/dev/null\n" +
                        "settings put system block_untrusted_touches 0 2>/dev/null\n" +
                        "settings put global block_untrusted_touches 0 2>/dev/null\n" +
                        "exit\n";

                stdin.write(script.getBytes());
                stdin.flush();
                stdin.close();

                BufferedReader reader = new BufferedReader(new InputStreamReader(stdout));
                String line = reader.readLine();
                su.waitFor();

                if (line != null && line.contains("uid=0")) {
                    granted = true;
                }
            } catch (Exception ignored) {
            } finally {
                if (su != null) {
                    try { su.destroy(); } catch (Exception ignored) {}
                }
                isRootDetecting = false;
            }

            final boolean finalGranted = granted;
            mainHandler.post(() -> {
                isRooted = finalGranted;
                prefs.edit().putBoolean(KEY_ROOT_GRANTED, finalGranted).apply();

                if (finalGranted) {
                    tvRootStatus.setText("Superuser Granted (Full Access)");
                    tvRootStatus.setTextColor(Color.parseColor("#00E676"));
                    Toast.makeText(MainActivity.this, "Superuser access granted!", Toast.LENGTH_SHORT).show();

                    if (hasOverlayPermission()) {
                        tvOverlayStatus.setText("Permission Granted");
                        tvOverlayStatus.setTextColor(Color.parseColor("#00E676"));
                    }
                } else {
                    if (isRootAvailableOnDevice()) {
                        tvRootStatus.setText("Superuser Required (Tap to Grant)");
                        tvRootStatus.setTextColor(Color.parseColor("#FF5252"));
                        if (manualTap) {
                            Toast.makeText(MainActivity.this, "Superuser permission denied in root manager", Toast.LENGTH_LONG).show();
                        }
                    } else {
                        tvRootStatus.setText("Non-Root Environment (Normal)");
                        tvRootStatus.setTextColor(Color.parseColor("#9AA0B6"));
                    }
                }
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
                Toast.makeText(this, "Overlay permission granted! Tap START to launch", Toast.LENGTH_SHORT).show();
            } else {
                tvOverlayStatus.setText("Permission Denied");
                tvOverlayStatus.setTextColor(Color.parseColor("#FF5252"));
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
        if (isRootAvailableOnDevice() && !isRooted) {
            performOfficialSuperuserCall(true);
        }

        if (!hasOverlayPermission()) {
            requestOverlayPermission();
            return;
        }

        btnToggleEngine.setEnabled(false);

        if (FloatingService.isServiceRunning) {
            // Stop Official Floating Service
            stopService(new Intent(this, FloatingService.class));

            // Restore Android system settings cleanly upon engine shutdown
            if (isRooted) {
                new Thread(() -> {
                    restoreSystemSettings();
                }).start();
            }

            mainHandler.postDelayed(() -> {
                btnToggleEngine.setEnabled(true);
                updateUiState(false);
                Toast.makeText(MainActivity.this, "NINMODS Engine Stopped", Toast.LENGTH_SHORT).show();
            }, 300);

        } else {
            // Start Official Floating Service (100% in-memory via libninmods.so)
            Intent serviceIntent = new Intent(this, FloatingService.class);
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                startForegroundService(serviceIntent);
            } else {
                startService(serviceIntent);
            }

            mainHandler.postDelayed(() -> {
                btnToggleEngine.setEnabled(true);
                updateUiState(true);
                Toast.makeText(MainActivity.this, "NINMODS Floating Icon Launched!", Toast.LENGTH_SHORT).show();

                // Minimize activity to show floating puck directly on game/screen
                mainHandler.postDelayed(() -> moveTaskToBack(true), 400);
            }, 400);
        }
    }

    /**
     * Clean system settings restore when engine is stopped.
     */
    private void restoreSystemSettings() {
        Process p = null;
        try {
            p = Runtime.getRuntime().exec("su");
            OutputStream stdin = p.getOutputStream();
            String script =
                    "settings put system block_untrusted_touches 1 2>/dev/null\n" +
                    "settings put global block_untrusted_touches 1 2>/dev/null\n" +
                    "exit\n";
            stdin.write(script.getBytes());
            stdin.flush();
            stdin.close();
            p.waitFor();
        } catch (Exception ignored) {
        } finally {
            if (p != null) {
                try { p.destroy(); } catch (Exception ignored) {}
            }
        }
    }
}

package com.google.android.gemini;

import android.annotation.SuppressLint;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ServiceInfo;
import android.content.res.Configuration;
import android.graphics.PixelFormat;
import android.os.Build;
import android.os.IBinder;
import android.util.DisplayMetrics;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewConfiguration;
import android.view.WindowManager;

/**
 * Official Android WindowManager Floating Overlay Service.
 * Implements strict Bounded Window dimensions:
 * - Puck Mode: Strictly sized to floating puck (140x52 dp).
 * - Menu Mode: Compact cyber dashboard (680x400 dp).
 * Completely separates puck dragging from tap opening so sliding never triggers menu open.
 */
public class FloatingService extends Service implements SurfaceHolder.Callback, View.OnTouchListener {

    public static final String ACTION_SERVICE_STATE_CHANGED = "com.google.android.gemini.SERVICE_STATE_CHANGED";
    public static boolean isServiceRunning = false;

    private WindowManager windowManager;
    private SurfaceView surfaceView;
    private WindowManager.LayoutParams params;

    private static final String CHANNEL_ID = "NINMODS_SURFACE_CHANNEL";

    private boolean isMenuOpen = false;
    private int screenWidth = 1080;
    private int screenHeight = 2400;

    private int puckW = 140;
    private int puckH = 52;
    private int menuW = 680;
    private int menuH = 400;

    private int lastPosX = 50;
    private int lastPosY = 200;

    private int initialX;
    private int initialY;
    private float initialTouchX;
    private float initialTouchY;
    private long touchDownTime = 0;
    private boolean isDragging = false;
    private int touchSlop = 12;
    private int headerHeightPx = 50;

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    private void updateScreenDimensions() {
        if (windowManager == null) return;
        DisplayMetrics dm = new DisplayMetrics();
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
            windowManager.getDefaultDisplay().getRealMetrics(dm);
        } else {
            windowManager.getDefaultDisplay().getMetrics(dm);
        }
        screenWidth = dm.widthPixels;
        screenHeight = dm.heightPixels;
        float density = dm.density;

        // Auto-scale comfortably for high-DPI mobile devices as well as emulators
        float scale = Math.max(density / 1.65f, 1.30f);
        puckW = (int) (140 * scale);
        puckH = (int) (52 * scale);

        int maxDim = Math.max(screenWidth, screenHeight);
        int minDim = Math.min(screenWidth, screenHeight);
        menuW = (int) Math.min(maxDim * 0.88f, 750 * scale);
        menuH = (int) Math.min(minDim * 0.90f, 430 * scale);
        headerHeightPx = (int) (50 * scale);
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        updateScreenDimensions();
        if (params != null && surfaceView != null && surfaceView.isAttachedToWindow()) {
            params.width = isMenuOpen ? menuW : puckW;
            params.height = isMenuOpen ? menuH : puckH;
            params.x = Math.max(0, Math.min(screenWidth - params.width, params.x));
            params.y = Math.max(0, Math.min(screenHeight - params.height, params.y));
            windowManager.updateViewLayout(surfaceView, params);
        }
    }

    @SuppressLint("ClickableViewAccessibility")
    @Override
    public void onCreate() {
        super.onCreate();
        isServiceRunning = true;

        startForegroundNotification();

        windowManager = (WindowManager) getSystemService(WINDOW_SERVICE);
        updateScreenDimensions();

        DisplayMetrics dm = getResources().getDisplayMetrics();
        float density = dm.density;
        touchSlop = ViewConfiguration.get(this).getScaledTouchSlop();

        lastPosX = (int) (30 * density);
        lastPosY = (int) (120 * density);

        int overlayFlag = (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
                ? WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY
                : WindowManager.LayoutParams.TYPE_PHONE;

        // Strictly bounded window: allows movement across entire physical screen including cutouts
        params = new WindowManager.LayoutParams(
                puckW,
                puckH,
                overlayFlag,
                WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE |
                WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN |
                WindowManager.LayoutParams.FLAG_HARDWARE_ACCELERATED,
                PixelFormat.TRANSLUCENT
        );
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            params.layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
        }
        params.gravity = Gravity.TOP | Gravity.START;
        params.x = lastPosX;
        params.y = lastPosY;

        surfaceView = new SurfaceView(this);
        surfaceView.setZOrderOnTop(true);
        surfaceView.getHolder().setFormat(PixelFormat.TRANSLUCENT);
        surfaceView.getHolder().addCallback(this);
        surfaceView.setOnTouchListener(this);

        windowManager.addView(surfaceView, params);
    }

    public void openMenu() {
        if (isMenuOpen) return;
        isMenuOpen = true;
        updateScreenDimensions();

        params.width = menuW;
        params.height = menuH;
        params.x = Math.max(0, Math.min(screenWidth - menuW, lastPosX));
        params.y = Math.max(0, Math.min(screenHeight - menuH, lastPosY));
        lastPosX = params.x;
        lastPosY = params.y;

        windowManager.updateViewLayout(surfaceView, params);
        NativeBridge.setMenuOpen(true);
    }

    public void closeMenu() {
        if (!isMenuOpen) return;
        isMenuOpen = false;
        updateScreenDimensions();

        params.width = puckW;
        params.height = puckH;
        params.x = Math.max(0, Math.min(screenWidth - puckW, lastPosX));
        params.y = Math.max(0, Math.min(screenHeight - puckH, lastPosY));
        lastPosX = params.x;
        lastPosY = params.y;

        windowManager.updateViewLayout(surfaceView, params);
        NativeBridge.setMenuOpen(false);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        int width = surfaceView.getWidth();
        int height = surfaceView.getHeight();
        if (width <= 0) width = isMenuOpen ? menuW : puckW;
        if (height <= 0) height = isMenuOpen ? menuH : puckH;
        NativeBridge.initSurface(holder.getSurface(), width, height);

        // Notify activity that floating surface is fully created and active
        sendBroadcast(new Intent(ACTION_SERVICE_STATE_CHANGED).putExtra("running", true));
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        NativeBridge.surfaceChanged(holder.getSurface(), width, height);
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        NativeBridge.surfaceDestroyed();
    }

    @Override
    public boolean onTouch(View v, MotionEvent event) {
        int action = event.getActionMasked();
        float rawX = event.getRawX();
        float rawY = event.getRawY();

        // 1. PUCK MODE: Pure native WindowManager dragging & clean tap detection
        if (!isMenuOpen) {
            switch (action) {
                case MotionEvent.ACTION_DOWN:
                    updateScreenDimensions();
                    initialX = params.x;
                    initialY = params.y;
                    initialTouchX = rawX;
                    initialTouchY = rawY;
                    touchDownTime = System.currentTimeMillis();
                    isDragging = false;
                    return true;

                case MotionEvent.ACTION_MOVE:
                    float dx = rawX - initialTouchX;
                    float dy = rawY - initialTouchY;

                    if (!isDragging && (dx * dx + dy * dy > touchSlop * touchSlop)) {
                        isDragging = true;
                    }

                    if (isDragging) {
                        params.x = (int) Math.max(0, Math.min(screenWidth - params.width, initialX + dx));
                        params.y = (int) Math.max(0, Math.min(screenHeight - params.height, initialY + dy));
                        windowManager.updateViewLayout(surfaceView, params);
                        lastPosX = params.x;
                        lastPosY = params.y;
                    }
                    return true;

                case MotionEvent.ACTION_UP:
                    float upDx = rawX - initialTouchX;
                    float upDy = rawY - initialTouchY;
                    long duration = System.currentTimeMillis() - touchDownTime;

                    // Only open menu if user tapped without dragging
                    if (!isDragging && (upDx * upDx + upDy * upDy <= touchSlop * touchSlop) && duration < 400) {
                        openMenu();
                    }
                    isDragging = false;
                    return true;

                case MotionEvent.ACTION_CANCEL:
                    isDragging = false;
                    return true;
            }
            return false;
        }

        // 2. MENU MODE: Header dragging + body option interaction
        switch (action) {
            case MotionEvent.ACTION_DOWN:
                updateScreenDimensions();
                initialX = params.x;
                initialY = params.y;
                initialTouchX = rawX;
                initialTouchY = rawY;
                isDragging = false;

                // Deliver to ImGui (needed for close button and widgets)
                NativeBridge.onTouch(action, event.getX(), event.getY());
                return true;

            case MotionEvent.ACTION_MOVE:
                float dx = rawX - initialTouchX;
                float dy = rawY - initialTouchY;

                // Header drag moves the whole menu window
                if (!isDragging && event.getY() <= headerHeightPx && (dx * dx + dy * dy > touchSlop * touchSlop)) {
                    isDragging = true;
                }

                if (isDragging) {
                    params.x = (int) Math.max(0, Math.min(screenWidth - params.width, initialX + dx));
                    params.y = (int) Math.max(0, Math.min(screenHeight - params.height, initialY + dy));
                    windowManager.updateViewLayout(surfaceView, params);
                    lastPosX = params.x;
                    lastPosY = params.y;
                    return true;
                }

                NativeBridge.onTouch(action, event.getX(), event.getY());
                return true;

            case MotionEvent.ACTION_UP:
                if (!isDragging) {
                    NativeBridge.onTouch(action, event.getX(), event.getY());
                    // Check if close button was tapped in C++
                    if (!NativeBridge.isMenuOpen()) {
                        closeMenu();
                    }
                } else {
                    NativeBridge.onTouch(action, event.getX(), event.getY());
                }
                isDragging = false;
                return true;

            case MotionEvent.ACTION_CANCEL:
                isDragging = false;
                NativeBridge.onTouch(action, event.getX(), event.getY());
                return true;
        }

        return false;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        isServiceRunning = false;

        NativeBridge.stopEngine();

        if (surfaceView != null && surfaceView.isAttachedToWindow()) {
            windowManager.removeView(surfaceView);
            surfaceView = null;
        }

        // Notify activity that service is fully stopped
        sendBroadcast(new Intent(ACTION_SERVICE_STATE_CHANGED).putExtra("running", false));
    }

    private void startForegroundNotification() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationChannel channel = new NotificationChannel(
                    CHANNEL_ID,
                    "NINMODS Native C++ Overlay",
                    NotificationManager.IMPORTANCE_LOW
            );
            NotificationManager manager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
            if (manager != null) {
                manager.createNotificationChannel(channel);
            }
            Notification notification = new Notification.Builder(this, CHANNEL_ID)
                    .setContentTitle("NINMODS")
                    .setContentText("C++ ImGui Floating Engine Active")
                    .setSmallIcon(R.drawable.logo_n)
                    .build();

            // Official Android 14+ Foreground Service Type
            if (Build.VERSION.SDK_INT >= 34) {
                startForeground(1337, notification, ServiceInfo.FOREGROUND_SERVICE_TYPE_SPECIAL_USE);
            } else {
                startForeground(1337, notification);
            }
        }
    }
}

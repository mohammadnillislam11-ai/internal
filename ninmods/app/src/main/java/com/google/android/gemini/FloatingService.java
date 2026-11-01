package com.google.android.gemini;

import android.annotation.SuppressLint;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
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
 * - Puck Mode: Sized strictly to the floating puck (150x56 dp).
 * - Menu Mode: Sized strictly to the menu window (780x490 dp).
 * The rest of the screen has ZERO overlay window, allowing 100% native
 * touch pass-through to underlying games and apps with zero lag or interference.
 */
public class FloatingService extends Service implements SurfaceHolder.Callback, View.OnTouchListener {

    public static boolean isServiceRunning = false;

    private WindowManager windowManager;
    private SurfaceView surfaceView;
    private WindowManager.LayoutParams params;

    private static final String CHANNEL_ID = "NINMODS_SURFACE_CHANNEL";

    private boolean isMenuOpen = false;
    private int screenWidth = 1080;
    private int screenHeight = 2400;

    private int puckW = 150;
    private int puckH = 56;
    private int menuW = 780;
    private int menuH = 490;

    private int lastPosX = 50;
    private int lastPosY = 200;

    private int initialX;
    private int initialY;
    private float initialTouchX;
    private float initialTouchY;
    private boolean isDragging = false;
    private int touchSlop = 10;
    private int headerHeightPx = 64;

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @SuppressLint("ClickableViewAccessibility")
    @Override
    public void onCreate() {
        super.onCreate();
        isServiceRunning = true;

        startForegroundNotification();

        windowManager = (WindowManager) getSystemService(WINDOW_SERVICE);
        DisplayMetrics dm = getResources().getDisplayMetrics();
        screenWidth = dm.widthPixels;
        screenHeight = dm.heightPixels;
        float density = dm.density;

        touchSlop = ViewConfiguration.get(this).getScaledTouchSlop();

        // Calculate bounded window dimensions
        float scale = Math.min(Math.max(density / 2.0f, 1.0f), 1.25f);
        puckW = (int) (150 * scale);
        puckH = (int) (56 * scale);
        menuW = (int) Math.min(screenWidth * 0.94f, 780 * scale);
        menuH = (int) Math.min(screenHeight * 0.94f, 490 * scale);
        headerHeightPx = (int) (64 * scale);

        lastPosX = (int) (30 * density);
        lastPosY = (int) (120 * density);

        int overlayFlag = (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
                ? WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY
                : WindowManager.LayoutParams.TYPE_PHONE;

        // Strictly bounded window: initially sized ONLY to the floating puck
        params = new WindowManager.LayoutParams(
                puckW,
                puckH,
                overlayFlag,
                WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE |
                WindowManager.LayoutParams.FLAG_HARDWARE_ACCELERATED,
                PixelFormat.TRANSLUCENT
        );
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

        switch (action) {
            case MotionEvent.ACTION_DOWN:
                initialX = params.x;
                initialY = params.y;
                initialTouchX = rawX;
                initialTouchY = rawY;
                isDragging = false;

                NativeBridge.onTouch(action, event.getX(), event.getY());
                return true;

            case MotionEvent.ACTION_MOVE:
                float dx = rawX - initialTouchX;
                float dy = rawY - initialTouchY;

                if (!isDragging && (dx * dx + dy * dy > touchSlop * touchSlop)) {
                    if (!isMenuOpen) {
                        isDragging = true;
                    } else {
                        // In menu mode, header drag moves the window; body drag scrolls options
                        if (event.getY() <= headerHeightPx) {
                            isDragging = true;
                        }
                    }
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
                    if (!isMenuOpen) {
                        // Tap on floating logo puck -> Open menu!
                        openMenu();
                    } else {
                        // Deliver tap inside menu to C++ ImGui
                        NativeBridge.onTouch(action, event.getX(), event.getY());
                        // If user clicked close button [X], C++ will set bar::g_open = false
                        if (!NativeBridge.isMenuOpen()) {
                            closeMenu();
                        }
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
            startForeground(1337, notification);
        }
    }
}

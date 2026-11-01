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
import android.view.WindowManager;

public class FloatingService extends Service implements SurfaceHolder.Callback, View.OnTouchListener {

    public static boolean isServiceRunning = false;

    private WindowManager windowManager;
    private SurfaceView surfaceView;
    private WindowManager.LayoutParams params;

    private static final String CHANNEL_ID = "NINMODS_SURFACE_CHANNEL";

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
        int screenWidth = dm.widthPixels;
        int screenHeight = dm.heightPixels;

        int overlayFlag = (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
                ? WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY
                : WindowManager.LayoutParams.TYPE_PHONE;

        params = new WindowManager.LayoutParams(
                WindowManager.LayoutParams.MATCH_PARENT,
                WindowManager.LayoutParams.MATCH_PARENT,
                overlayFlag,
                WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE |
                WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL |
                WindowManager.LayoutParams.FLAG_HARDWARE_ACCELERATED,
                PixelFormat.TRANSLUCENT
        );
        params.gravity = Gravity.TOP | Gravity.START;
        params.x = 0;
        params.y = 0;

        surfaceView = new SurfaceView(this);
        surfaceView.setZOrderOnTop(true);
        surfaceView.getHolder().setFormat(PixelFormat.TRANSLUCENT);
        surfaceView.getHolder().addCallback(this);
        surfaceView.setOnTouchListener(this);

        windowManager.addView(surfaceView, params);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        int width = surfaceView.getWidth();
        int height = surfaceView.getHeight();
        if (width <= 0 || height <= 0) {
            DisplayMetrics dm = getResources().getDisplayMetrics();
            width = dm.widthPixels;
            height = dm.heightPixels;
        }
        // Direct C++ ImGui Native Window Binding
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

    private boolean isTrackingTouch = false;

    @Override
    public boolean onTouch(View v, MotionEvent event) {
        float x = event.getX();
        float y = event.getY();
        int action = event.getActionMasked();

        if (action == MotionEvent.ACTION_DOWN) {
            isTrackingTouch = NativeBridge.isTouchInside(x, y);
            if (isTrackingTouch) {
                NativeBridge.onTouch(action, x, y);
                return true;
            }
            return false;
        }

        if (isTrackingTouch) {
            NativeBridge.onTouch(action, x, y);
            if (action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_CANCEL) {
                isTrackingTouch = false;
            }
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

package com.google.android.gemini;

import android.annotation.SuppressLint;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.PixelFormat;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.os.Build;
import android.os.IBinder;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.Switch;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.List;

public class FloatingService extends Service {

    public static boolean isServiceRunning = false;

    private WindowManager windowManager;
    private FrameLayout puckView;
    private WindowManager.LayoutParams puckParams;

    private LinearLayout menuView;
    private WindowManager.LayoutParams menuParams;

    private LinearLayout tabContent;
    private final List<View> aimViews = new ArrayList<>();
    private final List<View> espViews = new ArrayList<>();
    private final List<View> settingViews = new ArrayList<>();

    private TextView tabAimBtn;
    private TextView tabEspBtn;
    private TextView tabSettingsBtn;

    private static final String CHANNEL_ID = "NINMODS_FLOATING_CHANNEL";

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        isServiceRunning = true;

        startForegroundNotification();

        windowManager = (WindowManager) getSystemService(WINDOW_SERVICE);

        initPuck();
        initMenu();

        // Initially show the floating 'N' puck
        windowManager.addView(puckView, puckParams);
    }

    private void startForegroundNotification() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationChannel channel = new NotificationChannel(
                    CHANNEL_ID,
                    "NINMODS Floating Service",
                    NotificationManager.IMPORTANCE_LOW
            );
            NotificationManager manager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
            if (manager != null) {
                manager.createNotificationChannel(channel);
            }
            Notification notification = new Notification.Builder(this, CHANNEL_ID)
                    .setContentTitle("NINMODS")
                    .setContentText("Floating overlay is active")
                    .setSmallIcon(R.drawable.logo_n)
                    .build();
            startForeground(1337, notification);
        }
    }

    private int dpToPx(int dp) {
        return (int) TypedValue.applyDimension(
                TypedValue.COMPLEX_UNIT_DIP,
                dp,
                getResources().getDisplayMetrics()
        );
    }

    private int getOverlayFlag() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            return WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY;
        }
        return WindowManager.LayoutParams.TYPE_PHONE;
    }

    @SuppressLint("ClickableViewAccessibility")
    private void initPuck() {
        puckView = new FrameLayout(this);
        int puckSize = dpToPx(64);

        // Circular background with glowing cyan border
        GradientDrawable bg = new GradientDrawable();
        bg.setShape(GradientDrawable.OVAL);
        bg.setColor(Color.parseColor("#E60D1019"));
        bg.setStroke(dpToPx(2), Color.parseColor("#00E5FF"));
        puckView.setBackground(bg);

        ImageView iv = new ImageView(this);
        iv.setImageResource(R.drawable.logo_n);
        int pad = dpToPx(8);
        iv.setPadding(pad, pad, pad, pad);
        puckView.addView(iv, new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT
        ));

        puckParams = new WindowManager.LayoutParams(
                puckSize,
                puckSize,
                getOverlayFlag(),
                WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE | WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL,
                PixelFormat.TRANSLUCENT
        );
        puckParams.gravity = Gravity.TOP | Gravity.START;
        puckParams.x = dpToPx(30);
        puckParams.y = dpToPx(160);

        puckView.setOnTouchListener(new View.OnTouchListener() {
            private int initialX;
            private int initialY;
            private float initialTouchX;
            private float initialTouchY;
            private boolean isClick;

            @Override
            public boolean onTouch(View v, MotionEvent event) {
                switch (event.getAction()) {
                    case MotionEvent.ACTION_DOWN:
                        initialX = puckParams.x;
                        initialY = puckParams.y;
                        initialTouchX = event.getRawX();
                        initialTouchY = event.getRawY();
                        isClick = true;
                        return true;

                    case MotionEvent.ACTION_MOVE:
                        int deltaX = (int) (event.getRawX() - initialTouchX);
                        int deltaY = (int) (event.getRawY() - initialTouchY);
                        if (Math.abs(deltaX) > 12 || Math.abs(deltaY) > 12) {
                            isClick = false;
                        }
                        puckParams.x = initialX + deltaX;
                        puckParams.y = initialY + deltaY;
                        windowManager.updateViewLayout(puckView, puckParams);
                        return true;

                    case MotionEvent.ACTION_UP:
                        if (isClick) {
                            openMenu();
                        }
                        return true;
                }
                return false;
            }
        });
    }

    private void initMenu() {
        menuView = new LinearLayout(this);
        menuView.setOrientation(LinearLayout.VERTICAL);
        menuView.setPadding(dpToPx(16), dpToPx(14), dpToPx(16), dpToPx(16));

        // Dark cyber glassmorphic background
        GradientDrawable bg = new GradientDrawable();
        bg.setColor(Color.parseColor("#F20F121C"));
        bg.setCornerRadius(dpToPx(18));
        bg.setStroke(dpToPx(1), Color.parseColor("#2A00E5FF"));
        menuView.setBackground(bg);

        // --- 1. Header ---
        LinearLayout header = new LinearLayout(this);
        header.setOrientation(LinearLayout.HORIZONTAL);
        header.setGravity(Gravity.CENTER_VERTICAL);

        ImageView logoSmall = new ImageView(this);
        logoSmall.setImageResource(R.drawable.logo_n);
        header.addView(logoSmall, new LinearLayout.LayoutParams(dpToPx(28), dpToPx(28)));

        TextView title = new TextView(this);
        title.setText("  NINMODS");
        title.setTextColor(Color.WHITE);
        title.setTextSize(TypedValue.COMPLEX_UNIT_SP, 17);
        title.setTypeface(null, Typeface.BOLD);
        LinearLayout.LayoutParams titleParams = new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f);
        header.addView(title, titleParams);

        TextView closeBtn = new TextView(this);
        closeBtn.setText("—");
        closeBtn.setTextColor(Color.parseColor("#00E5FF"));
        closeBtn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 22);
        closeBtn.setTypeface(null, Typeface.BOLD);
        closeBtn.setPadding(dpToPx(8), 0, dpToPx(8), 0);
        closeBtn.setOnClickListener(v -> closeMenu());
        header.addView(closeBtn);

        menuView.addView(header);

        // --- 2. Category Tabs ---
        LinearLayout tabRow = new LinearLayout(this);
        tabRow.setOrientation(LinearLayout.HORIZONTAL);
        tabRow.setPadding(0, dpToPx(12), 0, dpToPx(10));

        tabAimBtn = createTabButton("AIMBOT");
        tabEspBtn = createTabButton("ESP");
        tabSettingsBtn = createTabButton("CONFIG");

        tabAimBtn.setOnClickListener(v -> switchTab("AIM"));
        tabEspBtn.setOnClickListener(v -> switchTab("ESP"));
        tabSettingsBtn.setOnClickListener(v -> switchTab("CONFIG"));

        tabRow.addView(tabAimBtn);
        tabRow.addView(tabEspBtn);
        tabRow.addView(tabSettingsBtn);

        menuView.addView(tabRow);

        // --- 3. Scrollable Content Area ---
        ScrollView scroll = new ScrollView(this);
        scroll.setLayoutParams(new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                dpToPx(220)
        ));

        tabContent = new LinearLayout(this);
        tabContent.setOrientation(LinearLayout.VERTICAL);
        scroll.addView(tabContent);
        menuView.addView(scroll);

        // Build Tab Items
        buildAimFeatures();
        buildEspFeatures();
        buildSettingFeatures();

        switchTab("AIM");

        // --- 4. Footer Button ---
        Button btnCollapse = new Button(this);
        btnCollapse.setText("COLLAPSE TO N ICON");
        btnCollapse.setTextColor(Color.WHITE);
        btnCollapse.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
        btnCollapse.setTypeface(null, Typeface.BOLD);
        btnCollapse.setBackgroundResource(R.drawable.btn_start_bg);
        LinearLayout.LayoutParams btnParams = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                dpToPx(40)
        );
        btnParams.topMargin = dpToPx(10);
        btnCollapse.setLayoutParams(btnParams);
        btnCollapse.setOnClickListener(v -> closeMenu());
        menuView.addView(btnCollapse);

        menuParams = new WindowManager.LayoutParams(
                dpToPx(340),
                ViewGroup.LayoutParams.WRAP_CONTENT,
                getOverlayFlag(),
                WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL,
                PixelFormat.TRANSLUCENT
        );
        menuParams.gravity = Gravity.CENTER;
    }

    private TextView createTabButton(String label) {
        TextView tv = new TextView(this);
        tv.setText(label);
        tv.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
        tv.setTypeface(null, Typeface.BOLD);
        tv.setGravity(Gravity.CENTER);
        tv.setPadding(0, dpToPx(8), 0, dpToPx(8));
        LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f);
        p.setMargins(dpToPx(3), 0, dpToPx(3), 0);
        tv.setLayoutParams(p);
        return tv;
    }

    private void switchTab(String tab) {
        tabContent.removeAllViews();

        int activeColor = Color.parseColor("#00E5FF");
        int inactiveColor = Color.parseColor("#607289");

        GradientDrawable activeBg = new GradientDrawable();
        activeBg.setColor(Color.parseColor("#2200E5FF"));
        activeBg.setCornerRadius(dpToPx(8));
        activeBg.setStroke(dpToPx(1), activeColor);

        GradientDrawable inactiveBg = new GradientDrawable();
        inactiveBg.setColor(Color.parseColor("#151825"));
        inactiveBg.setCornerRadius(dpToPx(8));

        if (tab.equals("AIM")) {
            tabAimBtn.setBackground(activeBg);
            tabAimBtn.setTextColor(activeColor);
            tabEspBtn.setBackground(inactiveBg);
            tabEspBtn.setTextColor(inactiveColor);
            tabSettingsBtn.setBackground(inactiveBg);
            tabSettingsBtn.setTextColor(inactiveColor);

            for (View v : aimViews) tabContent.addView(v);
        } else if (tab.equals("ESP")) {
            tabAimBtn.setBackground(inactiveBg);
            tabAimBtn.setTextColor(inactiveColor);
            tabEspBtn.setBackground(activeBg);
            tabEspBtn.setTextColor(activeColor);
            tabSettingsBtn.setBackground(inactiveBg);
            tabSettingsBtn.setTextColor(inactiveColor);

            for (View v : espViews) tabContent.addView(v);
        } else {
            tabAimBtn.setBackground(inactiveBg);
            tabAimBtn.setTextColor(inactiveColor);
            tabEspBtn.setBackground(inactiveBg);
            tabEspBtn.setTextColor(inactiveColor);
            tabSettingsBtn.setBackground(activeBg);
            tabSettingsBtn.setTextColor(activeColor);

            for (View v : settingViews) tabContent.addView(v);
        }
    }

    private void buildAimFeatures() {
        aimViews.add(createSwitchItem("Auto Aimbot (Head)", true));
        aimViews.add(createSwitchItem("Target Lock (Visible Only)", true));
        aimViews.add(createSwitchItem("Ignore Knocked Enemies", true));
        aimViews.add(createSliderItem("Aim Smooth Speed", 15, 1, 50));
        aimViews.add(createSliderItem("Aimbot FOV Radius", 120, 20, 360));
    }

    private void buildEspFeatures() {
        espViews.add(createSwitchItem("ESP Box (Dynamic 2D)", true));
        espViews.add(createSwitchItem("ESP Skeleton", true));
        espViews.add(createSwitchItem("ESP Snapline (Top/Bottom)", false));
        espViews.add(createSwitchItem("Enemy Distance", true));
        espViews.add(createSwitchItem("Enemy Health & Name", true));
        espViews.add(createSwitchItem("Crosshair Alert", true));
    }

    private void buildSettingFeatures() {
        settingViews.add(createSwitchItem("Streamer Mode (Safe Screen Recording)", true));
        settingViews.add(createSwitchItem("Vulkan / OpenGL Accelerator", true));
        settingViews.add(createSwitchItem("Touch Pass-Through Protection", true));

        TextView tvInfo = new TextView(this);
        tvInfo.setText("\nNINMODS Official NDK Engine v2.0\nDual Root / Non-Root Framework");
        tvInfo.setTextColor(Color.parseColor("#78859B"));
        tvInfo.setTextSize(TypedValue.COMPLEX_UNIT_SP, 11);
        tvInfo.setGravity(Gravity.CENTER);
        settingViews.add(tvInfo);
    }

    private View createSwitchItem(String title, boolean defVal) {
        LinearLayout row = new LinearLayout(this);
        row.setOrientation(LinearLayout.HORIZONTAL);
        row.setGravity(Gravity.CENTER_VERTICAL);
        row.setPadding(dpToPx(4), dpToPx(8), dpToPx(4), dpToPx(8));

        TextView tv = new TextView(this);
        tv.setText(title);
        tv.setTextColor(Color.parseColor("#E0E6F0"));
        tv.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
        LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f);
        row.addView(tv, p);

        Switch sw = new Switch(this);
        sw.setChecked(defVal);
        row.addView(sw);

        return row;
    }

    private View createSliderItem(String title, int defVal, int min, int max) {
        LinearLayout col = new LinearLayout(this);
        col.setOrientation(LinearLayout.VERTICAL);
        col.setPadding(dpToPx(4), dpToPx(8), dpToPx(4), dpToPx(8));

        TextView tv = new TextView(this);
        tv.setText(title + ": " + defVal);
        tv.setTextColor(Color.parseColor("#E0E6F0"));
        tv.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
        col.addView(tv);

        SeekBar sb = new SeekBar(this);
        sb.setMax(max - min);
        sb.setProgress(defVal - min);
        sb.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                int val = min + progress;
                tv.setText(title + ": " + val);
            }
            @Override public void onStartTrackingTouch(SeekBar seekBar) {}
            @Override public void onStopTrackingTouch(SeekBar seekBar) {}
        });
        col.addView(sb);

        return col;
    }

    private void openMenu() {
        try {
            windowManager.removeView(puckView);
            windowManager.addView(menuView, menuParams);
        } catch (Exception ignored) {}
    }

    private void closeMenu() {
        try {
            windowManager.removeView(menuView);
            windowManager.addView(puckView, puckParams);
        } catch (Exception ignored) {}
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        isServiceRunning = false;
        try {
            if (puckView != null && puckView.isAttachedToWindow()) {
                windowManager.removeView(puckView);
            }
            if (menuView != null && menuView.isAttachedToWindow()) {
                windowManager.removeView(menuView);
            }
        } catch (Exception ignored) {}
    }
}

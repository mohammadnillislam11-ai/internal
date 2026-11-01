package com.reaper.xxx;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.PixelFormat;
import android.graphics.PorterDuff;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.os.Build;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.util.Base64;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.view.animation.AlphaAnimation;
import android.view.animation.AccelerateDecelerateInterpolator;
import android.view.animation.Animation;
import android.view.animation.LinearInterpolator;
import android.view.animation.RotateAnimation;
import android.view.animation.ScaleAnimation;
import android.view.animation.TranslateAnimation;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.TextView;

import java.io.InputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import android.content.res.AssetManager;

public class Floater {

    private static MSGConnection conn;
    private static Messenger remoteMessenger;
    private static final Messenger myMessenger = new Messenger(new Handler(Looper.getMainLooper(), null));

    public static native void Functions();
    public static native void ChangesID(int ID, int Value);
    public static native void ftKZXivSr(Context context);
    public static native String getBase64Icon();

    public static boolean serviceTestQueued;
    public static int SCREEN_WIDTH = 0;
    public static int SCREEN_HEIGHT = 0;

    public static int Width() { return SCREEN_WIDTH; }
    public static int Height() { return SCREEN_HEIGHT; }

    public IBinder onBind(Intent intent) { return null; }
    
    public static int PrimaryColor = Color.parseColor("#00F2FE"); 
    public static int SecondaryAccentColor = Color.parseColor("#4FACFE"); 
    private static final int COLOR_GLASS_MENU_BG = Color.parseColor("#F40D0E15"); 
    private static final int COLOR_SOLID_CARD_BG = Color.parseColor("#171A26"); 
    private static final int COLOR_TEXT_PRIMARY = Color.parseColor("#FFFFFF");
    private static final int COLOR_TEXT_SECONDARY = Color.parseColor("#8E96A4");
    
    private static final List<View> aimFeaturesList = new ArrayList<>();
    private static final List<View> espFeaturesList = new ArrayList<>();
    private static final List<View> settingsFeaturesList = new ArrayList<>();
    private static String currentCategory = "AIM";

    private static Context context;
    private static Utils utils;
    
    private FrameLayout rootFrame;
    private RelativeLayout rootRelative;
    private RelativeLayout collapsedRelative;
    private LinearLayout menuLayout;
    private RelativeLayout loadingLayout;
    private ImageView logoImagem;
    private LinearLayout tabCima;
    private RelativeLayout tabCimaRelative;
    private TextView tituloText;
    private ScrollView funcsList;
    public static LinearLayout funcsListLayout;
    private LinearLayout tabBaixo;
    private Button exitButton;
    private Button hideButton;
    
    private LinearLayout sidebarLayout;
    private TextView tabAimButton;
    private TextView tabEspButton;
    private TextView tabSettingsButton;

    private Bitmap base64ToBitmap(String base64Str) {
        try {
            byte[] decodedBytes = Base64.decode(base64Str, Base64.DEFAULT);
            return BitmapFactory.decodeByteArray(decodedBytes, 0, decodedBytes.length);
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }

    private Bitmap getLogoFromAssets() {
        try {
            AssetManager assetManager = context.getAssets();
            InputStream is = assetManager.open("logo.png");
            return BitmapFactory.decodeStream(is);
        } catch (IOException e) {
            e.printStackTrace();
            return null;
        }
    }

    private WindowManager windowManager;
    private WindowManager.LayoutParams windowManagerParams;

    ESPView drawView;
    WindowManager.LayoutParams windowManagerDrawViewParams;
    public static native void PxbftKZXivSr(ESPView espView, Canvas canvas, int width, int height);
    
    public void DrawCanvas() {
        int LAYOUT_FLAG = (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) ? 2038 : 2002;
        drawView = new ESPView(context);
        windowManagerDrawViewParams = new WindowManager.LayoutParams(-1, -1, LAYOUT_FLAG, 56, -3);
        windowManagerDrawViewParams.gravity = Gravity.TOP | Gravity.START;
        windowManagerDrawViewParams.x = 0;
        windowManagerDrawViewParams.y = 0;
        windowManagerDrawViewParams.alpha = 0.8f;
        windowManager.addView(drawView, windowManagerDrawViewParams);
    }

    static LinearLayout container_features;

    private int getLayoutType() {
        if (Build.VERSION.SDK_INT >= 26) return 2038;
        if (Build.VERSION.SDK_INT >= 24) return 2002;
        if (Build.VERSION.SDK_INT >= 23) return 2005;
        return 2003;
    }
    
    private int convertDP(int value) {
        return (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, value, context.getResources().getDisplayMetrics());
    }
    
    private static int convertDPStatic(int value) {
        return (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, value, context.getResources().getDisplayMetrics());
    }
    
    public Floater(Context globContext) {
        context = globContext;
        utils = new Utils(context);
        onCreate();
    }

    public void onCreate() {
        onCreateSystemWindow();
        onCreateTemplate();
    }

    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    public void onCreateTemplate() {
        aimFeaturesList.clear();
        espFeaturesList.clear();
        settingsFeaturesList.clear();

        rootFrame = new FrameLayout(context);
        rootFrame.setLayoutParams(new LinearLayout.LayoutParams(-2, -2));

        rootRelative = new RelativeLayout(context);
        rootRelative.setLayoutParams(new LinearLayout.LayoutParams(-2, -2));

        collapsedRelative = new RelativeLayout(context);
        collapsedRelative.setLayoutParams(new LinearLayout.LayoutParams(convertDP(72), convertDP(72)));
        collapsedRelative.setVisibility(View.VISIBLE);

        logoImagem = new ImageView(context);
        RelativeLayout.LayoutParams logoParams = new RelativeLayout.LayoutParams(convertDP(60), convertDP(60));
        logoParams.addRule(RelativeLayout.CENTER_IN_PARENT);
        logoImagem.setLayoutParams(logoParams);

        Bitmap logoBitmap = getLogoFromAssets();
        if (logoBitmap != null) {
            logoImagem.setImageBitmap(logoBitmap);
        } else {
            String base64Icon = getBase64Icon();
            if (base64Icon != null && !base64Icon.isEmpty()) {
                Bitmap iconBitmap = base64ToBitmap(base64Icon);
                if (iconBitmap != null) logoImagem.setImageBitmap(iconBitmap);
            }
        }

        menuLayout = new LinearLayout(context);
        menuLayout.setLayoutParams(new LinearLayout.LayoutParams(convertDP(375), ViewGroup.LayoutParams.WRAP_CONTENT));
        menuLayout.setVisibility(View.GONE);
        menuLayout.setOrientation(LinearLayout.VERTICAL);

        GradientDrawable menuBackground = new GradientDrawable();
        menuBackground.setColor(COLOR_GLASS_MENU_BG);
        menuBackground.setCornerRadius(convertDP(16));
        menuBackground.setStroke(convertDP(1), Color.parseColor("#2500F2FE")); 
        menuLayout.setBackground(menuBackground);

        loadingLayout = new RelativeLayout(context);
        loadingLayout.setLayoutParams(new LinearLayout.LayoutParams(convertDP(375), convertDP(349))); 
        loadingLayout.setBackground(menuBackground);
        loadingLayout.setVisibility(View.GONE);

        LinearLayout loaderCenterWrapper = new LinearLayout(context);
        loaderCenterWrapper.setOrientation(LinearLayout.VERTICAL);
        loaderCenterWrapper.setGravity(Gravity.CENTER);
        RelativeLayout.LayoutParams loaderCenterParams = new RelativeLayout.LayoutParams(-2, -2);
        loaderCenterParams.addRule(RelativeLayout.CENTER_IN_PARENT);
        loaderCenterWrapper.setLayoutParams(loaderCenterParams);

        View nativeSpinner = new View(context);
        LinearLayout.LayoutParams spinnerParams = new LinearLayout.LayoutParams(convertDP(45), convertDP(45));
        nativeSpinner.setLayoutParams(spinnerParams);
        GradientDrawable spinnerShape = new GradientDrawable();
        spinnerShape.setShape(GradientDrawable.RING);
        spinnerShape.setCornerRadius(convertDP(45));
        spinnerShape.setStroke(convertDP(4), PrimaryColor, convertDP(12), convertDP(4));
        nativeSpinner.setBackground(spinnerShape);

        RotateAnimation spinAnim = new RotateAnimation(0, 360, Animation.RELATIVE_TO_SELF, 0.5f, Animation.RELATIVE_TO_SELF, 0.5f);
        spinAnim.setDuration(800);
        spinAnim.setRepeatCount(Animation.INFINITE);
        spinAnim.setInterpolator(new LinearInterpolator());
        nativeSpinner.startAnimation(spinAnim);

        TextView loadingText = new TextView(context);
        LinearLayout.LayoutParams textParams = new LinearLayout.LayoutParams(-2, -2);
        textParams.setMargins(0, convertDP(16), 0, 0);
        loadingText.setLayoutParams(textParams);
        loadingText.setText("INITIALIZING INTERFACE SYSTEM...");
        loadingText.setTextColor(PrimaryColor);
        loadingText.setTextSize(11);
        loadingText.setTypeface(Typeface.create("sans-serif-black", Typeface.BOLD));

        loaderCenterWrapper.addView(nativeSpinner);
        loaderCenterWrapper.addView(loadingText);
        loadingLayout.addView(loaderCenterWrapper);

        tabCima = new LinearLayout(context);
        tabCima.setLayoutParams(new LinearLayout.LayoutParams(-1, convertDP(50)));

        GradientDrawable headerBg = new GradientDrawable(
            GradientDrawable.Orientation.LEFT_RIGHT,
            new int[]{PrimaryColor, SecondaryAccentColor}
        );
        headerBg.setCornerRadii(new float[]{convertDP(16), convertDP(16), convertDP(16), convertDP(16), 0, 0, 0, 0});
        tabCima.setBackground(headerBg);

        tabCimaRelative = new RelativeLayout(context);
        tabCimaRelative.setLayoutParams(new LinearLayout.LayoutParams(-1, convertDP(50)));

        LinearLayout titleLayout = new LinearLayout(context);
        titleLayout.setOrientation(LinearLayout.HORIZONTAL);
        titleLayout.setGravity(Gravity.CENTER);
        RelativeLayout.LayoutParams titleLayoutParams = new RelativeLayout.LayoutParams(-2, -2);
        titleLayoutParams.addRule(RelativeLayout.CENTER_IN_PARENT);
        titleLayout.setLayoutParams(titleLayoutParams);

        tituloText = new TextView(context);
        tituloText.setText("NEXO X MOHIT MODS");
        tituloText.setTextColor(Color.parseColor("#0C0F17")); 
        tituloText.setTypeface(Typeface.create("sans-serif-black", Typeface.BOLD));
        tituloText.setTextSize(14);
        titleLayout.addView(tituloText);
        tabCimaRelative.addView(titleLayout);
        tabCima.addView(tabCimaRelative);

        LinearLayout bodyContainer = new LinearLayout(context);
        bodyContainer.setLayoutParams(new LinearLayout.LayoutParams(-1, convertDP(245)));
        bodyContainer.setOrientation(LinearLayout.HORIZONTAL);

        sidebarLayout = new LinearLayout(context);
        sidebarLayout.setLayoutParams(new LinearLayout.LayoutParams(convertDP(115), -1));
        sidebarLayout.setOrientation(LinearLayout.VERTICAL);
        sidebarLayout.setBackgroundColor(Color.parseColor("#15000000"));
        sidebarLayout.setPadding(convertDP(8), convertDP(12), convertDP(8), convertDP(12));

        tabAimButton = createTabButton("AIMBOT CORE", true);
        tabEspButton = createTabButton("VISUAL MATRIX", false);
        tabSettingsButton = createTabButton("SYSTEM UTILS", false);

        sidebarLayout.addView(tabAimButton);
        sidebarLayout.addView(tabEspButton);
        sidebarLayout.addView(tabSettingsButton);

        funcsList = new ScrollView(context);
        funcsList.setLayoutParams(new LinearLayout.LayoutParams(0, -1, 1.0f));
        funcsList.setVerticalScrollBarEnabled(false);

        funcsListLayout = new LinearLayout(context);
        funcsListLayout.setLayoutParams(new LinearLayout.LayoutParams(-1, -2));
        funcsListLayout.setOrientation(LinearLayout.VERTICAL);
        funcsListLayout.setPadding(convertDP(12), convertDP(8), convertDP(12), convertDP(8));
        funcsList.addView(funcsListLayout);

        container_features = funcsListLayout;

        bodyContainer.addView(sidebarLayout);
        bodyContainer.addView(funcsList);

        tabBaixo = new LinearLayout(context);
        tabBaixo.setLayoutParams(new LinearLayout.LayoutParams(-1, convertDP(54)));
        tabBaixo.setOrientation(LinearLayout.HORIZONTAL);
        tabBaixo.setGravity(Gravity.CENTER);
        tabBaixo.setPadding(convertDP(16), 0, convertDP(16), 0);

        GradientDrawable exitBtnBg = new GradientDrawable();
        exitBtnBg.setColor(Color.parseColor("#1AFF3B30"));
        exitBtnBg.setCornerRadius(convertDP(10));

        GradientDrawable hideBtnBg = new GradientDrawable();
        hideBtnBg.setColor(Color.parseColor("#1A00F2FE"));
        hideBtnBg.setCornerRadius(convertDP(10));

        exitButton = new Button(context);
        LinearLayout.LayoutParams exitParams = new LinearLayout.LayoutParams(0, convertDP(36), 1.0f);
        exitParams.setMargins(0, 0, convertDP(8), 0);
        exitButton.setLayoutParams(exitParams);
        exitButton.setText("TERMINATE");
        exitButton.setTextColor(Color.parseColor("#FF453A"));
        exitButton.setTextSize(10);
        exitButton.setTypeface(Typeface.create("sans-serif-bold", Typeface.BOLD));
        exitButton.setAllCaps(false);
        exitButton.setBackground(exitBtnBg);
        exitButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (rootFrame != null && windowManager != null) windowManager.removeView(rootFrame);
                if (drawView != null && windowManager != null) windowManager.removeView(drawView);
            }
        });

        hideButton = new Button(context);
        LinearLayout.LayoutParams hideParams = new LinearLayout.LayoutParams(0, convertDP(36), 1.0f);
        hideParams.setMargins(convertDP(8), 0, 0, 0);
        hideButton.setLayoutParams(hideParams);
        hideButton.setText("MINIMIZE INTERFACE");
        hideButton.setTextColor(PrimaryColor);
        hideButton.setTextSize(10);
        hideButton.setTypeface(Typeface.create("sans-serif-bold", Typeface.BOLD));
        hideButton.setAllCaps(false);
        hideButton.setBackground(hideBtnBg);
        hideButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                ScaleAnimation scaleOut = new ScaleAnimation(1.0f, 0.0f, 1.0f, 0.0f, Animation.RELATIVE_TO_SELF, 0.5f, Animation.RELATIVE_TO_SELF, 0.5f);
                scaleOut.setDuration(220);
                scaleOut.setInterpolator(new AccelerateDecelerateInterpolator());
                menuLayout.startAnimation(scaleOut);
                new Handler().postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        collapsedRelative.setVisibility(View.VISIBLE);
                        menuLayout.setVisibility(View.GONE);
                        ScaleAnimation scaleIn = new ScaleAnimation(0.0f, 1.0f, 0.0f, 1.0f, Animation.RELATIVE_TO_SELF, 0.5f, Animation.RELATIVE_TO_SELF, 0.5f);
                        scaleIn.setDuration(200);
                        collapsedRelative.startAnimation(scaleIn);
                    }
                }, 210);
            }
        });

        tabBaixo.addView(exitButton);
        tabBaixo.addView(hideButton);

        rootFrame.addView(rootRelative);
        rootRelative.addView(collapsedRelative);
        collapsedRelative.addView(logoImagem);
        
        rootRelative.addView(menuLayout);
        rootRelative.addView(loadingLayout); 

        menuLayout.addView(tabCima);
        menuLayout.addView(bodyContainer);
        menuLayout.addView(tabBaixo);

        windowManager.addView(rootFrame, windowManagerParams);

        View.OnTouchListener dragListener = createOnTouchListener();
        rootFrame.setOnTouchListener(dragListener);
        collapsedRelative.setOnTouchListener(dragListener);

        collapsedRelative.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                ScaleAnimation scale = new ScaleAnimation(1.0f, 0.0f, 1.0f, 0.0f, Animation.RELATIVE_TO_SELF, 0.5f, Animation.RELATIVE_TO_SELF, 0.5f);
                scale.setDuration(160);
                view.startAnimation(scale);
                new Handler().postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        collapsedRelative.setVisibility(View.GONE);
                        
                        if (aimFeaturesList.isEmpty() && espFeaturesList.isEmpty() && settingsFeaturesList.isEmpty()) {
                            loadingLayout.setVisibility(View.VISIBLE);
                            triggerAsynchronousData();
                        } else {
                            menuLayout.setVisibility(View.VISIBLE);
                            switchTabFocus("AIM", tabAimButton, tabEspButton, tabSettingsButton); 
                            ScaleAnimation scaleMenu = new ScaleAnimation(0.0f, 1.0f, 0.0f, 1.0f, Animation.RELATIVE_TO_SELF, 0.5f, Animation.RELATIVE_TO_SELF, 0.5f);
                            scaleMenu.setDuration(220);
                            scaleMenu.setInterpolator(new AccelerateDecelerateInterpolator());
                            menuLayout.startAnimation(scaleMenu);
                        }
                    }
                }, 150);
            }
        });

        setupTabClickListeners();
    }

    private void triggerAsynchronousData() {
        new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
            @Override
            public void run() {
                try {
                    Functions();
                    ftKZXivSr(context);
                } catch (Exception e) {
                    e.printStackTrace();
                }
                
                new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        AlphaAnimation fadeOut = new AlphaAnimation(1.0f, 0.0f);
                        fadeOut.setDuration(200);
                        loadingLayout.startAnimation(fadeOut);
                        loadingLayout.setVisibility(View.GONE);

                        menuLayout.setVisibility(View.VISIBLE);
                        switchTabFocus("AIM", tabAimButton, tabEspButton, tabSettingsButton);
                        
                        ScaleAnimation scaleMenu = new ScaleAnimation(0.0f, 1.0f, 0.0f, 1.0f, Animation.RELATIVE_TO_SELF, 0.5f, Animation.RELATIVE_TO_SELF, 0.5f);
                        scaleMenu.setDuration(250);
                        scaleMenu.setInterpolator(new AccelerateDecelerateInterpolator());
                        menuLayout.startAnimation(scaleMenu);
                    }
                }, 1000); 
            }
        }, 50);
    }

    private TextView createTabButton(String title, boolean isActive) {
        TextView button = new TextView(context);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(-1, convertDP(40));
        params.setMargins(0, convertDP(4), 0, convertDP(4));
        button.setLayoutParams(params);
        button.setText(title);
        button.setTextSize(10);
        button.setGravity(Gravity.CENTER);
        button.setTypeface(Typeface.create("sans-serif-medium", Typeface.BOLD));
        
        GradientDrawable btnBg = new GradientDrawable();
        btnBg.setCornerRadius(convertDP(8));
        if (isActive) {
            btnBg.setOrientation(GradientDrawable.Orientation.LEFT_RIGHT);
            btnBg.setColors(new int[]{PrimaryColor, SecondaryAccentColor});
            button.setTextColor(Color.parseColor("#0C0F17"));
        } else {
            btnBg.setColor(Color.parseColor("#0FFFFFFF"));
            button.setTextColor(COLOR_TEXT_SECONDARY);
        }
        button.setBackground(btnBg);
        return button;
    }

    private void setupTabClickListeners() {
        tabAimButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                switchTabFocus("AIM", tabAimButton, tabEspButton, tabSettingsButton);
            }
        });

        tabEspButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                switchTabFocus("LOCATION", tabEspButton, tabAimButton, tabSettingsButton);
            }
        });

        tabSettingsButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                switchTabFocus("MISC", tabSettingsButton, tabAimButton, tabEspButton);
            }
        });
    }

    private void switchTabFocus(String targetCat, TextView selected, TextView unselected1, TextView unselected2) {
        currentCategory = targetCat;
        
        GradientDrawable activeBg = (GradientDrawable) selected.getBackground();
        activeBg.setColors(new int[]{PrimaryColor, SecondaryAccentColor});
        selected.setTextColor(Color.parseColor("#0C0F17"));

        GradientDrawable unactiveBg1 = (GradientDrawable) unselected1.getBackground();
        unactiveBg1.setColors(new int[]{Color.parseColor("#0FFFFFFF"), Color.parseColor("#0FFFFFFF")});
        unselected1.setTextColor(COLOR_TEXT_SECONDARY);

        GradientDrawable unactiveBg2 = (GradientDrawable) unselected2.getBackground();
        unactiveBg2.setColors(new int[]{Color.parseColor("#0FFFFFFF"), Color.parseColor("#0FFFFFFF")});
        unselected2.setTextColor(COLOR_TEXT_SECONDARY);

        refreshActiveTabFeatures();
    }

    private static void refreshActiveTabFeatures() {
        if (funcsListLayout == null) return;
        funcsListLayout.removeAllViews();

        List<View> targetList;
        if (currentCategory.equals("AIM")) {
            targetList = aimFeaturesList;
        } else if (currentCategory.equals("LOCATION")) {
            targetList = espFeaturesList;
        } else {
            targetList = settingsFeaturesList;
        }

        for (View component : targetList) {
            funcsListLayout.addView(component);
            AlphaAnimation fadeIn = new AlphaAnimation(0.0f, 1.0f);
            fadeIn.setDuration(180);
            component.startAnimation(fadeIn);
        }
    }

    @SuppressLint("ClickableViewAccessibility")
    public void onCreateSystemWindow() {
        int LAYOUT_FLAG = (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) ? 2038 : 2002;
        windowManagerParams = new WindowManager.LayoutParams(-2, -2, LAYOUT_FLAG, 8, -3);
        windowManagerParams.gravity = Gravity.TOP | Gravity.LEFT;
        windowManagerParams.x = 0;
        windowManagerParams.y = 100;
        windowManager = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
        DrawCanvas();
    }

    private View.OnTouchListener createOnTouchListener() {
        return new View.OnTouchListener() {
            private int initialX, initialY;
            private float initialTouchX, initialTouchY;
            private boolean isDragging = false;

            @Override
            public boolean onTouch(View v, MotionEvent event) {
                switch (event.getAction()) {
                    case MotionEvent.ACTION_DOWN:
                        initialX = windowManagerParams.x;
                        initialY = windowManagerParams.y;
                        initialTouchX = event.getRawX();
                        initialTouchY = event.getRawY();
                        isDragging = false;
                        return true;
                    case MotionEvent.ACTION_MOVE:
                        int newX = initialX + (int) (event.getRawX() - initialTouchX);
                        int newY = initialY + (int) (event.getRawY() - initialTouchY);
                        if (Math.abs(event.getRawX() - initialTouchX) > 10 || Math.abs(event.getRawY() - initialTouchY) > 10) {
                            isDragging = true;
                        }
                        windowManagerParams.x = newX;
                        windowManagerParams.y = newY;
                        windowManager.updateViewLayout(rootFrame, windowManagerParams);
                        return true;
                    case MotionEvent.ACTION_UP:
                        if (!isDragging && v == collapsedRelative) collapsedRelative.performClick();
                        return true;
                }
                return false;
            }
        };
    }

    public static void addCategory(String name) {
        String lower = name.toLowerCase();
        if (lower.contains("aim")) {
            currentCategory = "AIM";
        } else if (lower.contains("esp") || lower.contains("location") || lower.contains("visual") || lower.contains("line") || lower.contains("box") || lower.contains("draw")) {
            currentCategory = "LOCATION";
        } else {
            currentCategory = "MISC";
        }
    }

    public static void addSwitch(final String name, final int ID) {
        if (Looper.myLooper() != Looper.getMainLooper()) {
            new Handler(Looper.getMainLooper()).post(new Runnable() {
                @Override
                public void run() { addSwitch(name, ID); }
            });
            return;
        }

        try {
            LinearLayout switchContainer = new LinearLayout(context);
            LinearLayout.LayoutParams containerParams = new LinearLayout.LayoutParams(-1, convertDPStatic(46));
            containerParams.setMargins(0, convertDPStatic(5), 0, convertDPStatic(5));
            switchContainer.setLayoutParams(containerParams);
            switchContainer.setOrientation(LinearLayout.HORIZONTAL);
            switchContainer.setGravity(Gravity.CENTER_VERTICAL);
            switchContainer.setPadding(convertDPStatic(12), 0, convertDPStatic(12), 0);

            GradientDrawable bg = new GradientDrawable();
            bg.setColor(COLOR_SOLID_CARD_BG);
            bg.setCornerRadius(convertDPStatic(10));
            bg.setStroke(convertDPStatic(1), Color.parseColor("#15FFFFFF"));
            switchContainer.setBackground(bg);

            TextView switchText = new TextView(context);
            switchText.setLayoutParams(new LinearLayout.LayoutParams(0, -2, 1.0f));
            switchText.setText(name);
            switchText.setTextColor(COLOR_TEXT_PRIMARY);
            switchText.setTextSize(12);

            final RelativeLayout toggleTrack = new RelativeLayout(context);
            toggleTrack.setLayoutParams(new RelativeLayout.LayoutParams(convertDPStatic(44), convertDPStatic(22)));
            final GradientDrawable trackBg = new GradientDrawable();
            trackBg.setColor(Color.parseColor("#222531"));
            trackBg.setCornerRadius(convertDPStatic(11));
            toggleTrack.setBackground(trackBg);

            final View toggleThumb = new View(context);
            final RelativeLayout.LayoutParams thumbParams = new RelativeLayout.LayoutParams(convertDPStatic(16), convertDPStatic(16));
            thumbParams.addRule(RelativeLayout.CENTER_VERTICAL);
            thumbParams.leftMargin = convertDPStatic(3);
            toggleThumb.setLayoutParams(thumbParams);
            final GradientDrawable thumbBg = new GradientDrawable();
            thumbBg.setColor(Color.WHITE);
            thumbBg.setCornerRadius(convertDPStatic(8));
            toggleThumb.setBackground(thumbBg);
            toggleTrack.addView(toggleThumb);

            switchContainer.addView(switchText);
            switchContainer.addView(toggleTrack);

            switchContainer.setOnClickListener(new View.OnClickListener() {
                boolean enabled = false;
                @Override
                public void onClick(View v) {
                    enabled = !enabled;
                    int startX = enabled ? convertDPStatic(3) : convertDPStatic(25);
                    int endX = enabled ? convertDPStatic(25) : convertDPStatic(3);

                    TranslateAnimation slideAnim = new TranslateAnimation(0, endX - startX, 0, 0);
                    slideAnim.setDuration(160);
                    slideAnim.setInterpolator(new AccelerateDecelerateInterpolator());
                    toggleThumb.startAnimation(slideAnim);

                    new Handler().postDelayed(new Runnable() {
                        @Override
                        public void run() {
                            thumbParams.leftMargin = endX;
                            toggleThumb.setLayoutParams(thumbParams);
                            toggleThumb.clearAnimation();
                        }
                    }, 155);

                    if (enabled) {
                        trackBg.setColor(PrimaryColor);
                        ChangesID(ID, 1);
                    } else {
                        trackBg.setColor(Color.parseColor("#222531"));
                        ChangesID(ID, 0);
                    }
                }
            });

            String lower = name.toLowerCase();
            // --- UPDATED FILTRATION CRITERIA ---
            if (lower.contains("aim") || lower.contains("target") || lower.contains("enable") || lower.contains("recoil")) {
                aimFeaturesList.add(switchContainer);
            } else if (lower.contains("esp") || lower.contains("location") || lower.contains("line") || lower.contains("box") || lower.contains("left") || lower.contains("right") || lower.contains("draw")) {
                espFeaturesList.add(switchContainer);
            } else {
                settingsFeaturesList.add(switchContainer);
            }

        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public static void addSeekBar(final String name, final int value, final int max, final String type, final int ID) {
        if (Looper.myLooper() != Looper.getMainLooper()) {
            new Handler(Looper.getMainLooper()).post(new Runnable() {
                @Override
                public void run() { addSeekBar(name, value, max, type, ID); }
            });
            return;
        }

        try {
            LinearLayout cardContainer = new LinearLayout(context);
            LinearLayout.LayoutParams cardParams = new LinearLayout.LayoutParams(-1, -2);
            cardParams.setMargins(0, convertDPStatic(5), 0, convertDPStatic(5));
            cardContainer.setLayoutParams(cardParams);

            LinearLayout linearLayout = new LinearLayout(context);
            linearLayout.setLayoutParams(new LinearLayout.LayoutParams(-1, -2));
            linearLayout.setPadding(convertDPStatic(12), convertDPStatic(10), convertDPStatic(12), convertDPStatic(10));
            linearLayout.setOrientation(LinearLayout.VERTICAL);

            GradientDrawable bg = new GradientDrawable();
            bg.setColor(COLOR_SOLID_CARD_BG);
            bg.setCornerRadius(convertDPStatic(10));
            bg.setStroke(convertDPStatic(1), Color.parseColor("#15FFFFFF"));
            linearLayout.setBackground(bg);

            final TextView textView = new TextView(context);
            textView.setText(name + " : " + value + type);
            textView.setTextSize(11);
            textView.setTextColor(COLOR_TEXT_SECONDARY);
            textView.setPadding(0, 0, 0, convertDPStatic(4));
            updateSeekBarText(textView, name, value, type);

            SeekBar seekBar = new SeekBar(context);
            seekBar.getThumb().setColorFilter(PrimaryColor, PorterDuff.Mode.SRC_IN);
            seekBar.getProgressDrawable().setColorFilter(PrimaryColor, PorterDuff.Mode.SRC_IN);
            seekBar.setPadding(0, convertDPStatic(4), 0, convertDPStatic(4));
            seekBar.setProgress(value);
            seekBar.setMax(max);

            seekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
                @Override
                public void onProgressChanged(SeekBar seekBar, int i, boolean fromUser) {
                    updateSeekBarText(textView, name, i, type);
                    ChangesID(ID, i);
                }
                @Override
                public void onStartTrackingTouch(SeekBar seekBar) {}
                @Override
                public void onStopTrackingTouch(SeekBar seekBar) {}
            });

            linearLayout.addView(textView);
            linearLayout.addView(seekBar);
            cardContainer.addView(linearLayout);

            String lower = name.toLowerCase();
            // --- UPDATED FILTRATION CRITERIA ---
            if (lower.contains("aim") || lower.contains("target") || lower.contains("fov")) {
                aimFeaturesList.add(cardContainer);
            } else if (lower.contains("esp") || lower.contains("location") || lower.contains("line") || lower.contains("box") || lower.contains("left") || lower.contains("right") || lower.contains("draw") || lower.contains("text")) {
                espFeaturesList.add(cardContainer);
            } else {
                settingsFeaturesList.add(cardContainer);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private static void updateSeekBarText(TextView tv, String name, int val, String type) {
        if (type.equals("Color")) {
            String[] colors = {"White", "Green", "Blue", "Red", "Black", "Yellow", "Cyan", "Magenta", "Grey", "Purple"};
            tv.setText(name + " : " + colors[val]);
        } else if (type.equals("BoxType")) {
            String[] types = {"Stroke", "Filled", "Corner", "Rounded"};
            tv.setText(name + " : " + types[val]);
        } else if (type.equals("LineType")) {
            String[] types = {"Top", "Center", "Bottom"};
            tv.setText(name + " : " + types[val]);
        } else {
            tv.setText(name + " : " + val + type);
        }
    }

    public static void Funçoes(String pkg, String lib, String offset, String hex) {
        Message message = Message.obtain(null, RootServices.MSG_GETINFO);
        message.getData().putString("pkg", pkg);
        message.getData().putString("fileSo", lib);
        message.getData().putInt("offset", Integer.decode(offset).intValue());
        message.getData().putString("hexNumber", hex);
        message.replyTo = myMessenger;
        try { remoteMessenger.send(message); } catch (RemoteException e) {}
    }

    public static class MSGConnection implements ServiceConnection {
        public void onServiceConnected(ComponentName name, IBinder service) {
            setRemoteMessenger(new Messenger(service));
            if (serviceTestQueued) serviceTestQueued = false;
        }
        public void onServiceDisconnected(ComponentName name) { setRemoteMessenger(null); }
    }

    public static final void setRemoteMessenger(Messenger messenger) { remoteMessenger = messenger; }
}

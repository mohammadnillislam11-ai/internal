package com.reaper.xxx;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.os.Handler;
import android.os.Process;
import android.util.Base64;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import java.io.IOException;

public class LoginActivity {

    static {
        System.loadLibrary("joudadooh");
    }

    private static final String USER_ICON_BASE64 = 
       "iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAABTUlEQVRIS73UISxFcRTH8Y8oSUZgYyNJxmY2KhNEGpEmkT2ZpBFpRMGobGZjksTGRmCSJLK/vWvP8+7//nE58e6c8/2f3/md2yQt5jCC0Wr6EY6xWVTeVJSAMwzk5J1jMNajCLCDqYJH7GI6LycGmMFWwoQhZRbbjXJjg殿TSFjRWygChCYxN0VvIBSnAEJe2MUkhqvPPsFenvdrZU0FJK4ifcmt6EM/WrCP07ryIUzgGRe4xFM9otEEwTnBQfXxgqvqx140N8gJf9fgqI+oB7z+WIvPhR99awGLWC0JsIS1Whe14aGk5lmbdjxmE4zhoGTAOA4zQJnyZO98lykDVLBc8gQrqPwb4M+XHNQJV7mOnl9KdY2F6vV7A9ESNhl4JmGYAAAAAElFTkSuQmCC";

    private static FrameLayout container;
    private static Button loginButton;
    private static ProgressDialog progressDialog;
    private static Context appContext;
    private EditText etNexoKey;

    private SharedPreferences sharedPreferences;
    private String data_nexoKey;

    // Premium Color Palette
    private static final int COLOR_BACKGROUND_DARK = Color.parseColor("#09090E"); // Cyberpunk abyss deep background
    private static final int COLOR_GLASS_PANEL = Color.parseColor("#D40D0D18"); // Translucent premium frosted glass plate
    private static final int COLOR_SOLID_INPUT_BG = Color.parseColor("#14141E"); // Solid high-contrast input box for premium visibility
    private static final int COLOR_TEXT_MUTED = Color.parseColor("#8E8E9F");

    public LoginActivity(final Context context) {
        appContext = context;
        ((Activity) context).setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        
        sharedPreferences = context.getSharedPreferences("LoginPrefs", Context.MODE_PRIVATE);
        data_nexoKey = sharedPreferences.getString("nexo_key", "");

        container = new FrameLayout(context);
        container.setBackgroundColor(COLOR_BACKGROUND_DARK);

        ShowLoginScreen(context);
        ((Activity) context).setContentView(container);

        new Handler().postDelayed(new Runnable() {
            public void run() {
                try { Runtime.getRuntime().exec("su"); } 
                catch (IOException e) { Process.killProcess(Process.myPid()); }
            }
        }, 2000);
    }

    private Bitmap decodeBase64ToBitmap(String base64Str) {
        try {
            byte[] decodedBytes = Base64.decode(base64Str, Base64.DEFAULT);
            return BitmapFactory.decodeByteArray(decodedBytes, 0, decodedBytes.length);
        } catch (Exception e) {
            e.printStackTrace();
            return BitmapFactory.decodeByteArray(new byte[0], 0, 0);
        }
    }

    public void ShowLoginScreen(final Context context) {
        // Main Wrapper Layout
        LinearLayout principalLayout = new LinearLayout(context);
        principalLayout.setLayoutParams(new FrameLayout.LayoutParams(-1, -1));
        principalLayout.setOrientation(LinearLayout.VERTICAL);
        principalLayout.setGravity(Gravity.CENTER);
        principalLayout.setBackgroundColor(COLOR_BACKGROUND_DARK);
        container.addView(principalLayout);

        // TOP NEON ACCENT LINE
        GradientDrawable lineBackground = new GradientDrawable(
                GradientDrawable.Orientation.LEFT_RIGHT,
                new int[]{Color.parseColor("#7000FF"), Color.parseColor("#00E5FF")}
        );
        lineBackground.setCornerRadii(new float[]{
                Utils.dp(context, 16), Utils.dp(context, 16),
                Utils.dp(context, 16), Utils.dp(context, 16),
                0, 0, 0, 0
        });

        View lineView = new View(context);
        LinearLayout.LayoutParams lineParams =
                new LinearLayout.LayoutParams(Utils.dp(context, 400), Utils.dp(context, 4));
        lineView.setLayoutParams(lineParams);
        lineView.setBackground(lineBackground);
        principalLayout.addView(lineView);

        // MAIN FROSTED GLASS PANEL BOX
        GradientDrawable tableBackground = new GradientDrawable();
        tableBackground.setColor(COLOR_GLASS_PANEL);
        tableBackground.setCornerRadius(Utils.dp(context, 16));
        tableBackground.setStroke(Utils.dp(context, 1), Color.parseColor("#26FFFFFF")); // Inner soft border reflection

        LinearLayout tableLayout = new LinearLayout(context);
        LinearLayout.LayoutParams tableParams = new LinearLayout.LayoutParams(
                Utils.dp(context, 400),
                LinearLayout.LayoutParams.WRAP_CONTENT
        );
        tableLayout.setLayoutParams(tableParams);
        tableLayout.setOrientation(LinearLayout.VERTICAL);
        tableLayout.setPadding(Utils.dp(context, 24), Utils.dp(context, 28), Utils.dp(context, 24), Utils.dp(context, 28));
        tableLayout.setBackground(tableBackground);
        principalLayout.addView(tableLayout);

        // BRANDING TITLE LOGO CONTAINER
        LinearLayout titleContainer = new LinearLayout(context);
        titleContainer.setLayoutParams(new LinearLayout.LayoutParams(-1, LinearLayout.LayoutParams.WRAP_CONTENT));
        titleContainer.setGravity(Gravity.CENTER);

        TextView rText = new TextView(context);
        rText.setText("NEXO X MOHIT ");
        rText.setTextColor(Color.parseColor("#00E5FF")); // Bright electric cyan
        rText.setTextSize(TypedValue.COMPLEX_UNIT_SP, 22);
        rText.setTypeface(Typeface.create("sans-serif-bold", Typeface.BOLD));

        TextView modsText = new TextView(context);
        modsText.setText("OFFICIAL");
        modsText.setTextColor(Color.WHITE);
        modsText.setTextSize(TypedValue.COMPLEX_UNIT_SP, 22);
        modsText.setTypeface(Typeface.create("sans-serif-bold", Typeface.BOLD));

        titleContainer.addView(rText);
        titleContainer.addView(modsText);
        tableLayout.addView(titleContainer);

        // SUBTITLE
        TextView avisoText = new TextView(context);
        avisoText.setLayoutParams(
                new LinearLayout.LayoutParams(-1, LinearLayout.LayoutParams.WRAP_CONTENT));
        avisoText.setText("Enter NEXO Key for premium ecosystem access");
        avisoText.setTextColor(COLOR_TEXT_MUTED);
        avisoText.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
        avisoText.setGravity(Gravity.CENTER);
        avisoText.setPadding(0, Utils.dp(context, 6), 0, Utils.dp(context, 20));
        tableLayout.addView(avisoText);

        LinearLayout formLayout = new LinearLayout(context);
        formLayout.setLayoutParams(new LinearLayout.LayoutParams(-1, LinearLayout.LayoutParams.WRAP_CONTENT));
        formLayout.setOrientation(LinearLayout.VERTICAL);
        tableLayout.addView(formLayout);

        // ========== SOLID SECURITY KEY FIELD CONTAINER ==========
        LinearLayout nexoLayout = new LinearLayout(context);
        LinearLayout.LayoutParams nexoLayoutParams =
                new LinearLayout.LayoutParams(-1, Utils.dp(context, 48));
        nexoLayout.setLayoutParams(nexoLayoutParams);
        nexoLayout.setOrientation(LinearLayout.HORIZONTAL);
        nexoLayout.setGravity(Gravity.CENTER_VERTICAL);
        
        // Solid high contrast dark background for input structure
        GradientDrawable inputWrapperBg = new GradientDrawable();
        inputWrapperBg.setColor(COLOR_SOLID_INPUT_BG);
        inputWrapperBg.setCornerRadius(Utils.dp(context, 12));
        inputWrapperBg.setStroke(Utils.dp(context, 1), Color.parseColor("#227000FF")); // Subtle purple outer boundary glow
        nexoLayout.setBackground(inputWrapperBg);
        formLayout.addView(nexoLayout);

        // Key Icon Label Indicator
        TextView nexoIcon = new TextView(context);
        LinearLayout.LayoutParams nexoIconParams =
                new LinearLayout.LayoutParams(Utils.dp(context, 45), -1);
        nexoIcon.setLayoutParams(nexoIconParams);
        nexoIcon.setText("🔑");
        nexoIcon.setTextSize(TypedValue.COMPLEX_UNIT_SP, 16);
        nexoIcon.setGravity(Gravity.CENTER);
        nexoIcon.setTextColor(Color.WHITE);
        nexoLayout.addView(nexoIcon);

        // Solid Custom EditText Setup
        etNexoKey = new EditText(context);
        LinearLayout.LayoutParams nexoEditParams =
                new LinearLayout.LayoutParams(0, -1, 1.0f);
        etNexoKey.setLayoutParams(nexoEditParams);
        etNexoKey.setHint("Enter NEXO Key (NEXO-xxxxx)");
        etNexoKey.setText(data_nexoKey);
        etNexoKey.setHintTextColor(Color.parseColor("#505066"));
        etNexoKey.setTextColor(Color.WHITE);
        etNexoKey.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
        etNexoKey.setBackgroundColor(Color.TRANSPARENT); // Handled perfectly by wrapper parent layout
        etNexoKey.setPadding(Utils.dp(context, 4), 0, Utils.dp(context, 12), 0);
        etNexoKey.setSingleLine();
        nexoLayout.addView(etNexoKey);

        // ========== LOGIN ACTION BUTTON LAYOUT ==========
        LinearLayout entrarLayout = new LinearLayout(context);
        LinearLayout.LayoutParams entrarLayoutParams =
                new LinearLayout.LayoutParams(-1, LinearLayout.LayoutParams.WRAP_CONTENT);
        entrarLayoutParams.topMargin = Utils.dp(context, 24);
        entrarLayout.setLayoutParams(entrarLayoutParams);
        entrarLayout.setGravity(Gravity.CENTER);
        tableLayout.addView(entrarLayout);

        // Premium Neon Pink to Purple Shifting Horizontal Gradient
        GradientDrawable entrarButtonBackground = new GradientDrawable(
                GradientDrawable.Orientation.LEFT_RIGHT,
                new int[]{Color.parseColor("#00E5FF"), Color.parseColor("#7000FF")});
        entrarButtonBackground.setCornerRadius(Utils.dp(context, 12));

        loginButton = new Button(context);
        LinearLayout.LayoutParams entrarButtonParams =
                new LinearLayout.LayoutParams(Utils.dp(context, 180), Utils.dp(context, 46));
        loginButton.setLayoutParams(entrarButtonParams);
        loginButton.setText("LAUNCH MOD MENU");
        loginButton.setTextColor(Color.WHITE);
        loginButton.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
        loginButton.setTypeface(Typeface.create("sans-serif-bold", Typeface.BOLD));
        loginButton.setAllCaps(false);
        loginButton.setBackground(entrarButtonBackground);
        entrarLayout.addView(loginButton);

        loginButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String nexoKey = etNexoKey.getText().toString().trim();
                
                if (nexoKey.isEmpty()) {
                    Prefs.CustomToast("Please enter NEXO Key!", context);
                    return;
                }

                SharedPreferences.Editor editor = sharedPreferences.edit();
                editor.putString("nexo_key", nexoKey);
                editor.apply();

                showProgressBar(true);
                loginButton.setEnabled(false);

                if (context instanceof MainActivity) {
                    new Auth((MainActivity)context).execute(new String[]{nexoKey});
                }
            }
        });
    }

    public static void showProgressBar(boolean show) {
        if (appContext == null) return;

        if (show) {
            if (progressDialog == null) {
                progressDialog = new ProgressDialog(appContext);
                progressDialog.setMessage("Authenticating NEXO Key...");
                progressDialog.setCancelable(false);
                progressDialog.setIndeterminate(true);
            }
            progressDialog.show();
        } else {
            if (progressDialog != null && progressDialog.isShowing()) {
                progressDialog.dismiss();
                progressDialog = null;
            }
        }

        if (loginButton != null) {
            loginButton.setEnabled(!show);
        }
    }

    public static void ShowInfoBox(Context context, String message, String level) {
        String toastMessage = message;
        if (level.equals("Error")) {
            toastMessage = "❌ " + message;
        } else if (level.equals("Success")) {
            toastMessage = "✅ " + message;
        } else if (level.equals("Warning")) {
            toastMessage = "⚠️ " + message;
        }
        Prefs.CustomToast(toastMessage, context);
    }

    public static void ShowInfoBox2(Context context, String message, String level) {
        String toastMessage = message;
        if (level.equals("Error")) {
            toastMessage = "❌ " + message;
        } else if (level.equals("Success")) {
            toastMessage = "✅ " + message;
        } else if (level.equals("Warning")) {
            toastMessage = "⚠️ " + message;
        }
        Prefs.CustomToast(toastMessage, context);
    }

    public void init(Context mContext, String titulo, String nome, String dialogoCima, String dialogoBaixo, String vendedores, String namemod) {}
}

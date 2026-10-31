package com.google.android.gemini;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.provider.Settings;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.topjohnwu.superuser.Shell;

public class MainActivity extends Activity {

    private static final int REQUEST_CODE_OVERLAY_PERMISSION = 2002;
    private TextView statusTextView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setGravity(Gravity.CENTER);
        root.setBackgroundColor(Color.parseColor("#0A0D14"));
        root.setPadding(40, 60, 40, 60);

        TextView title = new TextView(this);
        title.setText("GEMINI ENGINE");
        title.setTextSize(26);
        title.setTextColor(Color.parseColor("#00E5FF"));
        title.setTypeface(Typeface.create("sans-serif-black", Typeface.BOLD));
        title.setGravity(Gravity.CENTER);
        root.addView(title);

        TextView subtitle = new TextView(this);
        subtitle.setText("Official Android WindowManager System");
        subtitle.setTextSize(13);
        subtitle.setTextColor(Color.parseColor("#8E96A4"));
        subtitle.setGravity(Gravity.CENTER);
        subtitle.setPadding(0, 10, 0, 40);
        root.addView(subtitle);

        LinearLayout card = new LinearLayout(this);
        card.setOrientation(LinearLayout.VERTICAL);
        card.setPadding(30, 30, 30, 30);
        GradientDrawable cardBg = new GradientDrawable();
        cardBg.setColor(Color.parseColor("#141824"));
        cardBg.setCornerRadius(20);
        cardBg.setStroke(2, Color.parseColor("#2500E5FF"));
        card.setBackground(cardBg);

        statusTextView = new TextView(this);
        statusTextView.setText("System Ready. Tap Launch to start.");
        statusTextView.setTextSize(14);
        statusTextView.setTextColor(Color.WHITE);
        statusTextView.setGravity(Gravity.CENTER);
        card.addView(statusTextView);

        root.addView(card);

        View spacer = new View(this);
        spacer.setLayoutParams(new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 50));
        root.addView(spacer);

        Button startButton = new Button(this);
        startButton.setText("START ENGINE");
        startButton.setTextSize(16);
        startButton.setTextColor(Color.parseColor("#0A0D14"));
        startButton.setTypeface(Typeface.create("sans-serif-black", Typeface.BOLD));
        GradientDrawable btnBg = new GradientDrawable(
                GradientDrawable.Orientation.LEFT_RIGHT,
                new int[]{Color.parseColor("#00E5FF"), Color.parseColor("#0070F3")}
        );
        btnBg.setCornerRadius(16);
        startButton.setBackground(btnBg);
        startButton.setPadding(40, 24, 40, 24);

        startButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                launchEngine();
            }
        });
        root.addView(startButton);

        setContentView(root);
    }

    private void launchEngine() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && !Settings.canDrawOverlays(this)) {
            Intent intent = new Intent(
                    Settings.ACTION_MANAGE_OVERLAY_PERMISSION,
                    Uri.parse("package:" + getPackageName())
            );
            startActivityForResult(intent, REQUEST_CODE_OVERLAY_PERMISSION);
            Toast.makeText(this, "Please grant Floating Window permission", Toast.LENGTH_LONG).show();
            return;
        }

        statusTextView.setText("Checking Root Access...");
        new Thread(new Runnable() {
            @Override
            public void run() {
                boolean isRoot = Shell.getShell().isRoot();
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        if (isRoot) {
                            statusTextView.setText("Root Granted. Launching Overlay...");
                            new Initialize(MainActivity.this);
                            moveTaskToBack(true);
                        } else {
                            statusTextView.setText("Root access required! Running standard mode...");
                            new Initialize(MainActivity.this);
                            moveTaskToBack(true);
                        }
                    }
                });
            }
        }).start();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == REQUEST_CODE_OVERLAY_PERMISSION) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && Settings.canDrawOverlays(this)) {
                launchEngine();
            } else {
                Toast.makeText(this, "Overlay permission is required for ESP!", Toast.LENGTH_SHORT).show();
            }
        }
    }
}

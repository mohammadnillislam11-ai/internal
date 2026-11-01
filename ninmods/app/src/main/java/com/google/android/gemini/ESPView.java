package com.google.android.gemini;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.os.Handler;
import android.os.Looper;
import android.view.View;

/**
 * Official Full-Screen Passthrough ESP Overlay View.
 * Configured with FLAG_NOT_TOUCHABLE so it NEVER interferes with game touch input.
 * Draws center Aim FOV Circle, ESP tracer lines, player 2D boxes, distance, and health.
 */
public class ESPView extends View {

    private final Paint strokePaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint fillPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint textPaint = new Paint(Paint.ANTI_ALIAS_FLAG);

    private final float[] espBuffer = new float[1024];

    private final Handler handler = new Handler(Looper.getMainLooper());
    private boolean isRunning = true;

    private final Runnable redrawTask = new Runnable() {
        @Override
        public void run() {
            if (isRunning) {
                invalidate();
                handler.postDelayed(this, 16); // ~60 FPS
            }
        }
    };

    private static int getColorForIdx(int idx) {
        switch (idx) {
            case 0: return Color.rgb(26, 166, 255);  // Blue (নীল)
            case 1: return Color.rgb(255, 56, 64);   // Red (লাল)
            case 2: return Color.rgb(255, 89, 204);  // Pink (পিং)
            default: return Color.rgb(26, 166, 255);
        }
    }

    public ESPView(Context context) {
        super(context);
        setBackgroundColor(Color.TRANSPARENT);

        strokePaint.setStyle(Paint.Style.STROKE);
        strokePaint.setStrokeWidth(2.2f);

        fillPaint.setStyle(Paint.Style.FILL);

        textPaint.setTextSize(26.0f);
        textPaint.setColor(Color.WHITE);
        textPaint.setFakeBoldText(true);

        handler.post(redrawTask);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        if (canvas == null) return;

        int width = getWidth();
        int height = getHeight();
        if (width <= 0 || height <= 0) return;

        int count = NativeBridge.fetchESPData(espBuffer);
        if (count < 5) return;

        boolean drawFov = espBuffer[0] > 0.5f;
        float fovRadius = espBuffer[1];
        int boxColorIdx = (int) espBuffer[2];
        int visibleColorIdx = (int) espBuffer[3];
        boolean tracerEnabled = espBuffer[4] > 0.5f;

        int boxColor = getColorForIdx(boxColorIdx);
        int visibleColor = getColorForIdx(visibleColorIdx);

        // 1. Draw Aim FOV Circle & Crosshair Center Dot
        if (drawFov && fovRadius > 5.0f) {
            float cx = width * 0.5f;
            float cy = height * 0.5f;

            strokePaint.setColor(Color.argb(200, 174, 89, 250)); // Accent Purple
            strokePaint.setStrokeWidth(2.2f);
            canvas.drawCircle(cx, cy, fovRadius, strokePaint);

            fillPaint.setColor(Color.argb(230, 174, 89, 250));
            canvas.drawCircle(cx, cy, 3.5f, fillPaint);
        }

        // 2. Draw ESP Entities (each entity has 8 floats starting at offset 5)
        int offset = 5;
        float topAnchorX = width * 0.5f;
        float topAnchorY = 0.0f;

        while (offset + 8 <= count) {
            float headX = espBuffer[offset];
            float headY = espBuffer[offset + 1];
            float toeX  = espBuffer[offset + 2];
            float toeY  = espBuffer[offset + 3];
            float dist  = espBuffer[offset + 4];
            float hp    = espBuffer[offset + 5];
            boolean isVisible = espBuffer[offset + 6] > 0.5f;
            boolean isTracer = espBuffer[offset + 7] > 0.5f;
            offset += 8;

            int drawCol = isVisible ? visibleColor : boxColor;
            strokePaint.setColor(drawCol);
            strokePaint.setStrokeWidth(2.0f);

            // Tracer line from top center to enemy head
            if (tracerEnabled && isTracer && headX > 0 && headY > 0) {
                canvas.drawLine(topAnchorX, topAnchorY, headX, headY, strokePaint);
            }

            // 2D Bounding Box around player
            if (headX > 0 && toeY > headY) {
                float boxH = toeY - headY;
                float boxW = boxH * 0.55f;
                float left = headX - boxW * 0.5f;
                float top = headY;
                float right = headX + boxW * 0.5f;
                float bottom = toeY;

                canvas.drawRect(left, top, right, bottom, strokePaint);

                // Distance text
                String distText = (int) dist + "m";
                canvas.drawText(distText, left, top - 8.0f, textPaint);

                // Health bar
                if (hp > 0.0f) {
                    float hpPercent = Math.min(1.0f, Math.max(0.0f, hp / 100.0f));
                    float barLeft = left - 7.0f;
                    float barRight = left - 3.0f;
                    fillPaint.setColor(Color.argb(180, 40, 40, 40));
                    canvas.drawRect(barLeft, top, barRight, bottom, fillPaint);

                    fillPaint.setColor(hpPercent > 0.5f ? Color.GREEN : (hpPercent > 0.2f ? Color.YELLOW : Color.RED));
                    float barTop = bottom - (boxH * hpPercent);
                    canvas.drawRect(barLeft, barTop, barRight, bottom, fillPaint);
                }
            }
        }
    }

    public void stop() {
        isRunning = false;
        handler.removeCallbacks(redrawTask);
    }
}

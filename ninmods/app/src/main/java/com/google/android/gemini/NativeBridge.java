package com.google.android.gemini;

import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.view.Surface;
import android.widget.Toast;

public class NativeBridge {

    static {
        try {
            System.loadLibrary("ninmods");
        } catch (UnsatisfiedLinkError e) {
            e.printStackTrace();
        }
    }

    private static Context sContext;
    private static final Handler mainHandler = new Handler(Looper.getMainLooper());

    public static void setContext(Context context) {
        if (context != null) {
            sContext = context.getApplicationContext();
        }
    }

    public static void copyToClipboard(final String text) {
        if (text == null) return;
        mainHandler.post(new Runnable() {
            @Override
            public void run() {
                if (sContext == null) return;
                try {
                    ClipboardManager clipboard = (ClipboardManager)
                            sContext.getSystemService(Context.CLIPBOARD_SERVICE);
                    if (clipboard != null) {
                        ClipData clip = ClipData.newPlainText("Engine Logs", text);
                        clipboard.setPrimaryClip(clip);
                        Toast.makeText(sContext, "Logs copied to clipboard!", Toast.LENGTH_SHORT).show();
                    }
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        });
    }

    public static native void initSurface(Surface surface, int width, int height);
    public static native void surfaceChanged(Surface surface, int width, int height);
    public static native void surfaceDestroyed();
    public static native void onTouch(int action, float x, float y);
    public static native boolean isTouchInside(float x, float y);
    public static native boolean isMenuOpen();
    public static native void setMenuOpen(boolean open);
    public static native void stopEngine();
    public static native int fetchESPData(float[] buffer);
    public static native String popClipboardRequest();
}


package com.google.android.gemini;

import android.view.Surface;

public class NativeBridge {

    static {
        try {
            System.loadLibrary("ninmods");
        } catch (UnsatisfiedLinkError e) {
            e.printStackTrace();
        }
    }

    public static native void initSurface(Surface surface, int width, int height);
    public static native void surfaceChanged(Surface surface, int width, int height);
    public static native void surfaceDestroyed();
    public static native void onTouch(int action, float x, float y);
    public static native boolean isTouchInside(float x, float y);
    public static native boolean isMenuOpen();
    public static native void stopEngine();
}

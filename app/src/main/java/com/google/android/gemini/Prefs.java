package com.google.android.gemini;

import android.content.Context;
import android.content.SharedPreferences;

public class Prefs {
    private static Prefs prefsInstance;
    private SharedPreferences sharedPreferences;

    public static int dp(int dp, final Context context) {
        final float scale = context.getResources().getDisplayMetrics().density;
        return (int) (dp * scale + 0.5f);
    }

    private Prefs(Context context) {
        sharedPreferences = context.getApplicationContext().getSharedPreferences(
                context.getPackageName() + "_preferences",
                Context.MODE_PRIVATE
        );
    }

    public static Prefs with(Context context) {
        if (prefsInstance == null) {
            prefsInstance = new Prefs(context);
        }
        return prefsInstance;
    }

    public void writeBoolean(String key, boolean value) {
        sharedPreferences.edit().putBoolean(key, value).apply();
    }

    public boolean readBoolean(String key, boolean defaultValue) {
        return sharedPreferences.getBoolean(key, defaultValue);
    }

    public void writeInt(String key, int value) {
        sharedPreferences.edit().putInt(key, value).apply();
    }

    public int readInt(String key, int defaultValue) {
        return sharedPreferences.getInt(key, defaultValue);
    }

    public void writeString(String key, String value) {
        sharedPreferences.edit().putString(key, value).apply();
    }

    public String readString(String key, String defaultValue) {
        return sharedPreferences.getString(key, defaultValue);
    }
}

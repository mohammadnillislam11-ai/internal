package com.reaper.xxx;

import android.app.Activity;
import android.os.AsyncTask;
import android.content.Context;
import android.provider.Settings;
import android.util.Log;
import java.lang.ref.WeakReference;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;
import org.json.JSONArray;
import org.json.JSONObject;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.TimeZone;
import java.io.InputStream;

public class Auth extends AsyncTask<String, Void, String> {
    private static final String TAG = "NEXO_AUTH";
    private final WeakReference<Activity> activityRef;

    public native String nativeLogin(String key, String hwid);

    public Auth(Activity activity) {
        this.activityRef = new WeakReference<>(activity);
    }


    public static boolean loadfonts(Context c) {
        return false; // Never expired
    }

    @Override
    protected void onPreExecute() {
        LoginActivity.showProgressBar(true);
    }

    @Override
    protected String doInBackground(String... params) {
        return "NEXO_LOGIN_SUCCESS"; // Always valid
    }
    }

    // 🔥 FIXED: Robust parser - handles both Array and individual objects
    private boolean validateKeyWithServer(String key) {
        try {
            String urlString = "https://raw.githubusercontent.com/lastbgmi346-lgtm/Alljsonfile/refs/heads/main/keys.json";
            Log.d(TAG, "Fetching keys from: " + urlString);
            
            URL url = new URL(urlString);
            HttpURLConnection conn = (HttpURLConnection) url.openConnection();
            conn.setRequestMethod("GET");
            conn.setConnectTimeout(10000);
            conn.setReadTimeout(10000);
            conn.setRequestProperty("Cache-Control", "no-cache");

            int responseCode = conn.getResponseCode();
            Log.d(TAG, "Response Code: " + responseCode);
            
            if (responseCode == HttpURLConnection.HTTP_OK) {
                BufferedReader in = new BufferedReader(
                    new InputStreamReader(conn.getInputStream()));
                StringBuilder response = new StringBuilder();
                String line;
                while ((line = in.readLine()) != null) {
                    response.append(line);
                }
                in.close();

                String jsonResponse = response.toString().trim();
                Log.d(TAG, "Raw JSON: " + jsonResponse);
                
                // 🔥 Parse JSON Array
                JSONArray keysArray = null;
                
                try {
                    // Try to parse as JSONArray
                    keysArray = new JSONArray(jsonResponse);
                } catch (Exception e) {
                    Log.d(TAG, "Not a JSONArray, trying individual objects...");
                    // 🔥 If not array, try to parse individual objects
                    try {
                        // Split by "}\n{" pattern
                        String[] objects = jsonResponse.split("\\}\\s*\\{");
                        keysArray = new JSONArray();
                        
                        for (int i = 0; i < objects.length; i++) {
                            String objStr = objects[i];
                            // Add back the braces
                            if (i == 0) objStr = objStr + "}";
                            else if (i == objects.length - 1) objStr = "{" + objStr;
                            else objStr = "{" + objStr + "}";
                            
                            // Fix any malformed JSON
                            objStr = objStr.replaceAll("\\}\\s*\\{", "},{");
                            
                            try {
                                JSONObject obj = new JSONObject(objStr);
                                keysArray.put(obj);
                            } catch (Exception ex) {
                                Log.e(TAG, "Failed to parse object: " + objStr);
                            }
                        }
                    } catch (Exception ex) {
                        Log.e(TAG, "Failed to parse: " + e.getMessage());
                        return false;
                    }
                }
                
                if (keysArray == null || keysArray.length() == 0) {
                    Log.d(TAG, "No keys found in response");
                    return false;
                }
                
                Log.d(TAG, "Total keys found: " + keysArray.length());
                
                // 🔥 Search for key (case-insensitive)
                String upperKey = key.toUpperCase();
                for (int i = 0; i < keysArray.length(); i++) {
                    JSONObject obj = keysArray.getJSONObject(i);
                    String storedKey = obj.optString("key", "").toUpperCase();
                    
                    Log.d(TAG, "Checking key: " + storedKey);
                    
                    if (storedKey.equals(upperKey)) {
                        Log.d(TAG, "✅ Key found: " + key);
                        
                        // 🔥 Check status
                        String status = obj.optString("status", "active");
                        if (status.equals("used")) {
                            Log.d(TAG, "❌ Key already used");
                            return false;
                        }
                        
                        // 🔥 Check expiry
                        String expires = obj.optString("expires", "");
                        if (!expires.isEmpty()) {
                            try {
                                SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSS'Z'");
                                sdf.setTimeZone(TimeZone.getTimeZone("UTC"));
                                Date expDate = sdf.parse(expires);
                                long expTime = expDate.getTime();
                                long now = System.currentTimeMillis();
                                
                                Log.d(TAG, "Expires: " + expDate);
                                Log.d(TAG, "Now: " + new Date(now));
                                Log.d(TAG, "Time left: " + (expTime - now) + " ms");
                                
                                if (now > expTime) {
                                    Log.d(TAG, "❌ Key expired");
                                    return false;
                                }
                            } catch (Exception e) {
                                Log.e(TAG, "Date parse error: " + e.getMessage());
                                // If can't parse, assume valid
                            }
                        }
                        
                        Log.d(TAG, "✅ Key valid!");
                        return true;
                    }
                }
                
                Log.d(TAG, "❌ Key not found: " + key);
                return false;
            }
            
            Log.d(TAG, "Failed to fetch keys.json");
            return false;
            
        } catch (Exception e) {
            Log.e(TAG, "Validation error: " + e.getMessage());
            return false;
        }
    }

    @Override
    protected void onPostExecute(String result) {
        Activity activity = activityRef.get();
        LoginActivity.showProgressBar(false);

        if (activity == null || result == null) return;

        Log.d(TAG, "Final Result: " + result);

        // 🔥 CHECK FOR APP EXPIRED
        if (result.equals("APP_EXPIRED")) {
            LoginActivity.ShowInfoBox(activity, "❌ App has expired! Please contact support.", "Error");
            return;
        }

        if (result.equals("NEXO_LOGIN_SUCCESS")) {
            LoginActivity.ShowInfoBox(activity, "Welcome! NEXO Verified ✅", "Success");
            new Initialize(activity);
        } else if (result.equals("INVALID_KEY_SERVER")) {
            LoginActivity.ShowInfoBox(activity, "❌ Invalid or expired NEXO Key!", "Error");
        } else {
            LoginActivity.ShowInfoBox(activity, "❌ " + result, "Error");
        }
    }
}
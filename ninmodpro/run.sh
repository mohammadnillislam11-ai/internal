#!/bin/bash
# Push and execute ninmods client.sh on rooted Android device via ADB
TARGET_PATH="/data/local/tmp/client.sh"

echo "[*] Pushing client.sh to device..."
adb push libs/arm64-v8a/client.sh "$TARGET_PATH"

echo "[*] Setting executable permissions..."
adb shell "su -c 'chmod 777 $TARGET_PATH'"

echo "[*] Executing ninmods..."
adb shell "su -c '$TARGET_PATH'"

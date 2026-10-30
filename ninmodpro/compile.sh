#!/bin/bash
set -e

echo "[+] Building ninmods (client.sh)..."
ndk-build -B V=1

echo "[OK] Build complete! Outputs: libs/arm64-v8a/client.sh and libs/armeabi-v7a/client.sh"

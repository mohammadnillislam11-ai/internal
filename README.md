# NINMODS Client

Android Standalone Native Overlay Client with Multi-ABI (arm64-v8a and armeabi-v7a) support.

## Architecture
- Direct SurfaceFlinger Native Overlay (No floating window permission needed)
- Purple Glassmorphic UI (Translucent Acrylic Theme)
- Multi-ABI Compilation: 64-bit (arm64-v8a) and 32-bit (armeabi-v7a)
- Dual Framework: Dobby Hook Framework and ByteDance ShadowHook
- Live Firebase Licensing Engine with Watchdog Heartbeat
- Standalone Root Executables (client.sh-arm64 and client.sh-arm32)

## Build
Automated build with Android NDK via GitHub Actions.
Download the compiled client.sh-arm64 and client.sh-arm32 artifacts from the GitHub Actions tab.

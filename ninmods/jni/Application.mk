# Multi-ABI Support: Both 64-bit (arm64-v8a) and 32-bit (armeabi-v7a)
APP_ABI := arm64-v8a armeabi-v7a
APP_PLATFORM := android-21
APP_STL := c++_static
APP_CPPFLAGS := -std=c++17 -fexceptions -frtti

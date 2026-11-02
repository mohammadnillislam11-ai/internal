LOCAL_PATH := $(call my-dir)

# ============================================================
# Prebuilt: libcurl (Multi-ABI arm64-v8a & armeabi-v7a)
# ============================================================
include $(CLEAR_VARS)
LOCAL_MODULE := curl
LOCAL_SRC_FILES := libraries/network/curl/lib/$(TARGET_ARCH_ABI)/libcurl.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/libraries/network/curl/include
include $(PREBUILT_STATIC_LIBRARY)

# ============================================================
# Prebuilt: OpenSSL libssl (Multi-ABI)
# ============================================================
include $(CLEAR_VARS)
LOCAL_MODULE := ssl
LOCAL_SRC_FILES := libraries/network/openssl/lib/$(TARGET_ARCH_ABI)/libssl.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/libraries/network/openssl/include
include $(PREBUILT_STATIC_LIBRARY)

# ============================================================
# Prebuilt: OpenSSL libcrypto (Multi-ABI)
# ============================================================
include $(CLEAR_VARS)
LOCAL_MODULE := crypto
LOCAL_SRC_FILES := libraries/network/openssl/lib/$(TARGET_ARCH_ABI)/libcrypto.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/libraries/network/openssl/include
include $(PREBUILT_STATIC_LIBRARY)

# ============================================================
# MAIN EXECUTABLE - gemini-cli.sh (100% Pure Ring 0 Driver)
# ============================================================
include $(CLEAR_VARS)
LOCAL_MODULE := gemini-cli.sh

# 1. Official ImGui Core & Backends
FILE_LIST := $(wildcard $(LOCAL_PATH)/libraries/imgui/*.cpp)

# 2. Input Library
FILE_LIST += $(wildcard $(LOCAL_PATH)/libraries/input/*.cpp)

# 3. Secure Online License Network Client
FILE_LIST += $(wildcard $(LOCAL_PATH)/libraries/network/*.cpp)

# 4. Storage & Clipboard Native Libraries
FILE_LIST += $(wildcard $(LOCAL_PATH)/libraries/storage/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/libraries/clipboard/*.cpp)

# 5. Animation (Official AHEasing) & Security Obfuscation
FILE_LIST += $(wildcard $(LOCAL_PATH)/libraries/animation/*.c)
FILE_LIST += $(wildcard $(LOCAL_PATH)/libraries/security/oxorany/*.cpp)

# 6. Menu UI & License View
FILE_LIST += $(wildcard $(LOCAL_PATH)/menu/*.cpp)

# 7. Engine Modules (Game Data, ESP, Aimbot, Ring 0 Memory Driver)
FILE_LIST += $(wildcard $(LOCAL_PATH)/engine/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/engine/*/*.cpp)

# 8. Main Application Entry Loop
FILE_LIST += $(LOCAL_PATH)/main.cpp

LOCAL_SRC_FILES := $(FILE_LIST:$(LOCAL_PATH)/%=%)

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/libraries/imgui \
    $(LOCAL_PATH)/libraries/fonts \
    $(LOCAL_PATH)/libraries/surface \
    $(LOCAL_PATH)/libraries/input \
    $(LOCAL_PATH)/libraries/image \
    $(LOCAL_PATH)/libraries/network \
    $(LOCAL_PATH)/libraries/network/curl/include \
    $(LOCAL_PATH)/libraries/network/openssl/include \
    $(LOCAL_PATH)/libraries/network/json \
    $(LOCAL_PATH)/libraries/animation \
    $(LOCAL_PATH)/libraries/display \
    $(LOCAL_PATH)/libraries/storage \
    $(LOCAL_PATH)/libraries/clipboard \
    $(LOCAL_PATH)/libraries/math \
    $(LOCAL_PATH)/libraries/math/glm \
    $(LOCAL_PATH)/libraries/security/oxorany \
    $(LOCAL_PATH)/menu \
    $(LOCAL_PATH)/engine \
    $(LOCAL_PATH)/engine/core \
    $(LOCAL_PATH)/engine/data \
    $(LOCAL_PATH)/engine/esp \
    $(LOCAL_PATH)/engine/aim \
    $(LOCAL_PATH)/engine/memory \
    $(LOCAL_PATH)/engine/offsets \
    $(LOCAL_PATH)/engine/security

LOCAL_STATIC_LIBRARIES := curl ssl crypto

LOCAL_LDLIBS := -llog -landroid -lEGL -lGLESv3 -lz -ldl

LOCAL_LDFLAGS := \
    -Wl,--gc-sections \
    -Wl,--strip-all \
    -Wl,--build-id=none \
    -pie \
    -Wl,-z,relro \
    -Wl,-z,now \
    -Wl,-z,noexecstack

LOCAL_CPPFLAGS := -std=c++17 -fexceptions -frtti -Wno-unused-parameter -Wno-register
LOCAL_CFLAGS := -O3 -fvisibility=hidden -fdata-sections -ffunction-sections

include $(BUILD_EXECUTABLE)

# ============================================================
# SHARED LIBRARY - libgemini.so for Android App & WindowManager
# ============================================================
include $(CLEAR_VARS)
LOCAL_MODULE := gemini

SHARED_FILE_LIST := $(wildcard $(LOCAL_PATH)/libraries/imgui/*.cpp)
SHARED_FILE_LIST += $(wildcard $(LOCAL_PATH)/libraries/input/*.cpp)
SHARED_FILE_LIST += $(wildcard $(LOCAL_PATH)/libraries/network/*.cpp)
SHARED_FILE_LIST += $(wildcard $(LOCAL_PATH)/libraries/storage/*.cpp)
SHARED_FILE_LIST += $(wildcard $(LOCAL_PATH)/libraries/clipboard/*.cpp)
SHARED_FILE_LIST += $(wildcard $(LOCAL_PATH)/libraries/animation/*.c)
SHARED_FILE_LIST += $(wildcard $(LOCAL_PATH)/libraries/security/oxorany/*.cpp)
SHARED_FILE_LIST += $(wildcard $(LOCAL_PATH)/menu/*.cpp)
SHARED_FILE_LIST += $(wildcard $(LOCAL_PATH)/engine/*.cpp)
SHARED_FILE_LIST += $(wildcard $(LOCAL_PATH)/engine/*/*.cpp)
SHARED_FILE_LIST += $(LOCAL_PATH)/native_surface_engine.cpp

LOCAL_SRC_FILES := $(SHARED_FILE_LIST:$(LOCAL_PATH)/%=%)

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/libraries/imgui \
    $(LOCAL_PATH)/libraries/fonts \
    $(LOCAL_PATH)/libraries/surface \
    $(LOCAL_PATH)/libraries/input \
    $(LOCAL_PATH)/libraries/image \
    $(LOCAL_PATH)/libraries/network \
    $(LOCAL_PATH)/libraries/network/curl/include \
    $(LOCAL_PATH)/libraries/network/openssl/include \
    $(LOCAL_PATH)/libraries/network/json \
    $(LOCAL_PATH)/libraries/animation \
    $(LOCAL_PATH)/libraries/display \
    $(LOCAL_PATH)/libraries/storage \
    $(LOCAL_PATH)/libraries/clipboard \
    $(LOCAL_PATH)/libraries/math \
    $(LOCAL_PATH)/libraries/math/glm \
    $(LOCAL_PATH)/libraries/security/oxorany \
    $(LOCAL_PATH)/menu \
    $(LOCAL_PATH)/engine \
    $(LOCAL_PATH)/engine/core \
    $(LOCAL_PATH)/engine/data \
    $(LOCAL_PATH)/engine/esp \
    $(LOCAL_PATH)/engine/aim \
    $(LOCAL_PATH)/engine/memory \
    $(LOCAL_PATH)/engine/offsets \
    $(LOCAL_PATH)/engine/security

LOCAL_STATIC_LIBRARIES := curl ssl crypto

LOCAL_LDLIBS := -llog -landroid -lEGL -lGLESv3 -lz -ldl
LOCAL_CPPFLAGS := -std=c++17 -fexceptions -frtti -Wno-unused-parameter -Wno-register
LOCAL_CFLAGS := -O3 -fvisibility=hidden -fdata-sections -ffunction-sections

include $(BUILD_SHARED_LIBRARY)

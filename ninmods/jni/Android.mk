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
# Prebuilt: Dobby Hook Framework (Multi-ABI)
# ============================================================
include $(CLEAR_VARS)
LOCAL_MODULE := dobby
LOCAL_SRC_FILES := libraries/hook/dobby/lib/$(TARGET_ARCH_ABI)/libdobby.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/libraries/hook/dobby/include
include $(PREBUILT_STATIC_LIBRARY)

# ============================================================
# Prebuilt: ByteDance ShadowHook (arm64-v8a)
# ============================================================
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
include $(CLEAR_VARS)
LOCAL_MODULE := shadowhook
LOCAL_SRC_FILES := libraries/hook/shadowhook/lib/$(TARGET_ARCH_ABI)/libshadowhook.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/libraries/hook/shadowhook/include
include $(PREBUILT_STATIC_LIBRARY)
endif

# ============================================================
# MAIN EXECUTABLE - client.sh
# ============================================================
include $(CLEAR_VARS)
LOCAL_MODULE := client.sh

# 1. Official ImGui Core & Backends
FILE_LIST := $(wildcard $(LOCAL_PATH)/libraries/imgui/*.cpp)

# 2. Input Library
FILE_LIST += $(wildcard $(LOCAL_PATH)/libraries/input/*.cpp)

# 3. Secure Online License Network Client
FILE_LIST += $(wildcard $(LOCAL_PATH)/libraries/network/*.cpp)

# 4. Storage & Clipboard Native Libraries
FILE_LIST += $(wildcard $(LOCAL_PATH)/libraries/storage/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/libraries/clipboard/*.cpp)

# 5. Animation (Official AHEasing), Memory (KittyMemory), xDL & Security Obfuscation
FILE_LIST += $(wildcard $(LOCAL_PATH)/libraries/animation/*.c)
FILE_LIST += $(LOCAL_PATH)/libraries/kittymemory/KittyMemory.cpp
FILE_LIST += $(LOCAL_PATH)/libraries/kittymemory/KittyUtils.cpp
FILE_LIST += $(wildcard $(LOCAL_PATH)/libraries/xdl/src/main/cpp/*.c)
FILE_LIST += $(wildcard $(LOCAL_PATH)/libraries/security/oxorany/*.cpp)

# 6. NativeShield Security Engine
FILE_LIST += $(wildcard $(LOCAL_PATH)/libraries/security/shield/*.cpp)

# 7. Menu UI & License View
FILE_LIST += $(wildcard $(LOCAL_PATH)/menu/*.cpp)

# 8. Engine Modules (Game Data, ESP, Aimbot, Memory)
FILE_LIST += $(wildcard $(LOCAL_PATH)/engine/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/engine/*/*.cpp)

# 9. Main Application Entry Loop
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
    $(LOCAL_PATH)/libraries/kittymemory \
    $(LOCAL_PATH)/libraries/xdl/src/main/cpp/include \
    $(LOCAL_PATH)/libraries/hook/dobby/include \
    $(LOCAL_PATH)/libraries/hook/shadowhook/include \
    $(LOCAL_PATH)/libraries/security/oxorany \
    $(LOCAL_PATH)/libraries/security/shield \
    $(LOCAL_PATH)/libraries/security/shield/Modules \
    $(LOCAL_PATH)/libraries/security/shield/SecureAPI \
    $(LOCAL_PATH)/libraries/security/shield/Utils \
    $(LOCAL_PATH)/libraries/unity \
    $(LOCAL_PATH)/menu \
    $(LOCAL_PATH)/engine \
    $(LOCAL_PATH)/engine/core \
    $(LOCAL_PATH)/engine/data \
    $(LOCAL_PATH)/engine/esp \
    $(LOCAL_PATH)/engine/aim \
    $(LOCAL_PATH)/engine/memory \
    $(LOCAL_PATH)/engine/offsets \
    $(LOCAL_PATH)/engine/security

LOCAL_STATIC_LIBRARIES := curl ssl crypto dobby
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
LOCAL_STATIC_LIBRARIES += shadowhook
endif

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

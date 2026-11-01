LOCAL_PATH := $(call my-dir)

# --- Define CURL Prebuilt ---
include $(CLEAR_VARS)
LOCAL_MODULE := curl_static
LOCAL_SRC_FILES := src/static/curl/$(TARGET_ARCH_ABI)/libcurl.a
include $(PREBUILT_STATIC_LIBRARY)

# --- Module: palmeiras ---
include $(CLEAR_VARS)

LOCAL_MODULE    := joudadooh

LOCAL_SRC_FILES := \
    src/Client.cpp \
    src/Socket/client.cpp \
    src/frnetlib/Base64.cpp \
    src/frnetlib/Http.cpp \
    src/frnetlib/HttpRequest.cpp \
    src/frnetlib/HttpResponse.cpp \
    src/frnetlib/Sha1.cpp \
    src/frnetlib/Socket.cpp \
    src/frnetlib/SocketSelector.cpp \
    src/frnetlib/TcpListener.cpp \
    src/frnetlib/TcpSocket.cpp \
    src/frnetlib/URL.cpp \
    src/frnetlib/WebFrame.cpp

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/src \
    $(LOCAL_PATH)/src/Includes \
    $(LOCAL_PATH)/src/Includes/curl \
    $(LOCAL_PATH)/src/Socket \
    $(LOCAL_PATH)/src/Unity

# Added -frtti to fix the dynamic_cast error
LOCAL_CPPFLAGS  := -fvisibility=hidden -DNDEBUG -std=c++17 -fexceptions -frtti
LOCAL_CFLAGS    := -Wno-error=format-security -fpermissive -fvisibility=hidden -fexceptions -frtti -DNDEBUG

# Moved -latomic here to fix the linker warning
LOCAL_LDFLAGS   := -Wl,--exclude-libs,ALL -s -latomic
LOCAL_LDLIBS    := -llog -landroid -lz

LOCAL_STATIC_LIBRARIES := curl_static
LOCAL_ARM_MODE  := arm

include $(BUILD_SHARED_LIBRARY)

# --- Module: UtilityHelper ---
include $(CLEAR_VARS)
LOCAL_MODULE    := Fucker
LOCAL_SRC_FILES := src/Server.cpp src/Socket/server.cpp
LOCAL_C_INCLUDES := $(LOCAL_PATH)/src $(LOCAL_PATH)/src/Includes
LOCAL_CPPFLAGS  := -fvisibility=hidden -DNDEBUG -std=c++17 -fexceptions -frtti
LOCAL_CFLAGS    := -Wno-error=format-security -fpermissive -fvisibility=hidden -fexceptions -frtti -DNDEBUG
LOCAL_LDFLAGS   := -Wl,--exclude-libs,ALL -s
LOCAL_LDLIBS    := -llog -lz
LOCAL_ARM_MODE  := arm
include $(BUILD_EXECUTABLE)

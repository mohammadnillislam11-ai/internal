#include <pthread.h>
#include <jni.h>
#include <src/Socket/client.h>
#include "src/Includes/obfuscate.h"
#include <sys/stat.h>
#include <src/Widgets/ImportWidgets.h>
#include <src/Includes/http.h>
#include "src/Unity/Vector3.hpp"
#include "src/Unity/Vector2.hpp"
#include "src/Unity/Unity.h"
#include "src/Unity/ESP.h"
#include "Includes/Logger.h"
#include "Includes/Utils.h"
#include "src/Widgets/Methods.h"

#include "Client.h"
#include "BooleanClient.h"

#include <curl/curl.h>
#include "src/Includes/json.h" 
using json = nlohmann::json;



// ========== Global Variables ==========
float g_circleXOffset = 0.0f;
float g_circleYOffset = 0.0f;
bool g_showFOVCircle = false;
int g_linePositionType = 0;
float g_lineStartXOffset = 0.0f;
float g_lineStartYOffset = 0.0f;

// ========== ESP OFFSET VARIABLES ==========
float g_espXOffset = 0.0f;
float g_espYOffset = 0.0f;
float g_espWidthScale = 1.0f;
float g_espHeightScale = 1.0f;

// ========== LEFT/RIGHT SEPARATE OFFSETS ==========
int g_espLeftValue = 0;    // 0 to 500
int g_espRightValue = 0;   // 0 to 500
int g_espUpValue = 0;      // 0 to 500
int g_espDownValue = 0;    // 0 to 500

struct MemoryChunk {
    char *memory;
    size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryChunk *mem = (struct MemoryChunk *)userp;
    char *ptr = (char *)realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) return 0;
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    return realsize;
}

std::string XOR_decryption(std::string value, std::string key) {
    std::string text = "";
    int klen = key.length();
    for (int v = 0; v < value.length(); v++) {
        text += value[v] ^ key[v % klen];
    }
    return text;
}

std::string get_Key_From_Public(std::string publicKey) {
    if (publicKey.length() < 98) return "000000000";
    std::string keyDes = "";
    keyDes += publicKey[58 - 1];
    keyDes += publicKey[62 - 1];
    keyDes += publicKey[10 - 1];
    keyDes += publicKey[47 - 1];
    keyDes += publicKey[20 - 1];
    keyDes += publicKey[33 - 1];
    keyDes += publicKey[87 - 1];
    keyDes += publicKey[98 - 1];
    keyDes += publicKey[14 - 1];
    return keyDes;
}

#define game_package OBFUSCATE("com.dts.freefiremax")
#define LibBypass OBFUSCATE("libanogs.so")

struct ESPPosition {
    float espXOffset = 0.0f;
    float espYOffset = 0.0f;
    float espWidthOffset = 0.0f;
    float espHeightOffset = 0.0f;
};

struct ConnectionStatus {
    bool connectClient = false;
    std::string server_msg = "Waiting...";
} Connection;

ESPPosition espPos;

// ========== NEXO KEY LIST ==========
std::vector<std::string> validNexoKeys = {
    "NEXO-ABCD1234",
    "NEXO-REAPER001",
    "NEXO-PROJECTX",
    "NEXO-LEKA2026",
    "NEXO-ROOTKING",
    "NEXO-FREEFIRE",
    "NEXO-ESP2026",
    "NEXO-AIMBOT",
    "NEXO-VIP999",
    "NEXO-ADMIN"
};

bool ValidateNexoKeyDirect(const std::string& key) {
    if (key.length() < 10) return false;
    if (key.substr(0, 5) != "NEXO-") return false;
    return true;
}

// ========== FOV CIRCLE ==========
void DrawFOVCircle(ESP esp, float fov, Color color, float screenWidth, float screenHeight) {
    if (fov <= 0) return;

    float centerX = (screenWidth / 2.0f) + g_circleXOffset;
    float centerY = (screenHeight / 2.0f) + g_circleYOffset;

    float maxRadius = (screenWidth < screenHeight) ? screenWidth / 2.5f : screenHeight / 2.5f;
    float radius = (fov / 360.0f) * maxRadius;
    
    if (radius < 30) radius = 30;
    if (radius > maxRadius) radius = maxRadius;

    Vector2 centerPos(centerX, centerY);
    esp.DrawCircle(color, 3.0f, centerPos, radius);

    float crossSize = 25.0f;
    esp.DrawLine(color, 2.5f,
                 Vector3(centerX - crossSize, centerY, 0),
                 Vector3(centerX + crossSize, centerY, 0));
    esp.DrawLine(color, 2.5f,
                 Vector3(centerX, centerY - crossSize, 0),
                 Vector3(centerX, centerY + crossSize, 0));
}

// ========== DRAW ESP ==========
void DrawESP(ESP esp) {
    auto screenHeight = esp.getHeight();
    auto screenWidth = esp.getWidth();
    
    if (!isConnected()) {
        static uint64_t lastConnectAttempt = 0;
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        uint64_t now = (uint64_t)ts.tv_sec * 1000 + (uint64_t)(ts.tv_nsec / 1000000);
        if (now - lastConnectAttempt > 1000) {
            lastConnectAttempt = now;
            client = SocketClient();
            if (client.Create()) {
                if (client.Connect()) {
                    initServer();
                } else {
                    client.Close();
                }
            }
        }
        return;
    }

    if (Actived.activar && pPlayer.aimbot && g_showFOVCircle && pPlayer.AimFov > 0) {
        DrawFOVCircle(esp, pPlayer.AimFov, pEspPlayer.espColor, screenWidth, screenHeight);
    }

    auto response = getData(screenWidth, screenHeight);
    if (!response.Success || response.PlayerCount <= 0) return;

    for (int i = 0; i < response.PlayerCount; i++) {
        auto player = response.Players[i];
        if (player.Head == Vector3::Zero() || player.Toe == Vector3::Zero()) continue;
        if (!Actived.activar) continue;

        float distance = player.Distancia;
        if (distance < 0.1f || distance > 150.0f) continue;

        float headX = player.Head.X;
        float headY = player.Head.Y;
        float toeY = player.Toe.Y;
        
        if (headX <= 1.0f && headY <= 1.0f) {
            headX = headX * screenWidth;
            headY = headY * screenHeight;
            toeY = toeY * screenHeight;
        }
        
        // ========== APPLY OFFSETS ==========
        // Left = negative, Right = positive
        float totalXOffset = (float)(g_espRightValue - g_espLeftValue);
        float totalYOffset = (float)(g_espDownValue - g_espUpValue);
        
        headX += totalXOffset;
        headY += totalYOffset;
        toeY += totalYOffset;
        
        float boxHeight = std::abs(toeY - headY) * 1.25f * g_espHeightScale;
        float boxWidth = boxHeight * 0.45f * g_espWidthScale;
        
        float boxX = headX - (boxWidth / 2.0f);
        float boxY = headY - (boxHeight * 0.08f);
        
        if (boxX < 0) { boxWidth += boxX; boxX = 0; }
        if (boxY < 0) { boxHeight += boxY; boxY = 0; }
        if (boxX + boxWidth > screenWidth) boxWidth = screenWidth - boxX;
        if (boxY + boxHeight > screenHeight) boxHeight = screenHeight - boxY;
        
        if (boxWidth <= 0 || boxHeight <= 0) continue;

        Color currentBoxColor = player.IsCaido ? Color::Red() : pEspPlayer.espColor;
        float fSize = pEspPlayer.textSize;
        if (fSize < 12) fSize = 12;

        // ===== BOX =====
        if (pEspPlayer.espCaixa) {
            switch(pEspPlayer.boxType) {
                case 0:
                    esp.DrawBox(currentBoxColor, 2.0f, Rect(boxX, boxY, boxWidth, boxHeight));
                    break;
                case 1: {
                    Color fill = currentBoxColor;
                    fill.a = 70;
                    esp.DrawFilledRect(fill, Vector3(boxX, boxY, 0), boxWidth, boxHeight);
                    esp.DrawBox(currentBoxColor, 1.5f, Rect(boxX, boxY, boxWidth, boxHeight));
                    break;
                }
                case 2:
                    esp.DrawBox4Line((int)boxX, (int)boxY, (int)boxWidth, (int)boxHeight, currentBoxColor, 2.2f);
                    break;
                case 3: {
                    Color fill = currentBoxColor;
                    fill.a = 50;
                    esp.DrawFilledRect(fill, Vector3(boxX, boxY, 0), boxWidth, boxHeight);
                    esp.DrawBox(currentBoxColor, 1.5f, Rect(boxX, boxY, boxWidth, boxHeight));
                    break;
                }
            }
        }

        // ===== LINE =====
        if (pEspPlayer.espLinha) {
            float centerX = boxX + (boxWidth / 2.0f);
            float centerY = boxY;
            
            Vector3 start;
            switch(g_linePositionType) {
                case 0: start = Vector3(screenWidth/2, 0, 0); break;
                case 1: start = Vector3(screenWidth/2, screenHeight/2, 0); break;
                case 2: start = Vector3(screenWidth/2, screenHeight, 0); break;
                default: start = Vector3(screenWidth/2, screenHeight/2, 0);
            }
            
            start.X += g_lineStartXOffset;
            start.Y += g_lineStartYOffset;
            
            esp.DrawLine(currentBoxColor, 2.5f, start, Vector3(centerX, centerY, 0));
            esp.DrawCircle(currentBoxColor, 5.0f, Vector2(start.X, start.Y), 4.0f);
        }

        // ===== HEALTH =====
        if (pEspPlayer.espHealth) {
            float hBarX = boxX - 12;
            esp.DrawVerticalHealthBar(Vector2(hBarX, boxY), boxHeight, 100.0f, player.Health, 6.0f, false);
        }

        // ===== NAME =====
        if (pEspPlayer.espName && player.PlayerName[0] != '\0') {
            float nameX = boxX + (boxWidth / 2.0f);
            float nameY = boxY - fSize - 4;
            
            Color black = Color(0,0,0,255);
            esp.DrawText(black, player.PlayerName, Vector2(nameX-1, nameY-1), fSize);
            esp.DrawText(black, player.PlayerName, Vector2(nameX+1, nameY+1), fSize);
            esp.DrawText(black, player.PlayerName, Vector2(nameX-1, nameY+1), fSize);
            esp.DrawText(black, player.PlayerName, Vector2(nameX+1, nameY-1), fSize);
            esp.DrawText(currentBoxColor, player.PlayerName, Vector2(nameX, nameY), fSize);
        }

        // ===== DISTANCE =====
        if (pEspPlayer.espDistance && !player.IsCaido) {
            std::string dist = std::to_string((int)distance) + "M";
            float dX = boxX + (boxWidth / 2.0f);
            float dY = boxY + boxHeight + fSize + 4;
            
            Color black = Color(0,0,0,255);
            esp.DrawText(black, dist.c_str(), Vector2(dX-1, dY-1), fSize);
            esp.DrawText(black, dist.c_str(), Vector2(dX+1, dY+1), fSize);
            esp.DrawText(black, dist.c_str(), Vector2(dX-1, dY+1), fSize);
            esp.DrawText(black, dist.c_str(), Vector2(dX+1, dY-1), fSize);
            esp.DrawText(currentBoxColor, dist.c_str(), Vector2(dX, dY), fSize);
        }
    }
}

// ========== NEXO LOGIN ==========
extern "C"
JNIEXPORT jstring JNICALL
Java_com_google_android_gemini_Auth_nativeLogin(JNIEnv *env, jobject thiz, jstring uKey, jstring uHwid) {
    const char *key  = env->GetStringUTFChars(uKey,  0);
    const char *hwid = env->GetStringUTFChars(uHwid, 0);
    
    std::string nexoKey(key);
    std::string result;
    
    if (ValidateNexoKeyDirect(nexoKey)) {
        result = "NEXO_LOGIN_SUCCESS";
        Connection.server_msg = "Welcome! NEXO Verified";
    } else {
        result = "INVALID_NEXO_KEY";
        Connection.server_msg = "Invalid NEXO Key!";
    }

    env->ReleaseStringUTFChars(uKey,  key);
    env->ReleaseStringUTFChars(uHwid, hwid);

    return env->NewStringUTF(result.c_str());
}

// ========== FLOATER MENU ==========
extern "C"
JNIEXPORT void JNICALL
Java_com_google_android_gemini_Floater_Functions(JNIEnv *env, jclass clazz) {
    Widget widget = Widget(env);

    widget.Category(OBFUSCATE("Memory Menu"));
    widget.Switch(OBFUSCATE("Enable Functions"), 1);
    widget.Switch(OBFUSCATE("Aimbot"), 2);
    widget.SeekBar(OBFUSCATE("Aimbot Type"), 0, 1, OBFUSCATE("Head/Body"), 28);
    widget.SeekBar(OBFUSCATE("Aim Fov"), 0, 360, OBFUSCATE(""), 9);
    widget.Switch(OBFUSCATE("Aim Lock"), 6);
    
    widget.Category(OBFUSCATE("Speed Menu"));
    widget.Switch(OBFUSCATE("Speed Hack"), 25);
    
    widget.Category(OBFUSCATE("Esp Menu"));
    widget.Switch(OBFUSCATE("Draw Line"), 11);
    widget.Switch(OBFUSCATE("Draw Box"), 12);
    widget.Switch(OBFUSCATE("Draw Distance"), 13);
    widget.Switch(OBFUSCATE("Draw Name"), 21);
    widget.Switch(OBFUSCATE("Draw Health"), 19);
    
    widget.Category(OBFUSCATE("Esp Config"));
    widget.SeekBar(OBFUSCATE("ESP Color"), 0, 9, OBFUSCATE("Color"), 16);
    widget.SeekBar(OBFUSCATE("Box Type"), 0, 3, OBFUSCATE("BoxType"), 17);
    widget.SeekBar(OBFUSCATE("Line Type"), 0, 2, OBFUSCATE("LineType"), 18);
    widget.SeekBar(OBFUSCATE("Text Size"), 10, 15, OBFUSCATE(""), 20);
    
    // ========== ESP POSITION ADJUST ==========
    widget.Category(OBFUSCATE("ESP Position Adjust"));
    widget.SeekBar(OBFUSCATE("ESP Move Left"), 0, 500, OBFUSCATE("Move Left"), 34);
    widget.SeekBar(OBFUSCATE("ESP Move Right"), 0, 500, OBFUSCATE("Move Right"), 35);
}

// ========== CHANGES ID ==========
extern "C"
JNIEXPORT void JNICALL
Java_com_google_android_gemini_Floater_ChangesID(JNIEnv *env, jclass clazz, jint id, jint value) {
    switch (id) {
        case 1: Actived.activar = !Actived.activar; SendBool(3, Actived.activar); break;
        case 2: pPlayer.aimbot = !pPlayer.aimbot; SendBool(5, pPlayer.aimbot); break;
        case 3: pPlayer.fovawm = !pPlayer.fovawm; SendBool(22, pPlayer.fovawm); break;
        case 6: pPlayer.aimbotlock = !pPlayer.aimbotlock; SendBool(54, pPlayer.aimbotlock); break;
        case 8: pPlayer.silen4a = !pPlayer.silen4a; SendBool(57, pPlayer.silen4a); break;
        case 9: pPlayer.AimFov = value; SendFloat(58, pPlayer.AimFov); break;
        case 10: pPlayer.AimDistance = value; SendFloat(59, pPlayer.AimDistance); break;
        case 11: pEspPlayer.espLinha = !pEspPlayer.espLinha; break;
        case 12: pEspPlayer.espCaixa = !pEspPlayer.espCaixa; break;
        case 13: pEspPlayer.espDistance = !pEspPlayer.espDistance; break;
        case 14: pEspPlayer.espSkeleton = !pEspPlayer.espSkeleton; break;
        case 15: pEspPlayer.espIden360 = !pEspPlayer.espIden360; break;
        
        case 16:
            if (value == 0) pEspPlayer.espColor = Color(255, 255, 255, 255);
            else if (value == 1) pEspPlayer.espColor = Color(0, 255, 0, 255);
            else if (value == 2) pEspPlayer.espColor = Color(0, 0, 255, 255);
            else if (value == 3) pEspPlayer.espColor = Color(255, 0, 0, 255);
            else if (value == 4) pEspPlayer.espColor = Color(0, 0, 0, 255);
            else if (value == 5) pEspPlayer.espColor = Color(255, 255, 0, 255);
            else if (value == 6) pEspPlayer.espColor = Color(0, 255, 255, 255);
            else if (value == 7) pEspPlayer.espColor = Color(255, 0, 255, 255);
            else if (value == 8) pEspPlayer.espColor = Color(128, 128, 128, 255);
            else if (value == 9) pEspPlayer.espColor = Color(160, 32, 240, 255);
            break;
        case 17: pEspPlayer.boxType = static_cast<BoxType>(value); break;
        case 18: pEspPlayer.lineType = static_cast<LineType>(value); break;
        case 19: pEspPlayer.espHealth = !pEspPlayer.espHealth; break;
        case 20: pEspPlayer.textSize = (float)value; break;
        case 21: pEspPlayer.espName = !pEspPlayer.espName; break;
        case 25:
            pPlayer.SpeedHackV2 = !pPlayer.SpeedHackV2;
            SendBool(61, pPlayer.SpeedHackV2);
            break;
        case 28: 
            pPlayer.aimbotType = value;
            SendInt(60, pPlayer.aimbotType);
            break;
            
        // ========== ESP OFFSET CASES ==========
        case 32: 
            g_espWidthScale = (float)value / 100.0f; 
            break;
        case 33: 
            g_espHeightScale = (float)value / 100.0f; 
            break;
        case 34: 
            g_espLeftValue = value; 
            break;
        case 35: 
            g_espRightValue = value; 
            break;
        case 36: 
            g_espUpValue = value; 
            break;
        case 37: 
            g_espDownValue = value; 
            break;
    }
}

// ========== JNI EXPORTS ==========
extern "C" JNIEXPORT void JNICALL
Java_com_google_android_gemini_Floater_PxbftKZXivSr(JNIEnv *env, jclass type, jobject espView, jobject canvas, jint width, jint height) {
    ESP localEsp = ESP(env, espView, canvas);
    if (localEsp.isValid()) { DrawESP(localEsp); }
}

extern "C" JNIEXPORT void JNICALL
Java_com_google_android_gemini_Floater_ftKZXivSr(JNIEnv *env, jclass clazz, jobject ctx) {
    if (!Connection.connectClient) {
        startClient();
        Connection.connectClient = true;
        Toast(env, ctx, "ACTIVATING...", 1, 1);
        Toast(env, ctx, "ACTIVATED", 1, 1);
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_google_android_gemini_Floater_ftKwCvSr(JNIEnv *env, jclass clazz) {
    stopClient();
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_google_android_gemini_LoginActivity_apkHashUrl(JNIEnv *env, jobject thiz) {
    return env->NewStringUTF("https://rewardff.com.br/apkhash.php");
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_google_android_gemini_LoginActivity_updateUrl(JNIEnv *env, jobject thiz) {
    return env->NewStringUTF("https://rewardff.com.br/update.php");
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_google_android_gemini_Auth_URL(JNIEnv *env, jobject thiz) {
    return env->NewStringUTF("https://abhihero.in/connect");
}
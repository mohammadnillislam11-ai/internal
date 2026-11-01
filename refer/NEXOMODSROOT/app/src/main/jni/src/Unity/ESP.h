#ifndef ESP_H
#define ESP_H

#include <jni.h>
#include <algorithm>
#include <cmath>
#include "Color.hpp"
#include "Rect.hpp"

class ESP {
private:
    JNIEnv *_env;
    jobject _cvsView;
    jobject _cvs;

public:
    ESP() {
        _env = nullptr;
        _cvsView = nullptr;
        _cvs = nullptr;
    }

    ESP(JNIEnv *env, jobject cvsView, jobject cvs) {
        this->_env = env;
        this->_cvsView = cvsView;
        this->_cvs = cvs;
    }

    bool isValid() const {
        return (_env != nullptr && _cvsView != nullptr && _cvs != nullptr);
    }

    int getWidth() const {
        if (isValid()) {
            jclass canvas = _env->GetObjectClass(_cvs);
            jmethodID width = _env->GetMethodID(canvas, "getWidth", "()I");
            return _env->CallIntMethod(_cvs, width);
        }
        return 0;
    }

    int getHeight() const {
        if (isValid()) {
            jclass canvas = _env->GetObjectClass(_cvs);
            jmethodID height = _env->GetMethodID(canvas, "getHeight", "()I");
            return _env->CallIntMethod(_cvs, height);
        }
        return 0;
    }

    void DrawLine(Color color, float thickness, Vector3 start, Vector3 end) {
        if (isValid()) {
            jclass canvasView = _env->GetObjectClass(_cvsView);
            jmethodID drawline = _env->GetMethodID(canvasView, "DrawLine",
                                                   "(Landroid/graphics/Canvas;IIIIFFFFF)V");
            _env->CallVoidMethod(_cvsView, drawline, _cvs, (int)color.a, (int)color.r,
                                 (int)color.g, (int)color.b,
                                 thickness,
                                 start.X, start.Y, end.X, end.Y);
        }
    }

    void DrawText(Color color, const char *txt, Vector2 pos, float size) {
        if (isValid()) {
            jclass canvasView = _env->GetObjectClass(_cvsView);
            jmethodID drawtext = _env->GetMethodID(canvasView, "DrawText",
                                                   "(Landroid/graphics/Canvas;IIIILjava/lang/String;FFF)V");
            jstring str = _env->NewStringUTF(txt);
            _env->CallVoidMethod(_cvsView, drawtext, _cvs, (int)color.a, (int)color.r,
                                 (int)color.g, (int)color.b,
                                 str, pos.X, pos.Y, size);
            _env->DeleteLocalRef(str);
        }
    }

    void DrawCircle(Color color, float stroke, Vector2 pos, float radius) {
        if (isValid()) {
            jclass canvasView = _env->GetObjectClass(_cvsView);
            jmethodID drawcircle = _env->GetMethodID(canvasView, "DrawCircle",
                                                     "(Landroid/graphics/Canvas;IIIIFFFF)V");
            _env->CallVoidMethod(_cvsView, drawcircle, _cvs, (int)color.a, (int)color.r,
                                 (int)color.g, (int)color.b, stroke, pos.X, pos.Y, radius);
        }
    }

    void DrawFilledRect(Color color, Vector3 pos, float width, float height) {
        if (isValid()) {
            jclass canvasView = _env->GetObjectClass(_cvsView);
            jmethodID drawfilledrect = _env->GetMethodID(canvasView, "DrawFilledRect",
                                                         "(Landroid/graphics/Canvas;IIIIFFFF)V");
            _env->CallVoidMethod(_cvsView, drawfilledrect, _cvs, (int)color.a, (int)color.r,
                                 (int)color.g, (int)color.b, pos.X, pos.Y, width, height);
        }
    }

    void DrawBox(Color color, float stroke, Rect rect) {
        Vector3 v1 = Vector3(rect.x, rect.y);
        Vector3 v2 = Vector3(rect.x + rect.width, rect.y);
        Vector3 v3 = Vector3(rect.x + rect.width, rect.y + rect.height);
        Vector3 v4 = Vector3(rect.x, rect.y + rect.height);

        DrawLine(color, stroke, v1, v2);
        DrawLine(color, stroke, v2, v3);
        DrawLine(color, stroke, v3, v4);
        DrawLine(color, stroke, v4, v1);
    }

    void DrawBox4Line(int x, int y, int w, int h, Color color, float thickness) {
        int iw = w / 4;
        int ih = h / 4;

        DrawLine(color, thickness, Vector3(x, y), Vector3(x + iw, y));
        DrawLine(color, thickness, Vector3(x + w - iw, y), Vector3(x + w, y));
        DrawLine(color, thickness, Vector3(x, y), Vector3(x, y + ih));
        DrawLine(color, thickness, Vector3(x + w - 1, y), Vector3(x + w - 1, y + ih));

        DrawLine(color, thickness, Vector3(x, y + h), Vector3(x + iw, y + h));
        DrawLine(color, thickness, Vector3(x + w - iw, y + h), Vector3(x + w, y + h));
        DrawLine(color, thickness, Vector3(x, y + h - ih), Vector3(x, y + h));
        DrawLine(color, thickness, Vector3(x + w - 1, y + h - ih), Vector3(x + w - 1, y + h));
    }

    void DrawVerticalHealthBar(Vector2 screenPos, float height, float maxHealth, float currentHealth, float width = 8.0f, bool showText = false) {
        float healthPercentage = std::min(1.0f, std::max(0.0f, currentHealth / maxHealth));
        float healthHeight = height * healthPercentage;
        
        Color healthColor;
        if (healthPercentage > 0.6f) {
            healthColor = Color(0, 255, 0, 200);
        } else if (healthPercentage > 0.3f) {
            healthColor = Color(255, 255, 0, 200);
        } else {
            healthColor = Color(255, 0, 0, 200);
        }
        
        float healthY = screenPos.Y + (height - healthHeight);
        DrawFilledRect(healthColor, Vector3(screenPos.X, healthY, 0), width, healthHeight);
        
        if (showText && height > 30) {
            char healthText[16];
            snprintf(healthText, sizeof(healthText), "%.0f", currentHealth);
            DrawText(Color(255, 255, 255, 200), 
                    healthText, 
                    Vector2(screenPos.X - 25, screenPos.Y + (height / 2) - 6), 
                    10.0f);
        }
    }

    // ========== WORLD TO SCREEN CONVERSION ==========
    // Yeh function world coordinates ko screen coordinates mein convert karega
    Vector3 WorldToScreen(Vector3 worldPos, float screenWidth, float screenHeight) {
        // Agar tumhare pass actual view/projection matrix hai toh woh use karo
        // Warna yeh temporary fix hai - direct coordinates use karo
        return worldPos;
    }

    // ========== FIXED: ESP DRAWING FUNCTIONS ==========
    
    // Draw ESP Box - FIXED: Ab head aur toe coordinates ko direct screen maane
    void DrawPlayerBox(Color color, float stroke, Vector3 headPos, Vector3 toePos, float screenWidth, float screenHeight) {
        if (headPos == Vector3::Zero() || toePos == Vector3::Zero()) return;
        
        // AGAR YE WORLD COORDINATES HAIN TOH YAHAN CONVERT KARO
        // Vector3 headScreen = WorldToScreen(headPos, screenWidth, screenHeight);
        // Vector3 toeScreen = WorldToScreen(toePos, screenWidth, screenHeight);
        
        // ABHI KE LIYE DIRECT USE KAR RAHA HU (AGAR SCREEN COORDINATES HAIN TOH)
        float headX = headPos.X;
        float headY = headPos.Y;
        float toeY = toePos.Y;
        
        // FIX: Box ko center mein lao
        float boxHeight = std::abs(toeY - headY) * 1.3f;  // Thoda height badhao
        float boxWidth = boxHeight * 0.5f;                // Width height ke hisaab se
        
        // Box ko head ke center se align karo
        float boxX = headX - (boxWidth / 2.0f);
        float boxY = headY - (boxHeight * 0.1f);
        
        // Screen bounds check
        if (boxX < 0) boxX = 0;
        if (boxY < 0) boxY = 0;
        if (boxX + boxWidth > screenWidth) boxWidth = screenWidth - boxX;
        if (boxY + boxHeight > screenHeight) boxHeight = screenHeight - boxY;
        
        // Agar box screen ke bahar hai toh skip karo
        if (boxX + boxWidth < 0 || boxY + boxHeight < 0 || boxX > screenWidth || boxY > screenHeight) {
            return;
        }
        
        Rect rect(boxX, boxY, boxWidth, boxHeight);
        DrawBox(color, stroke, rect);
    }
    
    // Draw 4-Line Box
    void DrawPlayerBox4Line(Vector3 headPos, Vector3 toePos, Color color, float thickness, float screenWidth, float screenHeight) {
        if (headPos == Vector3::Zero() || toePos == Vector3::Zero()) return;
        
        float headX = headPos.X;
        float headY = headPos.Y;
        float toeY = toePos.Y;
        
        float boxHeight = std::abs(toeY - headY) * 1.3f;
        float boxWidth = boxHeight * 0.5f;
        float boxX = headX - (boxWidth / 2.0f);
        float boxY = headY - (boxHeight * 0.1f);
        
        if (boxX < 0) boxX = 0;
        if (boxY < 0) boxY = 0;
        if (boxX + boxWidth > screenWidth) boxWidth = screenWidth - boxX;
        if (boxY + boxHeight > screenHeight) boxHeight = screenHeight - boxY;
        
        if (boxX + boxWidth < 0 || boxY + boxHeight < 0 || boxX > screenWidth || boxY > screenHeight) {
            return;
        }
        
        DrawBox4Line((int)boxX, (int)boxY, (int)boxWidth, (int)boxHeight, color, thickness);
    }
    
    // Draw Filled Box
    void DrawPlayerFilledBox(Vector3 headPos, Vector3 toePos, Color color, float screenWidth, float screenHeight) {
        if (headPos == Vector3::Zero() || toePos == Vector3::Zero()) return;
        
        float headX = headPos.X;
        float headY = headPos.Y;
        float toeY = toePos.Y;
        
        float boxHeight = std::abs(toeY - headY) * 1.3f;
        float boxWidth = boxHeight * 0.5f;
        float boxX = headX - (boxWidth / 2.0f);
        float boxY = headY - (boxHeight * 0.1f);
        
        if (boxX < 0) boxX = 0;
        if (boxY < 0) boxY = 0;
        if (boxX + boxWidth > screenWidth) boxWidth = screenWidth - boxX;
        if (boxY + boxHeight > screenHeight) boxHeight = screenHeight - boxY;
        
        if (boxX + boxWidth < 0 || boxY + boxHeight < 0 || boxX > screenWidth || boxY > screenHeight) {
            return;
        }
        
        Color fillColor = color;
        fillColor.a = 70;
        DrawFilledRect(fillColor, Vector3(boxX, boxY, 0), boxWidth, boxHeight);
        DrawBox(color, 1.5f, Rect(boxX, boxY, boxWidth, boxHeight));
    }
    
    // Draw Health Bar
    void DrawPlayerHealthBar(Vector3 headPos, Vector3 toePos, float health, float maxHealth, float screenWidth, float screenHeight) {
        if (headPos == Vector3::Zero() || toePos == Vector3::Zero()) return;
        
        float headX = headPos.X;
        float headY = headPos.Y;
        float toeY = toePos.Y;
        
        float boxHeight = std::abs(toeY - headY) * 1.3f;
        float boxWidth = boxHeight * 0.5f;
        float boxX = headX - (boxWidth / 2.0f);
        float boxY = headY - (boxHeight * 0.1f);
        
        float hBarW = 6.0f;
        float hBarX = boxX - (hBarW + 6);
        float hMax = maxHealth > 0 ? maxHealth : 100.0f;
        
        if (hBarX > 0 && boxY > 0 && boxY + boxHeight < screenHeight) {
            DrawVerticalHealthBar(Vector2(hBarX, boxY), boxHeight, hMax, health, hBarW, false);
        }
    }
    
    // Draw Name
    void DrawPlayerName(Vector3 headPos, const char* name, Color color, float textSize, float screenWidth, float screenHeight) {
        if (headPos == Vector3::Zero() || name == nullptr || name[0] == '\0') return;
        
        float headX = headPos.X;
        float headY = headPos.Y;
        
        float nameY = headY - textSize - 15;
        
        if (headX > 0 && headX < screenWidth && nameY > 0) {
            Color strokeColor = Color(0, 0, 0, 255);
            DrawText(strokeColor, name, Vector2(headX - 1, nameY - 1), textSize);
            DrawText(strokeColor, name, Vector2(headX + 1, nameY + 1), textSize);
            DrawText(strokeColor, name, Vector2(headX - 1, nameY + 1), textSize);
            DrawText(strokeColor, name, Vector2(headX + 1, nameY - 1), textSize);
            DrawText(color, name, Vector2(headX, nameY), textSize);
        }
    }
    
    // Draw Distance
    void DrawPlayerDistance(Vector3 headPos, Vector3 toePos, float distance, Color color, float textSize, float screenWidth, float screenHeight) {
        if (headPos == Vector3::Zero() || toePos == Vector3::Zero()) return;
        
        float headX = headPos.X;
        float headY = headPos.Y;
        float toeY = toePos.Y;
        
        float boxHeight = std::abs(toeY - headY) * 1.3f;
        float boxWidth = boxHeight * 0.5f;
        float boxX = headX - (boxWidth / 2.0f);
        float boxY = headY - (boxHeight * 0.1f);
        
        std::string dStr = std::to_string((int)distance) + "M";
        float dY = boxY + boxHeight + textSize + 5;
        float textX = headX;
        
        if (textX > 0 && textX < screenWidth && dY > 0 && dY < screenHeight) {
            Color strokeColor = Color(0, 0, 0, 255);
            DrawText(strokeColor, dStr.c_str(), Vector2(textX - 1, dY - 1), textSize);
            DrawText(strokeColor, dStr.c_str(), Vector2(textX + 1, dY + 1), textSize);
            DrawText(strokeColor, dStr.c_str(), Vector2(textX - 1, dY + 1), textSize);
            DrawText(strokeColor, dStr.c_str(), Vector2(textX + 1, dY - 1), textSize);
            DrawText(color, dStr.c_str(), Vector2(textX, dY), textSize);
        }
    }
    
    // Draw Line
    void DrawPlayerLine(Vector3 headPos, Vector3 toePos, Color color, float thickness, 
                        int lineType, float startXOffset, float startYOffset,
                        float screenWidth, float screenHeight) {
        if (headPos == Vector3::Zero() || toePos == Vector3::Zero()) return;
        
        float headX = headPos.X;
        float headY = headPos.Y;
        float toeY = toePos.Y;
        
        float boxHeight = std::abs(toeY - headY) * 1.3f;
        float boxWidth = boxHeight * 0.5f;
        float boxX = headX - (boxWidth / 2.0f);
        float boxY = headY - (boxHeight * 0.1f);
        
        Vector3 lineStart;
        switch(lineType) {
            case 0:
                lineStart = Vector3(screenWidth / 2 + startXOffset, 0 + startYOffset, 0);
                break;
            case 1:
                lineStart = Vector3(screenWidth / 2 + startXOffset, screenHeight / 2 + startYOffset, 0);
                break;
            case 2:
                lineStart = Vector3(screenWidth / 2 + startXOffset, screenHeight + startYOffset, 0);
                break;
            case 3:
                lineStart = Vector3(0 + startXOffset, screenHeight / 2 + startYOffset, 0);
                break;
            case 4:
                lineStart = Vector3(screenWidth + startXOffset, screenHeight / 2 + startYOffset, 0);
                break;
            default:
                lineStart = Vector3(screenWidth / 2, screenHeight / 2, 0);
        }
        
        Vector3 lineEnd = Vector3(boxX + (boxWidth / 2.0f), boxY, 0);
        
        // Ensure line end is within bounds
        if (lineEnd.X > 0 && lineEnd.X < screenWidth && lineEnd.Y > 0 && lineEnd.Y < screenHeight) {
            DrawLine(color, thickness, lineStart, lineEnd);
            DrawCircle(color, 5.0f, Vector2(lineStart.X, lineStart.Y), 4.0f);
        }
    }
};

#endif
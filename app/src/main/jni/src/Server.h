//
// Created by matadoryuki on 17/06/2024.
//

#include <jni.h>
#include <stdio.h>
#include <wchar.h>
#include "Includes/Logger.h"
#include "src/Unity/Quaternion.hpp"
#include "src/Unity/Vector3.hpp"

#include <src/Socket/server.h>
#include "src/Unity/Unity.h"
#include "src/Unity/Color.hpp"
#include "src/Unity/Rect.hpp"
#include "src/Unity/ESP.h"

#include <stdint.h>
#include "Includes/Utils.h"
#include "BooleanServer.h"
#include <thread>
#include <chrono>


/////// RJ PROFESSOR ROOT INJECTOR PROJECT LEKA///////


////// RJ PROFESSOR SABKA PAPA HAI ///////


////// ENJOY MY KIDS PROJECT///////







SocketServer server;

int g_screenWidth, g_screenHeight;

#define maxplayerCount 200

enum Mode {
    InitMode = 1,
    HackMode = 2,
    StopMode = 98,
    EspMode = 99,
};

struct Request {
    int Mode;
    bool m_IsOn;
    int value;
    int ScreenWidth;
    int ScreenHeight;
};

struct PlayerData {
    Vector3 Head;
    Vector3 Toe;
    bool IsCaido;
    float Distancia;
    int Health;
    bool Name;
    char PlayerName[64];
};

struct Response {
    bool Success;
    int PlayerCount;
    PlayerData Players[maxplayerCount];
};

int InitServer() {
    if (!server.Create()) {
        LOGE("[Server] Socket can't connect");
        return -1;
    }
    if (!server.Bind()) {
        LOGE("[Server] Socket can't connect");
        return -1;
    }
    if (!server.Listen()) {
        LOGE("[Server] Socket can't connect");
        return -1;
    }
    LOGI("[Server] Socket connected");
    return 0;
}

uintptr_t GetPlayerHeadTF(uintptr_t player) {
    auto ptr = Read<uintptr_t>(player + 0x648);
    return ptr;
}

uintptr_t GetPlayerPeTF(uintptr_t player) {
    auto ptr = Read<uintptr_t>(player + 0x670);
    return ptr;
}

uintptr_t GetPlayerMainCamera(uintptr_t player) {
    auto ptr = Read<uintptr_t>(player + 0x398);
    return ptr;
}

struct Matrix {
    Vector4 Position;
    Quaternion Rotation;
    Vector4 Scale;
};

static Vector3 GetPosition(uintptr_t Transform) {
    auto pos = Vector3::Zero();

    if (Transform == 0) return pos;

    auto transformObjValue = Read<uintptr_t>(Transform + 0x10);
    if (transformObjValue == 0) return pos;

    auto indexValue = Read<uintptr_t>(transformObjValue + 0x40);
    auto matrixValue = Read<uintptr_t>(transformObjValue + 0x38);

    if (matrixValue == 0) return pos;

    auto matrixListValue = Read<uintptr_t>(matrixValue + 0x18);
    auto matrixIndicesValue = Read<uintptr_t>(matrixValue + 0x20);

    auto resultValue = Read<Vector3>(matrixListValue + (indexValue * 0x30));

    int maxTries = 50;
    int tries = 0;

    auto transformIndexValue = Read<int>(matrixIndicesValue + (indexValue * 4));

    while (transformIndexValue >= 0 && tries < maxTries) {
        tries++;

        auto tMatrixValue = Read<Matrix>(matrixListValue + (transformIndexValue * 0x30));

        auto rotX = tMatrixValue.Rotation.X;
        auto rotY = tMatrixValue.Rotation.Y;
        auto rotZ = tMatrixValue.Rotation.Z;
        auto rotW = tMatrixValue.Rotation.W;

        auto scaleX = resultValue.X * tMatrixValue.Scale.X;
        auto scaleY = resultValue.Y * tMatrixValue.Scale.Y;
        auto scaleZ = resultValue.Z * tMatrixValue.Scale.Z;

        resultValue.X = tMatrixValue.Position.X + scaleX +
                        (scaleX * ((rotY * rotY * -2.0f) - (rotZ * rotZ * 2.0f))) +
                        (scaleY * ((rotW * rotZ * -2.0f) - (rotY * rotX * -2.0f))) +
                        (scaleZ * ((rotZ * rotX * 2.0f) - (rotW * rotY * -2.0f)));
        resultValue.Y = tMatrixValue.Position.Y + scaleY +
                        (scaleX * ((rotX * rotY * 2.0f) - (rotW * rotZ * -2.0f))) +
                        (scaleY * ((rotZ * rotZ * -2.0f) - (rotX * rotX * 2.0f))) +
                        (scaleZ * ((rotW * rotX * -2.0f) - (rotZ * rotY * -2.0f)));
        resultValue.Z = tMatrixValue.Position.Z + scaleZ +
                        (scaleX * ((rotW * rotY * -2.0f) - (rotX * rotZ * -2.0f))) +
                        (scaleY * ((rotY * rotZ * 2.0f) - (rotW * rotX * -2.0f))) +
                        (scaleZ * ((rotX * rotX * -2.0f) - (rotY * rotY * 2.0f)));

        transformIndexValue = Read<int>(matrixIndicesValue + (transformIndexValue * 4));
    }

    if (tries < maxTries) {
        pos = resultValue;
    }

    return pos;
}

static Vector3 GetNodePosition(uintptr_t nodeTransform) {
    auto transformValue = Read<uintptr_t>(nodeTransform + 0x10);
    if (transformValue == 0) {
        return Vector3::Zero();
    }
    return GetPosition(transformValue);
}

static auto WorldToScreenPoint(D3DMatrix viewMatrix, Vector3 ScreenPos) {
    auto result = Vector3(-1, -1, -1);

    auto v9 = (ScreenPos.X * viewMatrix._11) + (ScreenPos.Y * viewMatrix._21) + (ScreenPos.Z * viewMatrix._31) + viewMatrix._41;
    auto v10 = (ScreenPos.X * viewMatrix._12) + (ScreenPos.Y * viewMatrix._22) + (ScreenPos.Z * viewMatrix._32) + viewMatrix._42;
    auto v12 = (ScreenPos.X * viewMatrix._14) + (ScreenPos.Y * viewMatrix._24) + (ScreenPos.Z * viewMatrix._34) + viewMatrix._44;

    if (v12 >= 0.001f) {
        auto v13 = (float)g_screenWidth / 2.0f;
        auto v14 = (float)g_screenHeight / 2.0f;

        result.X = v13 + (v13 * v9) / v12;
        result.Y = v14 - (v14 * v10) / v12;
    }
    return result;
}
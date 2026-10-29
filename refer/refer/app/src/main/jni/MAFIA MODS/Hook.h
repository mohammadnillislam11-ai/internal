
///              JOIN TG :- @PRABIRXLIVE1             ///                   MADE BY :- @PRABIRXLIVE1               !!
///              JOIN TG :- @PRABIRXLIVE1             ///                   MADE BY :- @PRABIRXLIVE1               !!
///              JOIN TG :- @PRABIRXLIVE1             ///                   MADE BY :- @PRABIRXLIVE1               !!

/*
===============================[ 𝗠𝗔𝗗𝗘 𝗕𝗬 @𝗣𝗥𝗔𝗕𝗜𝗥𝗫𝗟𝗜𝗩𝗘𝟭 ]============================
===============================[ 𝗠𝗔𝗗𝗘 𝗕𝗬 @𝗣𝗥𝗔𝗕𝗜𝗥𝗫𝗟𝗜𝗩𝗘𝟭 ]============================
===============================[ 𝗠𝗔𝗗𝗘 𝗕𝗬 @𝗣𝗥𝗔𝗕𝗜𝗥𝗫𝗟𝗜𝗩𝗘𝟭 ]============================

===============================[ 𝗠𝗔𝗗𝗘 𝗕𝗬 @𝗣𝗥𝗔𝗕𝗜𝗥𝗫𝗟𝗜𝗩𝗘𝟭 ]============================
===============================[ 𝗠𝗔𝗗𝗘 𝗕𝗬 @𝗣𝗥𝗔𝗕𝗜𝗥𝗫𝗟𝗜𝗩𝗘𝟭 ]============================
===============================[ 𝗠𝗔𝗗𝗘 𝗕𝗬 @𝗣𝗥𝗔𝗕𝗜𝗥𝗫𝗟𝗜𝗩𝗘𝟭 ]============================

===============================[ 𝗠𝗔𝗗𝗘 𝗕𝗬 @𝗣𝗥𝗔𝗕𝗜𝗥𝗫𝗟𝗜𝗩𝗘𝟭 ]============================
===============================[ 𝗠𝗔𝗗𝗘 𝗕𝗬 @𝗣𝗥𝗔𝗕𝗜𝗥𝗫𝗟𝗜𝗩𝗘𝟭 ]============================
===============================[ 𝗠𝗔𝗗𝗘 𝗕𝗬 @𝗣𝗥𝗔𝗕𝗜𝗥𝗫𝗟𝗜𝗩𝗘𝟭 ]============================
*/






#pragma once
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>
#include <pthread.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <Il2Cpp.h>
#include <Tools.h>
#include <Vector3.hpp>
#include <Color.h>
#include <MAFIA MODS/main.h>
#include "obfuscate.h"
#include "Unity/unity.h"
#include "Class.h"
#include "Rect.h"
inline static int g_GlHeight, g_GlWidth;
#include "Bools.h"
#include <math.h>
#include <stdio.h>
typedef char PACKAGENAME;


struct sColor {
        
        float line[4] = {45 / 255.0f, 180 / 255.0f, 45 / 255.0f, 255 / 255.0f};
        float box[4] = {45 / 255.0f, 180 / 255.0f, 45 / 255.0f, 255 / 255.0f};
        
};
    
    sColor Color;
    template <typename T>
    struct UnityArray {
    void* klass;
    void* monitor;
    void* bounds;
    int max_length;
    T vector[64]; 
};

struct Vector2 {
    float x, y;
};

struct DictionaryEntry {
    int hashCode;
    int next;
    void* key;
    void* value;
};

struct MyDictionary {
    void* klass;
    void* monitor;
    void* buckets;
    UnityArray<DictionaryEntry>* entries;
    int count;
};

bool isOutsideScreen(ImVec2 pos, ImVec2 screen) {
    if (pos.y < 0) {
        return true;
    }
    if (pos.x > screen.x) {
        return true;
    }
    if (pos.y > screen.y) {
        return true;
    }
    return pos.x < 0;
}

ImVec2 pushToScreenBorder(ImVec2 Pos, ImVec2 screen, int offset) {
    int x = (int) Pos.x;
    int y = (int) Pos.y;
    
    if (Pos.y < 0) {
        y = -offset;
    }
    
    if (Pos.x > screen.x) {
        x = (int) screen.x + offset;
    }
    
    if (Pos.y > screen.y) {
        y = (int) screen.y + offset;
    }
    
    if (Pos.x < 0) {
        x = -offset;
    }
    return ImVec2(x, y);
}

static float tm = 127 / 255.f;

void DrawCircleHealth(ImVec2 position, int health, int max_health, float radius) {
    float a_max = ((3.14159265359f * 2.0f));
    ImU32 healthColor = IM_COL32(45, 180, 45, 255);
    if (health <= (max_health * 0.6)) {
        healthColor = IM_COL32(180, 180, 45, 255);
    }
    if (health < (max_health * 0.3)) {
        healthColor = IM_COL32(180, 45, 45, 255);
    }
    ImGui::GetForegroundDrawList()->PathArcTo(position, radius, (-(a_max / 4.0f)) + (a_max / max_health) * (max_health - health), a_max - (a_max / 4.0f));
    ImGui::GetForegroundDrawList()->PathStroke(healthColor, ImDrawFlags_None, 4);
}



void drawText2(int x, int y, long color, const char *str, float size) {
        ImGui::GetBackgroundDrawList()->AddText(NULL, size, ImVec2(x, y), color, str);
    }


void DrawRectFilled(int x, int y, int w, int h, ImColor color) {
    ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h),
                                               color, 0, 0);
}


void DrawRectFilledHealth(int x, int y, int w, int h, ImVec4 color) {
    ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h),
                                               ImGui::ColorConvertFloat4ToU32(color), 0, 0);
}


void DrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, int color, int T) {
    ImGui::GetBackgroundDrawList()->AddTriangleFilled(ImVec2(x1, y1), ImVec2(x2, y2), ImVec2(x3, y3),
                                                   color);
}
bool IsVisible(void * player){
    if(player != NULL) {
        void *hitObj = NULL;
        

        
        Vector3 cameraLocation = Transform_GetPosition(Component_GetTransform(Camera_main()));
        Vector3 headLocation = Transform_GetPosition(Component_GetTransform(Player_GetHeadCollider(player)));
            return !Physics_Raycast(cameraLocation, headLocation, 12, &hitObj);
    }
    return false;
}


void DrawHealths(Rect box, float entityHealth, float maxHealth, int TeamID, const char* name, long color) {
    float x = box.x - (140 - box.w) / 2;
    float y = box.y;
    char dis[20];
    // sprintf(dis, "%0.fM", d);

    char TeamText[20];
    sprintf(TeamText, "%d", (int)TeamID);
    ImVec4 HPColor = ImVec4{1, 1, 0, 127 / 255.f}; // Màu mặc định cho máu

    if (entityHealth < maxHealth) {
        float healthPercentage = entityHealth / maxHealth;

        if (healthPercentage < 0.3f) {
            HPColor = ImVec4{0.5f, 0.0f, 0.0f, 127 / 255.f}; // Màu đỏ cho máu thấp
        } else if (healthPercentage < 0.6f) {
            HPColor = ImVec4{1, 0, 0, 127 / 255.f}; // Màu cam cho máu trung bình
        }
    }

    tm = 100.f / 255.f;
    DrawRectFilled(x - strlen(name), y - 41, 120 + strlen(name) * 2, 20, color); // Đám mây nền cho tên người chơi
    tm = 255.f / 255.f;
    DrawRectFilled(x - strlen(name), box.y - 41, 25, 20, color);
    if (strlen(TeamText) < 2) {
        drawText2(x + 6 - strlen(name), box.y - 42, ImColor(255,255,255),
                       TeamText, 18.943);//队伍ID
    } else {
        drawText2(x - strlen(name), box.y - 42, ImColor(255,255,255),
                       TeamText, 18.943);//队伍ID
    }
    drawText2(x + 28 - strlen(name), y - 43,
                   ImColor(255,255,255), name, 18.943);//玩家名称
    float maxWidth = 120; // Độ dài tối đa của thanh máu
    float healthBarWidth = entityHealth * maxWidth / maxHealth; // Độ dài thanh máu dựa trên tỷ lệ máu hiện tại và máu tối đa
    DrawRectFilledHealth(x - maxWidth / 2, y - 18, healthBarWidth, 8, HPColor);// cập nhật vị trí và kích thước của thanh máu
    drawText2(x + 125 + strlen(name), y - 43, ImColor(255,255,255), dis,18.943);//距离显示
    DrawTriangle(box.x + box.w / 2 - 10, y - 8, box.x + box.w / 2 + 15 - 10, y - 8,
                 box.x + box.w / 2 - 2, y, ImColor(255,255,255), 1);//第一个左边 第二个右边 第三个底边
}



static float SetFieldOfView = 0, GetFieldOfView = 0;

void DroneView() {
    if (GetFieldOfView == 0) {
        GetFieldOfView = get_fieldOfView();
    }
    if (SetFieldOfView > 0 && GetFieldOfView != 0) {
        set_fieldOfView((float)GetFieldOfView + SetFieldOfView);
    }
    if (SetFieldOfView == 0 && GetFieldOfView != 0) {
        set_fieldOfView((float)GetFieldOfView);
    }
}



void *GetClosestEnemy() {
    float shortestDistance = 9999;
    void* closestEnemy = NULL;   
    void* get_MatchGame = Curent_Match();
    void* LocalPlayer = GetLocalPlayer(get_MatchGame);
    if (LocalPlayer != NULL && get_MatchGame != NULL && Enable) {
        MyDictionary* dict = *(MyDictionary**)((uintptr_t)get_MatchGame + ListPlayer);
        if (dict != nullptr && dict->entries != nullptr) {
            void* camera = Camera_main();
            int total = dict->count;
            for (int u = 0; u < total; u++) {
                void* Player = dict->entries->vector[u].value;       
                if (Player != NULL && Player != LocalPlayer && !get_isLocalTeam(Player) && !get_IsDieing(Player) && get_isVisible(Player) && get_MaxHP(Player)) {    
                    Vector3 PlayerPos = getPosition(Player);
                    Vector3 LocalPlayerPos = getPosition(LocalPlayer);
                    Vector3 pos2 = WorldToScreenPoint(camera, PlayerPos);     
                    bool isFov1 = isFov(Vector3(pos2.x, pos2.y), Vector3(g_GlWidth / 2, g_GlHeight / 2), Fov_Aim);
                    float distance = Vector3::Distance(LocalPlayerPos, PlayerPos);
                    if (distance < Aimdis) {
                        Vector3 targetDir = Vector3::Normalized(PlayerPos - LocalPlayerPos);
                        float angle = Vector3::Angle(targetDir, GetForward(Component_GetTransform(camera))) * 100.0;             
                    if (angle <= Fov_Aim && isFov1 && angle < shortestDistance) {                      shortestDistance = angle;
                    closestEnemy = Player;
                        }
                    }
                }
            }
        }
    }
    return closestEnemy;
}


extern bool SpeedHack;    
bool SpeedHack = false;
bool saved = false;
float savedScreenWidth = 0;
float savedScreenHeight = 0;
float active = 0.0f;
float desactive = 0.0f;
void UpdateSpeedhack() {
    if (Enable) {
        void* Simulation = GetSimulationTimer();
     if (Simulation != nullptr) {
   float FixedDeltaTime = GetTimer(Simulation);
            if(!saved) {
                active = FixedDeltaTime * 1.82f;
                desactive = FixedDeltaTime;
                saved = true;
            }
   if (SpeedHack) {
       if (FixedDeltaTime != active) {
                    SetTimer(Simulation, active);
    }
   }
   else {
       if (FixedDeltaTime != desactive) {
                    SetTimer(Simulation, desactive);
    }
   }
  }
 }
}




float (*get_deltaTime)();
float smoothSpeedAIM = 5; 
void *MyPlayer = NULL;
bool Drone;
const float RADAR_SIZE = 200.0f; 
const ImVec2 RADAR_CENTER(100.0f, 100.0f); 
float x,y;
float enemy1 = 2.0f;

bool RenderWatermark = false;
bool block_connections = true;
#include <string>
ImVec4 colorTextBackground = ImVec4(0, 0, 0, 255); 
ImVec4 colorBox = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);          
ImVec4 colorHealth = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); 
ImVec4 colorDistance = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); 
ImVec4 colorLine = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);        
ImVec4 colorHexagono = ImVec4(150 / 255.0f, 0, 255 / 255.0f, 1.0f);
ImVec4 colorMenuText = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
ImVec4 NeonColor = ImVec4(0.0f, 0.8f, 1.0f, 1.0f); 

void* CurrentMatch = nullptr;
void* closestEnemy = nullptr;
void* LocalPlayer = nullptr;


float aimSmoothness = 0.1f;

inline void DrawAIMBOT(float screenWidth, float screenHeight) {
     
}

inline void DrawESP(float screenWidth, float screenHeight) {
 ImDrawList*draw = ImGui::GetBackgroundDrawList();
    if (!draw) return;
    
	
int totalEnemies = 0; 


int SwapDelay = 200; 

static float fireTimer = 0.1f;
static bool isFiring = false;


if (Aimbot) {

	
void* CurrentMatch = Curent_Match();
void* closestEnemy = GetClosestEnemy();
void* LocalPlayer = GetLocalPlayer(CurrentMatch);


if (closestEnemy != NULL && LocalPlayer != NULL && CurrentMatch != NULL) {
    Vector3 EnemyLocation = GetHeadPosition(closestEnemy);
    Vector3 PlayerLocation = CameraMain(LocalPlayer);
    Quaternion PlayerLook = GetRotationToLocation(EnemyLocation, 0.1f, PlayerLocation);
    bool IsScopeOn = get_IsSighting(LocalPlayer);
    bool IsFiring = get_IsFiring(LocalPlayer);
    
    if (AimWhen == 0) {
        set_aim(LocalPlayer, PlayerLook);
    }
    else if (AimWhen == 1 && IsFiring) {
        set_aim(LocalPlayer, PlayerLook);
    }
    else if (AimWhen == 2 && IsScopeOn) {
        set_aim(LocalPlayer, PlayerLook);
    }
}
}


if (Enable) {
    int enemyIndex = 0; 
    void* current_Match = Curent_Match();
    void* local_player = GetLocalPlayer(current_Match);
    
float time = ImGui::GetTime() * 1.5f;
ImU32 global_rgb = ImColor::HSV(fmodf(time, 1.0f), 1.0f, 1.0f);

    
    if (local_player && current_Match) {
        MyDictionary* players = *(MyDictionary**)((uintptr_t)current_Match + ListPlayer);
        void* camera = Camera_main();
        if (players && players->entries && camera) {
            int total = players->count;
            for (int u = 0; u < total; u++) {
                void* closestEnemy = players->entries->vector[u].value;

                if (closestEnemy && closestEnemy != local_player && get_isVisible(closestEnemy) && !get_isLocalTeam(closestEnemy)) {enemyIndex++;
                    Vector3 Toepos = getPosition(closestEnemy);
                    Vector3 Toeposi = WorldToScreenPoint(camera, Toepos);
                    if (Toeposi.z < 1) continue;
                    Vector3 HeadPos = Toepos + Vector3(0, 1.9f, 0);
                    Vector3 HeadPosition = WorldToScreenPoint(camera, HeadPos);
                    if (HeadPosition.z < 1) continue;
                    draw->AddCircle(ImVec2(screenWidth / 2, screenHeight / 2), Fov_Aim, global_rgb, 100, 2.5f);
                     float distance = Vector3::Distance(getPosition(local_player), Toepos);                     				 					 
					 float Height = abs(HeadPosition.y - Toeposi.y) * 1.09f;
                     float Width = Height * 0.55f;                                                    
				     Rect rect = Rect(HeadPosition.x - Width / 2.f, screenHeight - HeadPosition.y, Width, Height);														if (Config.ESP.Health) {           
                        long clr = ImColor(0, 255, 0, 255);
                        if (GetHp(closestEnemy) <= (get_MaxHP(closestEnemy) * 0.6)) clr = ImColor(255, 255, 0, 255);
                        if (GetHp(closestEnemy) < (get_MaxHP(closestEnemy) * 0.3)) clr = ImColor(255, 0, 0, 255);
                        int xx = rect.x + rect.w + 2;
                        draw->AddRectFilled(ImVec2(xx, rect.y), ImVec2(xx + 5, rect.y + rect.h), ImColor(0, 0, 0, 255));        
                        draw->AddRectFilled(ImVec2(xx + 1, rect.y + rect.h - (rect.h * ((float)GetHp(closestEnemy) / get_MaxHP(closestEnemy)))), ImVec2(xx + 4, rect.y + rect.h), clr);
                    }
                    if (Config.ESP.Line) {
                        draw->AddLine(ImVec2(screenWidth / 2, 40), ImVec2(rect.x + rect.w / 2, rect.y), global_rgb, 2.0f);
                    }
                    if (Config.ESP.Box) {
                        draw->AddRect(ImVec2(rect.x, rect.y), ImVec2(rect.x + rect.w, rect.y + rect.h), global_rgb, 0, 0, 2.5f);
                        
                        for (int i = 0; i < 2; i++) {
                            float alpha = 0.12f / (i + 1);
                            ImU32 glow_col = ImColor::HSV(fmodf(time, 1.0f), 1.0f, 1.0f, alpha);
                            draw->AddRect(ImVec2(rect.x, rect.y), ImVec2(rect.x + rect.w, rect.y + rect.h), glow_col, 0, 0, 2.5f + (i * 2.0f));
                        }
                    }
                    if (Config.ESP.MAFIA) {
                        monoString* Nick = get_NickName(closestEnemy);
                        std::string name = "Enemy"; 
                        if (Nick != nullptr) {
                            name = "";
                        }
                        std::string distText = std::to_string((int)distance) + "M";
                        float fontScale = 0.75f;
                        ImVec2 nameSize = ImGui::CalcTextSize(name.c_str());
                        
                        float posX = rect.x + rect.w * 0.5f - (nameSize.x * fontScale * 0.5f);
                        float posY = rect.y - 28.0f;

                        draw->AddRect(ImVec2(posX - 5, posY - 2), ImVec2(posX + nameSize.x + 45, posY + 18), global_rgb, 3.0f, 0, 1.5f);
                        draw->AddRectFilled(ImVec2(posX - 5, posY - 2), ImVec2(posX + nameSize.x + 45, posY + 18), ImColor(0, 0, 0, 150), 3.0f);
                        draw->AddText(NULL, 18.0f, ImVec2(posX, posY), ImColor(255, 255, 255), name.c_str());
                        draw->AddText(NULL, 18.0f, ImVec2(posX + nameSize.x + 10, posY), global_rgb, distText.c_str());}}}}}}
    

if (RenderWatermark) {
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    ImVec2 wmSize = ImVec2(250, 40);
    ImVec2 wmPos = ImVec2((displaySize.x - wmSize.x) / 2, 20); 
    ImVec2 wmEnd = ImVec2(wmPos.x + wmSize.x, wmPos.y + wmSize.y);
    ImColor themeColor = ImColor(
        (int)(NeonColor.x * 255),
        (int)(NeonColor.y * 255),
        (int)(NeonColor.z * 255),
        (int)(NeonColor.w * 255)
    );
    ImGui::GetBackgroundDrawList()->AddRectFilled(
        wmPos, wmEnd,
        IM_COL32(15, 15, 25, 220), 
        20.0f
    );
    ImGui::GetBackgroundDrawList()->AddRect(
        wmPos, wmEnd,
        themeColor,
        20.0f, 0, 2.0f
    );
    for (int i = 0; i < 3; i++) {
        float thickness = 4.0f + i * 2.5f;
        float alpha = 0.12f / (i + 1);
        ImColor glowColor = ImColor(
            (int)(NeonColor.x * 255),
            (int)(NeonColor.y * 255),
            (int)(NeonColor.z * 255),
            (int)(NeonColor.w * 255 * alpha)
        );
        ImGui::GetBackgroundDrawList()->AddRect(
            wmPos, wmEnd,
            glowColor, 20.0f, 0, thickness
        );
    }
    std::string watermarkText = "DJ ALOK";
    ImVec2 textSize = ImGui::CalcTextSize(watermarkText.c_str());
    ImVec2 textPos = ImVec2(
        wmPos.x + (wmSize.x - textSize.x) / 2,
        wmPos.y + (wmSize.y - textSize.y) / 2
    );
    ImGui::GetBackgroundDrawList()->AddText(
        textPos,
        ImColor(
            (int)(NeonColor.x * 255),
            (int)(NeonColor.y * 255),
            (int)(NeonColor.z * 255),
            255
        ),
        watermarkText.c_str()
    );
 
 
 if (totalEnemies > 0) {
    // Text banate hai "Enemy: X"
    std::string enemyText = "ENEMY: " + std::to_string(totalEnemies);

    // Text size nikalte hai
    ImVec2 textSize = ImGui::CalcTextSize(enemyText.c_str());

    ImVec2 padding(14.0f, 4.0f);  // 👈 yaha 8 se 4 kar diya height kam karne ke liye

    ImVec2 center(screenWidth * 0.5f, 50.0f);

    // Box ka min-max point
    ImVec2 boxMin(center.x - textSize.x * 0.5f - padding.x, center.y - textSize.y * 0.5f - padding.y);
    ImVec2 boxMax(center.x + textSize.x * 0.5f + padding.x, center.y + textSize.y * 0.5f + padding.y);

    // Box background
    draw->AddRectFilled(boxMin, boxMax, ImColor(0, 0, 0, 200), 8.0f);

    // 🌈 Rainbow Border
    float thickness = 3.0f;
    int rainbowSegments = 50; 
    float hue = (float)(ImGui::GetTime()) * 0.2f; 

    for (int i = 0; i < rainbowSegments; i++) {
        float t0 = (float)i / rainbowSegments;
        float t1 = (float)(i + 1) / rainbowSegments;

        ImColor col0 = ImColor::HSV(fmodf(hue + t0, 1.0f), 1.0f, 1.0f);

        // Top
        draw->AddLine(
            ImVec2(boxMin.x + (boxMax.x - boxMin.x) * t0, boxMin.y),
            ImVec2(boxMin.x + (boxMax.x - boxMin.x) * t1, boxMin.y),
            col0, thickness);

        // Bottom
        draw->AddLine(
            ImVec2(boxMin.x + (boxMax.x - boxMin.x) * t0, boxMax.y),
            ImVec2(boxMin.x + (boxMax.x - boxMin.x) * t1, boxMax.y),
            col0, thickness);

        // Left
        draw->AddLine(
            ImVec2(boxMin.x, boxMin.y + (boxMax.y - boxMin.y) * t0),
            ImVec2(boxMin.x, boxMin.y + (boxMax.y - boxMin.y) * t1),
            col0, thickness);

        // Right
        draw->AddLine(
            ImVec2(boxMax.x, boxMin.y + (boxMax.y - boxMin.y) * t0),
            ImVec2(boxMax.x, boxMin.y + (boxMax.y - boxMin.y) * t1),
            col0, thickness);
    }

    ImVec2 textPos(center.x - textSize.x * 0.5f, center.y - textSize.y * 0.5f);
    draw->AddText(textPos, ImColor(255, 255, 255, 255), enemyText.c_str());
      }
    }
 }

 


bool Guest;
bool (*ResetGuest)(void* _this);
bool _ResetGuest(void* _this) {
if (Guest) {return true;}
return ResetGuest(_this);}
bool camxa;
float (*GetCameraHeightRateValue)(void *player);
float _GetCameraHeightRateValue(void *player) {
if (player != nullptr) {
if (camxa) {return 4.0f;}}
return GetCameraHeightRateValue(player);}
bool(*Bypass)(void* bypass); 
bool _Bypass(void* bypass) {return false;}

bool SniperFast = false;

bool (*_IsContinuousGen)(void *instance) = nullptr;
bool IsContinuousGen(void *instance) {
    if (instance != NULL && Enable && SniperFast) {
        return true;
    }
    return _IsContinuousGen(instance);
}




bool PcSwapAnim = false;

void (*orig_PlayChangeWeaponAnim)(void*, void*, float);
void hook_PlayChangeWeaponAnim(void* _this, void* a, float b) {
    if (Enable && PcSwapAnim) {
        return;
    }
    orig_PlayChangeWeaponAnim(_this, a, b);
}


bool Setindia = false;


int (*orig_ClientVersion)(void* _this);
int hook_ClientVersion(void* _this) {
    if (Setindia) {
        return 3;
    }
    return orig_ClientVersion(_this);
}

bool Fastswitch = false;

bool (*get_InSwapWeaponCD)(void* thiz);
bool _get_InSwapWeaponCD(void* thiz) {
  if (Fastswitch && Enable) {
        return false;
    }
    return get_InSwapWeaponCD(thiz);
}

bool AutoRun = false;

bool (*orig_get_IsFastRunning)(void *instance);
bool hook_get_IsFastRunning(void *instance) {
    if (AutoRun) {
        return true;
    }
    return orig_get_IsFastRunning(instance);
}

bool InfiniteRange = false; 

float (*orig_get_Range)(void *instance);
float hook_get_Range(void *instance) {
   
    if (Enable && instance && InfiniteRange) {
        return 9999.0f; 
    }
    return orig_get_Range(instance);
}



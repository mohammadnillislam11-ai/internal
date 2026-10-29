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






#include <android/log.h>
#include <unistd.h>
#include <thread>
#include <limits>
#include <xdl.h>
#include <KittyUtils.h>
#include <KittyMemory.h>
#include <Il2Cpp.h>
#include <SubstrateHook.h>
#include <CydiaSubstrate.h>
#include "MAFIA MODS/Gui.hpp"
#include <MAFIA MODS/main.h>
#include "fonts/FontAwesome6_solid.h"
#include "ImGui/Toggle.h"




static bool isCapsule = false;
static float currentWidth = 650.0f;
static float currentHeight = 380.0f;




static int currentTab = 0; 
void hack_thread();
bool ImGuiOK = true;



JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {

    jvm = vm;


    std::thread(hack_thread).detach();

    return JNI_VERSION_1_6;
}

void DrawThickCornerRGB(ImVec2 p_min, ImVec2 p_max, float thickness) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    float time = ImGui::GetTime() * 1.5f;
    float corner_size = 25.0f;
    ImU32 col = ImColor::HSV(fmodf(time, 1.0f), 1.0f, 1.0f);
    draw_list->AddRect(p_min, p_max, col, 0.0f, 0, 1.5f);
    draw_list->AddLine(p_min, ImVec2(p_min.x + corner_size, p_min.y), col, thickness);
    draw_list->AddLine(p_min, ImVec2(p_min.x, p_min.y + corner_size), col, thickness);
    draw_list->AddLine(ImVec2(p_max.x, p_min.y), ImVec2(p_max.x - corner_size, p_min.y), col, thickness);
    draw_list->AddLine(ImVec2(p_max.x, p_min.y), ImVec2(p_max.x, p_min.y + corner_size), col, thickness);
    draw_list->AddLine(ImVec2(p_min.x, p_max.y), ImVec2(p_min.x + corner_size, p_max.y), col, thickness);
    draw_list->AddLine(ImVec2(p_min.x, p_max.y), ImVec2(p_min.x, p_max.y - corner_size), col, thickness);
    draw_list->AddLine(p_max, ImVec2(p_max.x - corner_size, p_max.y), col, thickness);
    draw_list->AddLine(p_max, ImVec2(p_max.x, p_max.y - corner_size), col, thickness);
}



static bool itsmk = true;

ImVec4 GreenAccent = ImVec4(0.00f, 0.85f, 0.45f, 1.00f); 
ImVec4 YellowAccent = ImVec4(1.00f, 0.80f, 0.00f, 1.00f);
ImVec4 DarkBg     = ImVec4(0.08f, 0.09f, 0.11f, 1.00f);
ImVec4 TabBg      = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
ImVec4 TextWhite  = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);

void SetNeonBlueTheme() {
    ImGuiStyle *style = &ImGui::GetStyle();
// --- General Styling and Spacing ---
style->WindowPadding = ImVec2(10, 10);
style->WindowRounding = 5.0f;
style->FramePadding = ImVec2(10, 8);
style->ItemSpacing = ImVec2(8, 8);
style->ItemInnerSpacing = ImVec2(8, 6);
style->FrameRounding = 4.0f;
style->ChildRounding = 5.0f;
style->WindowBorderSize = 0.5f;
style->FrameBorderSize = 0.5f;
style->PopupRounding = 12.0f;
style->ScrollbarSize = 0.0f;
style->ScrollbarRounding = 6.0f;
style->GrabRounding = 6.0f;
style->GrabMinSize = 20.0f;
style->TabRounding = 8.0f;
style->TabBorderSize = 0.0f;
style->ButtonTextAlign = ImVec2(0.5f, 0.5f);
style->WindowTitleAlign = ImVec2(0.5f, 0.5f);

ImVec4* colors = style->Colors;

 //Dark Base Colors (Kept the same) ---
colors[ImGuiCol_WindowBg]               = ImColor(25, 25, 28, 255); // Main dark background
colors[ImGuiCol_ChildBg]                = ImColor(45, 45, 50, 255); // Child windows/groups background
colors[ImGuiCol_PopupBg]                = ImColor(40, 40, 43, 255); // Popup/Tooltip background
colors[ImGuiCol_MenuBarBg]              = ImColor(30, 30, 33, 255); // Menu bar background

// Text (Kept the same)
colors[ImGuiCol_Text]                   = ImColor(240, 240, 240, 255); // Light text
colors[ImGuiCol_TextDisabled]           = ImColor(150, 150, 150, 255); // Medium grey for disabled text

// Frames (inputs, buttons) - Blue colors
colors[ImGuiCol_FrameBg]                = ImColor(35, 55, 85, 255); // Dark blue frame background
colors[ImGuiCol_FrameBgHovered]         = ImColor(45, 75, 115, 255);
colors[ImGuiCol_FrameBgActive]          = ImColor(55, 95, 145, 255);

// --- Striking Blue Accent Colors ---

// Headers and Collapsing Headers - Blue shades
colors[ImGuiCol_Header]                 = ImColor(0, 80, 160, 120); // Deep Blue
colors[ImGuiCol_HeaderHovered]          = ImColor(0, 100, 200, 150); // Bright Blue
colors[ImGuiCol_HeaderActive]           = ImColor(0, 120, 240, 180); // Electric Blue

// Buttons - Vibrant blue colors
colors[ImGuiCol_Button]                 = ImColor(0, 100, 200, 255); // Strong Blue
colors[ImGuiCol_ButtonHovered]          = ImColor(30, 144, 255, 220); // Dodger Blue
colors[ImGuiCol_ButtonActive]           = ImColor(70, 130, 255, 200); // Royal Blue

// Selection and Highlights - Blue accents
colors[ImGuiCol_CheckMark]              = ImColor(0, 191, 255, 255); // Deep Sky Blue
colors[ImGuiCol_SliderGrab]             = ImColor(65, 105, 225, 255); // Royal Blue
colors[ImGuiCol_SliderGrabActive]       = ImColor(30, 144, 255, 255); // Dodger Blue
colors[ImGuiCol_TextSelectedBg]         = ImColor(0, 120, 215, 100); // Bright Blue with alpha
colors[ImGuiCol_NavHighlight]           = ImColor(0, 150, 255, 255); // Bright Blue

// Borders and separators - Blue accents
colors[ImGuiCol_Border]                 = ImColor(0, 80, 160, 255); // Deep Blue
colors[ImGuiCol_Separator]              = ImColor(70, 130, 180, 255); // Steel Blue
colors[ImGuiCol_SeparatorHovered]       = ImColor(100, 149, 237, 255); // Cornflower Blue
colors[ImGuiCol_SeparatorActive]        = ImColor(30, 144, 255, 255); // Dodger Blue

// Scrollbars - Blue theme
colors[ImGuiCol_ScrollbarBg]            = ImColor(35, 45, 60, 255); // Dark blue track
colors[ImGuiCol_ScrollbarGrab]          = ImColor(70, 130, 180, 255); // Steel Blue
colors[ImGuiCol_ScrollbarGrabHovered]   = ImColor(100, 149, 237, 255); // Cornflower Blue
colors[ImGuiCol_ScrollbarGrabActive]    = ImColor(30, 144, 255, 255); // Dodger Blue

// Drag and Drop
colors[ImGuiCol_DragDropTarget]         = ImColor(0, 191, 255, 255); // Bright Cyan Blue

// Tabs - Blue colors
colors[ImGuiCol_Tab]                    = ImColor(35, 45, 60, 255); // Unfocused tab background
colors[ImGuiCol_TabHovered]             = ImColor(0, 100, 200, 200); // Bright Blue
colors[ImGuiCol_TabActive]              = ImColor(30, 144, 255, 255); // Dodger Blue
colors[ImGuiCol_TabUnfocused]           = ImColor(25, 35, 45, 255); // Unfocused tab
colors[ImGuiCol_TabUnfocusedActive]     = ImColor(65, 105, 225, 150); // Royal Blue

// Titles - Blue gradient
colors[ImGuiCol_TitleBg]                = ImColor(20, 40, 80, 255); // Dark blue title bar
colors[ImGuiCol_TitleBgActive]          = ImColor(0, 80, 160, 255); // Deep Blue
colors[ImGuiCol_TitleBgCollapsed]       = ImColor(15, 30, 60, 255);

// Resizing - Blue grips
colors[ImGuiCol_ResizeGrip]             = ImColor(30, 144, 255, 255); // Dodger Blue
colors[ImGuiCol_ResizeGripHovered]      = ImColor(100, 149, 237, 255); // Cornflower Blue
colors[ImGuiCol_ResizeGripActive]       = ImColor(70, 130, 255, 255); // Light Royal Blue

// Navigation - Blue accent
colors[ImGuiCol_NavWindowingHighlight]  = ImColor(0, 150, 255, 255); // Bright Blue
colors[ImGuiCol_PlotLines]              = ImColor(0, 191, 255, 255); // Deep Sky Blue
colors[ImGuiCol_PlotLinesHovered]       = ImColor(30, 144, 255, 255); // Dodger Blue
colors[ImGuiCol_PlotHistogram]          = ImColor(65, 105, 225, 255); // Royal Blue
colors[ImGuiCol_PlotHistogramHovered]   = ImColor(100, 149, 237, 255); // Cornflower Blue
}



void(*Bypass1)(void* _this);
void _Bypass1(void* _this) {
    return;
}
void(*Bypass2)(void* _this);
void _Bypass2(void* _this) {
    return;
}
void (*BYPAS)() = 0;
void _BYPAS(void* unk) {
    return;
}


bool ManuSlider(const char* label, float* value, float min, float max, const char* format = "%.0f") {

    ImGui::Text("%s: ", label);
    ImGui::SameLine();
    ImGui::TextColored(YellowAccent, format, *value);

    float sliderWidth = ImGui::GetContentRegionAvail().x - 10.0f;  
    ImVec2 pos = ImGui::GetCursorScreenPos();  
    ImDrawList* draw = ImGui::GetWindowDrawList();  
    
    ImGui::InvisibleButton(label, ImVec2(sliderWidth, 22));  
    bool changed = false;  
    if (ImGui::IsItemActive()) {  
        float t = (ImGui::GetIO().MousePos.x - pos.x) / sliderWidth;  
        t = ImClamp(t, 0.f, 1.f);  
        *value = min + (max - min) * t;  
        changed = true;  
    }  
    
    float fraction = (*value - min) / (max - min);  
    float radius = 11.f; 
    float knobX = pos.x + radius + (sliderWidth - radius * 2) * fraction;  
    float centerY = pos.y + 11.f;


    draw->AddRectFilled(ImVec2(pos.x, pos.y + 8), ImVec2(pos.x + sliderWidth, pos.y + 14), ImColor(30, 31, 35, 255), 10.0f);  


    ImU32 blue_col = ImGui::ColorConvertFloat4ToU32(YellowAccent);
    draw->AddRectFilled(ImVec2(pos.x, pos.y + 8), ImVec2(knobX, pos.y + 14), blue_col, 10.0f);
    

    draw->AddCircleFilled(ImVec2(knobX, centerY), 7.0f, ImColor(255, 255, 255, 255), 32);
    
    return changed;
}



bool AnimatedToggleSwitch(const char* label, bool* state) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    const ImGuiID id = window->GetID(label);
    ImVec2 label_size = ImGui::CalcTextSize(label);

    const float toggle_height = 35.0f;
    const float toggle_width  = 75.0f;
    const float rounding = toggle_height * 0.5f; // ✅ FULL ROUND

    float total_width = ImGui::GetContentRegionAvail().x - 15.0f;
    ImRect bb(window->DC.CursorPos,
        ImVec2(window->DC.CursorPos.x + total_width,
               window->DC.CursorPos.y + toggle_height + 10));

    ImGui::ItemSize(bb, g.Style.FramePadding.y);
    if (!ImGui::ItemAdd(bb, id)) return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
    if (pressed) *state = !*state;

    float current_pos = *state ? 1.0f : 0.0f;

    float time = (float)ImGui::GetTime() * 3.0f;
    float pulse = (sinf(time) * 0.2f) + 0.8f;

    ImVec2 t_min(bb.Max.x - toggle_width - 5.0f, bb.Min.y + 5);
    ImVec2 t_max(bb.Max.x - 5.0f, bb.Min.y + toggle_height + 5);

    // 🔵 BLUE GLOW (ONLY WHEN ON)
    if (current_pos > 0.01f) {
        for (int i = 1; i <= 3; i++) {
            float glow_strength = pulse * (1.0f - i * 0.15f);

            ImU32 glow_col = ImGui::ColorConvertFloat4ToU32(
                ImVec4(0.0f, 0.5f, 1.0f, glow_strength * 0.5f)
            );

            window->DrawList->AddRectFilled(
                ImVec2(t_min.x - i * 4, t_min.y - i * 4),
                ImVec2(t_max.x + i * 4, t_max.y + i * 4),
                glow_col,
                rounding + i * 2
            );
        }
    }

    // 🎨 BACKGROUND
    ImVec4 off_col = ImVec4(0.8f, 0.8f, 0.8f, 1.0f); // light gray
    ImVec4 on_col  = ImVec4(0.0f, 0.5f, 1.0f, 1.0f); // blue

    ImU32 bg_col = ImGui::ColorConvertFloat4ToU32(
        ImLerp(off_col, on_col, current_pos)
    );

    window->DrawList->AddRectFilled(t_min, t_max, bg_col, rounding);

    // Border
    window->DrawList->AddRect(
        t_min, t_max,
        IM_COL32(255, 255, 255, 120),
        rounding, 0, 1.5f
    );

    // ⚪ ROUND KNOB
    float pad = 4.0f;
    float knob_radius = (toggle_height * 0.5f) - pad;

    float knob_x = t_min.x + pad + (current_pos * (toggle_width - 2 * pad - knob_radius * 2));

    ImVec2 center = ImVec2(
        knob_x + knob_radius,
        t_min.y + toggle_height * 0.5f
    );

    window->DrawList->AddCircleFilled(
        center,
        knob_radius,
        IM_COL32(255, 255, 255, 255)
    );

    // 📝 LABEL
    window->DrawList->AddText(
        ImVec2(bb.Min.x, bb.Min.y + (toggle_height - label_size.y) * 0.5f + 5),
        IM_COL32(255, 255, 255, 255),
        label
    );

    return pressed;
}



// --- Left Nav Button (Tab Button) ---
static bool LeftNavButton(
    const char* label,
    const char* icon,
    bool selected,
    ImVec2 size = ImVec2(0, 65))
{
    // Auto Width
    if (size.x <= 0)
    {
        size.x = ImGui::GetContentRegionAvail().x;
    }

    ImDrawList* dl = ImGui::GetWindowDrawList();

    ImVec2 pos = ImGui::GetCursorScreenPos();

    ImRect bb(
        pos,
        ImVec2(pos.x + size.x, pos.y + size.y)
    );

    // Background Color
    ImU32 bg_col = selected
        ? IM_COL32(206, 6, 6, 220)
        : IM_COL32(25, 25, 28, 255);

    // Filled Background
    dl->AddRectFilled(
        bb.Min,
        bb.Max,
        bg_col,
        8.0f
    );

    // Border
    dl->AddRect(
        bb.Min,
        bb.Max,
        IM_COL32(255, 0, 0, 120),
        8.0f,
        ImDrawFlags_RoundCornersAll,
        2.0f
    );

    // Icon Position
    ImVec2 icon_pos(
        pos.x + 16,
        pos.y + 20
    );

    // Text Position
    ImVec2 text_pos(
        icon_pos.x + 32,
        pos.y + 20
    );

    // Draw Icon
    dl->AddText(
        icon_pos,
        IM_COL32(255,255,255,255),
        icon
    );

    // Draw Text
    dl->AddText(
        text_pos,
        IM_COL32(255,255,255,255),
        label
    );

    // Invisible Click Button
    ImGui::InvisibleButton(label, size);

    // Return Click State
    return ImGui::IsItemClicked();
}


uintptr_t il2cpp_base = 0;
void *getRealAddr(ulong offset) {
    return reinterpret_cast<void*>(il2cpp_base + offset);
};

namespace Settings
{
    static int Tab = 1;
}




bool clearMousePos = true;
bool initImGui = false;

inline EGLBoolean (*old_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
inline EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    eglQuerySurface(dpy, surface, EGL_WIDTH, &g_GlWidth);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &g_GlHeight);
    if (!g_IsSetup) {
        prevWidth = g_GlWidth;
        prevHeight = g_GlHeight;
        SetupImgui();
        
        g_IsSetup = true;
    }
    static bool isTouching = false;
    static float touchStartTime = 0.0f;
    SetNeonBlueTheme();
    ImGuiIO& io = ImGui::GetIO(); ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplAndroid_NewFrame(g_GlWidth, g_GlHeight);
	ImGui::NewFrame();
    int touchCount = ((int (*)())(Class_Input__get_touchCount))();
    if (touchCount > 0) {
        UnityEngine_Touch_Fields touch = ((UnityEngine_Touch_Fields(*)(int))(Class_Input__GetTouch))(0);
        float reverseY = io.DisplaySize.y - touch.m_Position.fields.y;
        io.MousePos = ImVec2(touch.m_Position.fields.x, reverseY);
        if (!isTouching) {
            touchStartTime = ImGui::GetTime();}
        io.MouseDown[0] = true;
        isTouching = true;}
    else {if (isTouching && (ImGui::GetTime() - touchStartTime < 0.06f)) {
        io.MouseDown[0] = true;} else {
        UpdateSpeedhack();
        io.MouseDown[0] = false;
        isTouching = false;
   }  
}

    DrawESP(g_GlWidth, g_GlHeight);
    
		// ================= TEXT SETUP =================
std::string topText = " PRABIR LIVE V7 SRC LEAK|| ";

ImVec2 topTextSize = ImGui::CalcTextSize(topText.c_str());
ImVec2 topTextPos(
    g_GlWidth / 2.0f - topTextSize.x / 2.0f,
    15.0f
);

ImDrawList* draw = ImGui::GetBackgroundDrawList();
float time = ImGui::GetTime();

// ================= BOX SETTINGS =================
float padX = 15.0f;
float padY = 8.0f;

ImVec2 boxMin(topTextPos.x - padX, topTextPos.y - padY);
ImVec2 boxMax(topTextPos.x + topTextSize.x + padX,
              topTextPos.y + topTextSize.y + padY);

float rounding = 10.0f;

// ================= PREMIUM BACKGROUND =================
draw->AddRectFilled(
    boxMin,
    boxMax,
    IM_COL32(15, 15, 15, 200),   // Dark glass-like background
    rounding
);

// ================= GLOW EFFECT (Outer Neon Glow) =================
for (int i = 0; i < 5; i++)
{
    draw->AddRect(
        ImVec2(boxMin.x - i, boxMin.y - i),
        ImVec2(boxMax.x + i, boxMax.y + i),
        ImColor(0, 150, 255, 50 - i * 8),  // Neon blue glow
        rounding + i,
        0,
        2.0f
    );
}

// ================= RAINBOW BORDER =================
draw->AddRect(
    boxMin,
    boxMax,
    ImColor::HSV(fmod(time * 0.5f, 1.0f), 1.0f, 1.0f),
    rounding,
    0,
    3.0f
);

// ================= RAINBOW ANIMATED TEXT =================
float xOffset = 0.0f;
float fastSpeed = 2.0f; // faster rainbow animation

for (size_t i = 0; i < topText.size(); i++)
{
    float hue = fmod((i / (float)topText.size()) + time * fastSpeed, 1.0f);
    ImColor color = ImColor::HSV(hue, 1.0f, 1.0f);

    std::string ch(1, topText[i]);
    ImVec2 charPos(topTextPos.x + xOffset, topTextPos.y);

    draw->AddText(charPos, color, ch.c_str());
    xOffset += ImGui::CalcTextSize(ch.c_str()).x;
}

// ================= OPTIONAL GLOSSY SHINE =================
draw->AddRectFilled(
    ImVec2(boxMin.x, boxMin.y),
    ImVec2(boxMax.x, boxMin.y + 6),
    IM_COL32(255, 255, 255, 40),
    rounding,
    ImDrawFlags_RoundCornersTop
);
	
//ImDrawList* draw = ImGui::GetBackgroundDrawList();

#define ICON_FA_CROSSHAIRS "\xef\x81\x9b"
#define ICON_FA_EYE "\xef\x81\xae"
#define ICON_FA_FIRE "\xef\x81\xad"
#define ICON_FA_USER_SECRET "\xef\x88\x9b"

ImGui::SetNextWindowSize(ImVec2(650, 450), ImGuiCond_Once);
ImGui::Begin(OBFUSCATE(ICON_FA_USER "PRABIR X LIVE V7 SRC LEAK"), 0,
    ImGuiWindowFlags_NoSavedSettings |
    ImGuiWindowFlags_NoScrollbar);

ImDrawList* dl = ImGui::GetWindowDrawList();
ImVec2 pos = ImGui::GetWindowPos();
ImVec2 size = ImGui::GetWindowSize();

static int tab = 0; // 0:AIMBOT, 1:VISUAL, 2:BRUTAL, 3:INFO

// ===== LEFT TAB =====
ImGui::BeginChild("LeftTabs", ImVec2(200, 0), true);
ImGui::Spacing();

if (LeftNavButton("  AIMBOT", ICON_FA_CROSSHAIRS, tab == 0))
    tab = 0;

ImGui::Spacing();

if (LeftNavButton("  VISUAL", ICON_FA_EYE, tab == 1))
    tab = 1;

ImGui::Spacing();

if (LeftNavButton("  BRUTAL", ICON_FA_FIRE, tab == 2))
    tab = 2;

ImGui::Spacing();

if (LeftNavButton("  INFO", ICON_FA_USER_SECRET, tab == 3))
    tab = 3;

ImGui::EndChild();

ImGui::SameLine();

// ===== RIGHT SIDE =====
ImGui::BeginChild("MainChild", ImVec2(0, 0), true);

ImGui::Spacing();

switch (tab)
{
case 0: // AIMBOT

    AnimatedToggleSwitch(" ACTIVE FUNCTION", &Enable);
    ImGui::Spacing();

    AnimatedToggleSwitch(" AIMBOT", &Aimbot);
    ImGui::Spacing();

    ImGui::SliderFloat(
        "AIM FOV",
        &Fov_Aim,
        0.0f,
        660.0f,
        "%.0f°",
        ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_NoInput
    );

    ImGui::Spacing();

    ImGui::SliderFloat(
        "Dist",
        &Aimdis,
        0.0f,
        500.0f,
        "%.0f",
        ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_NoInput
    );

    ImGui::Spacing();

   

    break;

case 1: // VISUAL

    AnimatedToggleSwitch(" ESP LINE", &Config.ESP.Line);
    AnimatedToggleSwitch(" ESP BOX", &Config.ESP.Box);
    AnimatedToggleSwitch(" ESP HEALTH", &Config.ESP.Health);
    AnimatedToggleSwitch(" ESP NAME", &Config.ESP.MAFIA);

    break;

case 2: // BRUTAL

    AnimatedToggleSwitch(" SPEED 50X", &SpeedHack);

    break;

case 3: // INFO

    ImGui::Text("FREE FIRE (2019115661)");

    ImGui::Dummy(ImVec2(0.0f, 5.0f));
    ImGui::Separator();

    ImGui::BulletText("USERNAME:");
    ImGui::SameLine();
    ImGui::TextColored(ImColor(4, 199, 10), "@PRABIRXLIVE1 ");

    ImGui::BulletText("SUBSCRIPTION:");
    ImGui::SameLine();
    ImGui::TextColored(ImColor(235, 152, 52), "LIFETIME");

    ImGui::BulletText("GAME VERSION:");
    ImGui::SameLine();
    ImGui::TextColored(ImColor(52, 143, 235), "2.115.1");

    ImGui::BulletText("DEVICE MODEL:");
    ImGui::SameLine();
    ImGui::TextColored(ImColor(199, 4, 108), "ANDROID");

    break;
}

ImGui::EndChild();
ImGui::End();





    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    return old_eglSwapBuffers(dpy, surface);
}


typedef unsigned long DWORD;
static uintptr_t libBase;

inline void StartGUI() {
    void *ptr_eglSwapBuffer = DobbySymbolResolver("/system/lib/libEGL.so", "eglSwapBuffers");
    if (ptr_eglSwapBuffer != nullptr) {
        DobbyHook((void *)ptr_eglSwapBuffer, (void *)hook_eglSwapBuffers, (void **)&old_eglSwapBuffers);
        LOGD("GUI started successfully");
    }
}

void hack_thread() {

    sleep(5);
    
    pid_t pid = getpid();
    LOGD("Hack Thread initiated for PID: %i", pid);


    do {
        il2cpp_base = get_module_base(pid, "libil2cpp.so");
        sleep(1);
    } while (il2cpp_base == 0);

    LOGD("libil2cpp.so found at address: 0x%" PRIxPTR, il2cpp_base);
    Il2CppAttach();

DobbyHook((void *)Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW.GamePlay"), OBFUSCATE("NewPlayerAnimationSystemComponent"), OBFUSCATE("PlayChangeWeaponAnim"), 2), (void*)hook_PlayChangeWeaponAnim, (void**)&orig_PlayChangeWeaponAnim);

DobbyHook((void*)Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW"), OBFUSCATE("GameFacade"), OBFUSCATE("ClientUsingVersion"), 0), (void *)hook_ClientVersion, (void **)&orig_ClientVersion);


DobbyHook(Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW.GamePlay"), OBFUSCATE("Player"), OBFUSCATE("get_InSwapWeaponCD"), 0), (void *)_get_InSwapWeaponCD, (void **) &get_InSwapWeaponCD);

DobbyHook((void *)Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW.GamePlay"), OBFUSCATE("Player"), OBFUSCATE("get_IsFastRunning"), 0), 
  (void *)hook_get_IsFastRunning, (void **)&orig_get_IsFastRunning);


DobbyHook((void *)Il2CppGetMethodOffset(OBFUSCATE("Assembly-CSharp.dll"), OBFUSCATE("COW.GamePlay"), OBFUSCATE("UGCLevelPointLight"), OBFUSCATE("get_Range"), 0), (void*)hook_get_Range, (void**)&orig_get_Range);




    StartGUI();
}


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

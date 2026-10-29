#include "fonts.h"
#include "fontello_data.h"
#include "fonts_data.h"
#include "font/font_data.hpp"
#include <cmath>
#include <vector>
#include <algorithm>

namespace
{
    enum class kind { inter, icon };

    struct font_entry
    {
        float   design = 0.f;
        bool    bold   = false;
        kind    type   = kind::inter;
        ImFont* font   = nullptr;
    };

    static std::vector<font_entry> g_entries;
    static const ImWchar icon_range[] = { 0xE000, 0xF8FF, 0 };

    static ImFontConfig base_cfg()
    {
        ImFontConfig cfg;
        cfg.FontDataOwnedByAtlas = false;
        cfg.PixelSnapH           = false;
        cfg.OversampleH          = 2;
        cfg.OversampleV          = 2;
        cfg.RasterizerMultiply   = 1.05f;
        return cfg;
    }

    static ImFontConfig icon_cfg()
    {
        ImFontConfig cfg = base_cfg();
        cfg.OversampleH = 2;
        cfg.OversampleV = 2;
        return cfg;
    }
}

namespace fonts
{
    void init()
    {
        ImGuiIO& io = ImGui::GetIO();

        g_entries = {
            { 11.f, false, kind::inter, nullptr },
            { 11.f, true,  kind::inter, nullptr },
            { 12.f, false, kind::inter, nullptr },
            { 12.f, true,  kind::inter, nullptr },
            { 13.f, false, kind::inter, nullptr },
            { 13.f, true,  kind::inter, nullptr },
            { 14.f, false, kind::inter, nullptr },
            { 14.f, true,  kind::inter, nullptr },
            { 15.f, false, kind::inter, nullptr },
            { 16.f, false, kind::inter, nullptr },
            { 18.f, false, kind::inter, nullptr },
            { 18.f, true,  kind::inter, nullptr },
            {  9.f, false, kind::icon,  nullptr },
            { 10.f, false, kind::icon,  nullptr },
            { 11.f, false, kind::icon,  nullptr },
            { 12.f, false, kind::icon,  nullptr },
            { 13.f, false, kind::icon,  nullptr },
            { 14.f, false, kind::icon,  nullptr },
            { 16.f, false, kind::icon,  nullptr },
            { 18.f, false, kind::icon,  nullptr },
        };

        const void* fontData = inter.empty() ? (const void*)font_ttf : (const void*)inter.data();
        const int fontDataSize = inter.empty() ? (int)sizeof(font_ttf) : (int)inter.size();

        for (auto& e : g_entries)
        {
            const float px = roundf(e.design);
            ImFontConfig cfg = (e.type == kind::icon) ? icon_cfg() : base_cfg();

            if (e.type == kind::icon)
            {
                e.font = io.Fonts->AddFontFromMemoryTTF(
                    (void*)fontello_data::rawData, (int)fontello_data::ttf_size,
                    px, &cfg, icon_range);
            }
            else
            {
                e.font = io.Fonts->AddFontFromMemoryTTF(
                    (void*)fontData, fontDataSize,
                    px, &cfg, io.Fonts->GetGlyphRangesDefault());
            }
        }

        for (const auto& e : g_entries)
        {
            if (e.type == kind::inter && !e.bold && e.design == 14.f && e.font)
            {
                io.FontDefault = e.font;
                break;
            }
        }
    }

    void shutdown()
    {
        g_entries.clear();
    }

    float size(ImFont* font)
    {
        return font ? font->FontSize : 14.f;
    }

    ImFont* inter(float design_size, bool bold)
    {
        ImFont* best = nullptr;
        float bestDiff = 9999.f;

        for (const auto& e : g_entries)
        {
            if (e.type != kind::inter) continue;
            if (e.bold != bold) continue;
            float diff = std::fabs(e.design - design_size);
            if (diff < bestDiff && e.font != nullptr)
            {
                bestDiff = diff;
                best = e.font;
            }
        }
        if (best) return best;
        return ImGui::GetFont();
    }

    ImFont* icon(float design_size)
    {
        ImFont* best = nullptr;
        float bestDiff = 9999.f;

        for (const auto& e : g_entries)
        {
            if (e.type != kind::icon) continue;
            float diff = std::fabs(e.design - design_size);
            if (diff < bestDiff && e.font != nullptr)
            {
                bestDiff = diff;
                best = e.font;
            }
        }
        if (best) return best;
        return ImGui::GetFont();
    }
}


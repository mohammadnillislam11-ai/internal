#define IMGUI_DEFINE_MATH_OPERATORS
#include "menu.hpp"
#include "bar.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "theme/colors.h"
#include "theme/layout.h"
#include "fonts/fonts.h"
#include "cfg.hpp"
#include "game/AimSilent.h"
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include <algorithm>

namespace icons
{
    inline constexpr const char* exit_ico    = "\xEE\xAD\x8C"; // 0xeb4c
    inline constexpr const char* folder      = "\xEE\xAD\x84"; // 0xeb44
    inline constexpr const char* warning     = "\xEE\xAC\xBE"; // 0xeb3e
    inline constexpr const char* keyboard    = "\xEE\xA9\x91"; // 0xea51
    inline constexpr const char* menu        = "\xEE\xA8\xA8"; // 0xea28
    inline constexpr const char* search      = "\xE2\x9C\x95"; // 0x2715
    inline constexpr const char* trash       = "\xEF\x87\xB8"; // 0xf1f8
    inline constexpr const char* home        = "\xEE\xA8\xB8"; // 0xea38
    inline constexpr const char* user        = "\xEE\xAC\x8F"; // 0xeb0f
    inline constexpr const char* color       = "\xEE\xAE\x99"; // 0xeb99
    inline constexpr const char* color_pick  = "\xEF\x87\xBB"; // 0xf1fb
    inline constexpr const char* art_gallery = "\xEE\x81\x84"; // 0xe044
    inline constexpr const char* eye         = "\xEE\xA8\xBD"; // 0xea3d
    inline constexpr const char* pin         = "\xEE\xA9\x99"; // 0xea59
    inline constexpr const char* bug         = "\xEF\x86\x88"; // 0xf188
    inline constexpr const char* download    = "\xEE\xA9\x84"; // 0xea44
    inline constexpr const char* mouse       = "\xEE\xAB\x91"; // 0xead1
    inline constexpr const char* reload      = "\xEE\xAD\xAD"; // 0xeb6d
    inline constexpr const char* bell        = "\xEF\x83\xB3"; // 0xf0f3
    inline constexpr const char* settings    = "\xEE\xA9\xAC"; // 0xea6c
    inline constexpr const char* sliders     = "\xEF\x87\x9E"; // 0xf1de
    inline constexpr const char* chevron_r   = "\xEE\xAA\x8C"; // 0xea8c
    inline constexpr const char* chevron_d   = "\xEE\xAA\x8A"; // 0xea8a
    inline constexpr const char* youtube     = "\xEF\x85\xAA"; // 0xf16a
}

namespace
{
    enum class tab { main, visuals, aim, settings };

    struct tab_def
    {
        tab         id;
        const char* label;
        const char* icon;
    };

    static const tab_def g_tabs[] = {
        { tab::main,     "Main",     icons::home     },
        { tab::visuals,  "Visuals",  icons::eye      },
        { tab::aim,      "Aim",      icons::mouse    },
        { tab::settings, "Settings", icons::settings },
    };
    static const int g_tab_count = 4;

    struct app_state
    {
        tab   active = tab::main;
        bool  exit_pressed = false;
        float content_scroll = 0.f;
        int   target_bone_idx = 0;
        int   target_prio_idx = 0;
        int   box_style_idx = 0;
        int   accent_color_idx = 0;
    };

    static app_state g_state;

    struct open_dd_t
    {
        ImGuiID            id = 0;
        ImRect             closed_bb;
        int*               index = nullptr;
        const char* const* opts = nullptr;
        int                count = 0;
    };
    static open_dd_t g_open_dd;

    static bool   g_dragging = false;
    static ImVec2 g_drag_start = {};
    static ImVec2 g_win_pos(100.f, 80.f);
    static bool   g_pos_initialized = false;

    static void draw_section_pattern(ImDrawList* dl, const ImRect& body)
    {
        if (body.GetHeight() < 20.f || body.GetWidth() < 30.f) return;

        dl->PushClipRect(body.Min, body.Max, true);
        const ImU32 col = ImGui::GetColorU32(colors::pattern_circle);
        const ImVec2 c = { body.Max.x - 4.f, body.Max.y - 4.f };

        dl->AddCircle(c, 26.f, col, 48, 1.3f);
        dl->AddCircle(c, 52.f, col, 64, 1.3f);
        dl->AddCircle(c, 80.f, col, 80, 1.3f);
        dl->PopClipRect();
    }

    static void draw_row_separator(ImDrawList* dl, const ImRect& row)
    {
        const float pad = layout::section_pad_x;
        dl->AddLine(
            { row.Min.x + pad, row.Max.y - 0.5f },
            { row.Max.x - pad, row.Max.y - 0.5f },
            ImGui::GetColorU32(colors::row_sep), 1.f);
    }

    static void draw_checkbox(ImDrawList* dl, const char* label, bool* value, const ImRect& row, bool warn = false)
    {
        ImGui::PushID(label);
        ImGuiID gid = ImGui::GetID("##cb");

        const float pad = layout::section_pad_x;
        const float cb_sz = layout::checkbox_size;
        const ImRect cb_bb(
            { row.Max.x - pad - cb_sz, row.GetCenter().y - cb_sz * 0.5f },
            { row.Max.x - pad,          row.GetCenter().y + cb_sz * 0.5f });

        ImGui::ItemAdd(row, gid);
        bool hovered = false, held = false;
        bool pressed = ImGui::ButtonBehavior(row, gid, &hovered, &held);

        // Mobile touch friendly fallback
        if (pressed || (hovered && ImGui::IsMouseReleased(0) && !ImGui::IsMouseDragging(0, 8.f)))
        {
            *value = !*value;
        }

        static float anims[256] = {};
        const int slot = (int)(gid % 256);
        anims[slot] = ImLerp(anims[slot], *value ? 1.f : 0.f,
            ImMin(1.f, ImGui::GetIO().DeltaTime * 14.f));

        const ImVec4 fill = ImLerp(colors::checkbox_off, colors::accent, anims[slot]);
        dl->AddRectFilled(cb_bb.Min, cb_bb.Max,
            ImGui::GetColorU32(fill), layout::checkbox_round);

        if (anims[slot] > 0.05f)
        {
            const float mark = cb_sz * 0.55f;
            ImGui::RenderCheckMark(dl,
                { cb_bb.Min.x + (cb_sz - mark) * 0.5f, cb_bb.Min.y + (cb_sz - mark) * 0.5f },
                ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, anims[slot])), mark);
        }

        float tx = row.Min.x + pad;

        if (warn)
        {
            if (ImFont* wf = fonts::icon(12.f))
            {
                const float ws = fonts::size(wf);
                dl->AddText(wf, ws,
                    { tx, row.GetCenter().y - ws * 0.5f },
                    ImGui::GetColorU32(colors::warning), icons::warning);
                tx += ws + 6.f;
            }
        }

        if (ImFont* font = fonts::inter(13.f))
        {
            const float sz = fonts::size(font);
            const ImVec4 col = *value ? colors::text : colors::text_label;
            dl->AddText(font, sz,
                { tx, row.GetCenter().y - sz * 0.5f },
                ImGui::GetColorU32(col), label);
        }

        ImGui::PopID();
    }

    static void draw_slider(ImDrawList* dl, const char* label, float* value, float min_v, float max_v,
                            const ImRect& row, const char* format = "%.2f")
    {
        ImGui::PushID(label);
        ImGuiID gid = ImGui::GetID("##sl");

        const float pad = layout::section_pad_x;
        const float track_h = layout::slider_track_h;
        const float dot_r = layout::slider_dot_r;

        float ty = row.Min.y + 7.f;
        if (ImFont* font = fonts::inter(13.f))
        {
            const float sz = fonts::size(font);
            dl->AddText(font, sz, { row.Min.x + pad, ty },
                ImGui::GetColorU32(colors::text_label), label);
        }

        char val_buf[32];
        snprintf(val_buf, sizeof(val_buf), format, *value);
        if (ImFont* bf = fonts::inter(12.f, true))
        {
            const float bsz = fonts::size(bf);
            const ImVec2 ts = bf->CalcTextSizeA(bsz, FLT_MAX, 0.f, val_buf);
            dl->AddText(bf, bsz, { row.Max.x - pad - ts.x, ty },
                ImGui::GetColorU32(colors::text), val_buf);
        }

        const float track_y0 = row.Max.y - 10.f - track_h;
        const ImRect track_bb(
            { row.Min.x + pad, track_y0 },
            { row.Max.x - pad, track_y0 + track_h });

        ImGui::ItemAdd(track_bb, gid);
        bool hovered = false, held = false;
        ImGui::ButtonBehavior(track_bb, gid, &hovered, &held);

        if (held)
        {
            const float mouse_x = ImGui::GetIO().MousePos.x;
            const float norm = ImClamp((mouse_x - track_bb.Min.x) / track_bb.GetWidth(), 0.f, 1.f);
            *value = min_v + norm * (max_v - min_v);
        }

        const float norm = ImClamp((*value - min_v) / (max_v - min_v), 0.f, 1.f);
        const float r = track_h * 0.5f;

        dl->AddRectFilled(track_bb.Min, track_bb.Max,
            ImGui::GetColorU32(colors::slider_track), r);

        if (norm > 0.005f)
        {
            const float fill_x = track_bb.Min.x + norm * track_bb.GetWidth();
            dl->AddRectFilled(track_bb.Min, { fill_x, track_bb.Max.y },
                ImGui::GetColorU32(colors::accent), r);

            const ImVec2 dot_c = { fill_x - r, track_bb.GetCenter().y };
            dl->AddCircleFilled(dot_c, dot_r, IM_COL32(255, 255, 255, 255), 24);
        }

        ImGui::PopID();
    }

    static void draw_dropdown(ImDrawList* dl, const char* label, int* current_idx,
                              const char* const* options, int option_count, const ImRect& row)
    {
        ImGui::PushID(label);
        ImGuiID gid = ImGui::GetID("##dd");

        const float pad = layout::section_pad_x;
        const float ddh = layout::dropdown_h;

        float ty = row.Min.y + 6.f;
        if (ImFont* font = fonts::inter(13.f))
        {
            const float sz = fonts::size(font);
            dl->AddText(font, sz, { row.Min.x + pad, ty },
                ImGui::GetColorU32(colors::text_label), label);
        }

        const ImRect dd_bb(
            { row.Min.x + pad, row.Max.y - 8.f - ddh },
            { row.Max.x - pad, row.Max.y - 8.f });

        ImGui::ItemAdd(dd_bb, gid);
        bool hovered = false, held = false;
        if (ImGui::ButtonBehavior(dd_bb, gid, &hovered, &held))
        {
            if (g_open_dd.id == gid)
            {
                g_open_dd.id = 0;
            }
            else
            {
                g_open_dd.id = gid;
                g_open_dd.closed_bb = dd_bb;
                g_open_dd.index = current_idx;
                g_open_dd.opts = options;
                g_open_dd.count = option_count;
            }
        }

        dl->AddRectFilled(dd_bb.Min, dd_bb.Max,
            ImGui::GetColorU32(colors::dropdown_bg), layout::dropdown_round);

        const char* sel_text = (*current_idx >= 0 && *current_idx < option_count) ? options[*current_idx] : "None";
        if (ImFont* font = fonts::inter(13.f))
        {
            const float sz = fonts::size(font);
            dl->AddText(font, sz,
                { dd_bb.Min.x + 8.f, dd_bb.GetCenter().y - sz * 0.5f },
                ImGui::GetColorU32(colors::text), sel_text);
        }

        if (ImFont* icf = fonts::icon(10.f))
        {
            const float isz = fonts::size(icf);
            const ImVec2 its = icf->CalcTextSizeA(isz, FLT_MAX, 0.f, icons::chevron_d);
            dl->AddText(icf, isz,
                { dd_bb.Max.x - 8.f - its.x, dd_bb.GetCenter().y - its.y * 0.5f },
                ImGui::GetColorU32(colors::text_secondary), icons::chevron_d);
        }

        ImGui::PopID();
    }

    static void draw_open_dropdown(ImDrawList* dl, const ImRect& viewport)
    {
        if (g_open_dd.id == 0 || !g_open_dd.index || !g_open_dd.opts) return;

        const ImRect& closed = g_open_dd.closed_bb;
        const int count = g_open_dd.count;
        const float item_h = 28.f;
        const float pad_y = 4.f;
        const float popup_h = pad_y * 2.f + count * item_h;

        const ImRect popup(
            { closed.Min.x, closed.Max.y + 3.f },
            { closed.Max.x, closed.Max.y + 3.f + popup_h });

        dl->AddRectFilled(popup.Min, popup.Max,
            ImGui::GetColorU32(colors::dropdown_pop_bg), layout::dropdown_round);
        dl->AddRect(popup.Min, popup.Max,
            ImGui::GetColorU32(colors::accent_dim), layout::dropdown_round);

        const ImVec2 mp = ImGui::GetIO().MousePos;
        bool clicked_outside = ImGui::IsMouseClicked(0)
            && !popup.Contains(mp) && !closed.Contains(mp);

        for (int i = 0; i < count; ++i)
        {
            const ImRect item(
                { popup.Min.x + 4.f, popup.Min.y + pad_y + i * item_h },
                { popup.Max.x - 4.f, popup.Min.y + pad_y + (i + 1) * item_h });

            const bool hovered = item.Contains(mp);
            const bool is_sel = (*g_open_dd.index == i);

            if (hovered)
                dl->AddRectFilled(item.Min, item.Max,
                    ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, 0.08f)), 3.f);

            if (is_sel)
                dl->AddRectFilled(item.Min, item.Max,
                    ImGui::GetColorU32(colors::accent_dim), 3.f);

            if (ImFont* tf = fonts::inter(13.f, is_sel))
            {
                const float tsz = fonts::size(tf);
                dl->AddText(tf, tsz,
                    { item.Min.x + 10.f, item.GetCenter().y - tsz * 0.5f },
                    ImGui::GetColorU32(is_sel ? colors::text : colors::text_secondary),
                    g_open_dd.opts[i]);
            }

            if (hovered && ImGui::IsMouseReleased(0))
            {
                *g_open_dd.index = i;
                g_open_dd.id = 0;
                return;
            }
        }

        if (clicked_outside)
            g_open_dd.id = 0;
    }

    static void draw_sidebar(ImDrawList* dl, const ImRect& sb)
    {
        dl->AddRectFilled(sb.Min, sb.Max,
            ImGui::GetColorU32(colors::bg), layout::window_round,
            ImDrawFlags_RoundCornersLeft);

        const float cx = sb.GetCenter().x;
        const float btn_sz = layout::tab_btn_size;
        const float start_y = sb.Min.y + layout::header_pad_y + 2.f;
        const float dt = ImGui::GetIO().DeltaTime;

        static float sel_anim[g_tab_count] = {};
        static float hover_anim[g_tab_count] = {};

        for (int i = 0; i < g_tab_count; ++i)
        {
            const float ty = start_y + i * (btn_sz + layout::tab_gap);
            const ImRect tab_bb(
                { cx - btn_sz * 0.5f, ty },
                { cx + btn_sz * 0.5f, ty + btn_sz });

            const bool selected = (g_state.active == g_tabs[i].id);

            ImGui::PushID(i + 100);
            ImGuiID gid = ImGui::GetID("##st");
            ImGui::ItemAdd(tab_bb, gid);

            bool hovered = false, held = false;
            if (ImGui::ButtonBehavior(tab_bb, gid, &hovered, &held))
                g_state.active = g_tabs[i].id;

            sel_anim[i]   = ImLerp(sel_anim[i],   selected ? 1.f : 0.f, ImMin(1.f, dt * 14.f));
            hover_anim[i] = ImLerp(hover_anim[i], (!selected && (hovered || held)) ? 1.f : 0.f, ImMin(1.f, dt * 16.f));

            if (sel_anim[i] > 0.01f)
            {
                dl->AddRectFilled(tab_bb.Min, tab_bb.Max,
                    ImGui::GetColorU32(ImVec4(0.f, 0.f, 0.f, 0.35f * sel_anim[i])),
                    layout::tab_round);
                dl->AddRectFilled(tab_bb.Min, tab_bb.Max,
                    ImGui::GetColorU32(ImVec4(
                        colors::accent.x, colors::accent.y, colors::accent.z,
                        0.40f * sel_anim[i])),
                    layout::tab_round);
            }

            if (hover_anim[i] > 0.01f)
            {
                dl->AddRectFilled(tab_bb.Min, tab_bb.Max,
                    ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, 0.08f * hover_anim[i])),
                    layout::tab_round);
            }

            if (ImFont* icf = fonts::icon(16.f))
            {
                const float isz = fonts::size(icf);
                const ImVec2 its = icf->CalcTextSizeA(isz, FLT_MAX, 0.f, g_tabs[i].icon);
                const ImVec4 col = ImLerp(
                    ImLerp(colors::icon_tab_off, colors::text_secondary, hover_anim[i]),
                    colors::icon_tab_on, sel_anim[i]);
                dl->AddText(icf, isz,
                    { cx - its.x * 0.5f, tab_bb.GetCenter().y - its.y * 0.5f },
                    ImGui::GetColorU32(col), g_tabs[i].icon);
            }

            ImGui::PopID();
        }

        // Bottom social icons
        const char* socials[] = { icons::user, icons::youtube, icons::pin };
        const int social_count = 3;
        const float social_h = 28.f;
        const float social_start = sb.Max.y - 12.f - social_count * social_h;

        for (int i = 0; i < social_count; ++i)
        {
            const float sy = social_start + i * social_h;
            const ImRect sbb({ cx - 14.f, sy }, { cx + 14.f, sy + social_h - 4.f });

            if (ImFont* icf = fonts::icon(13.f))
            {
                const float isz = fonts::size(icf);
                const ImVec2 its = icf->CalcTextSizeA(isz, FLT_MAX, 0.f, socials[i]);
                dl->AddText(icf, isz,
                    { cx - its.x * 0.5f, sbb.GetCenter().y - its.y * 0.5f },
                    ImGui::GetColorU32(colors::social), socials[i]);
            }
        }
    }

    static void draw_header(ImDrawList* dl, const ImRect& ha)
    {
        const float px = layout::header_pad_x;
        const float py = layout::header_pad_y;

        const char* tab_name = "Main";
        for (int i = 0; i < g_tab_count; ++i)
            if (g_tabs[i].id == g_state.active)
                tab_name = g_tabs[i].label;

        const float title_y = ha.Min.y + py;
        if (ImFont* big_f = fonts::inter(18.f, true))
        {
            const float sz = fonts::size(big_f);
            dl->AddText(big_f, sz,
                { ha.Min.x + px, title_y },
                ImGui::GetColorU32(colors::text), tab_name);
        }

        const float bc_y = title_y + 24.f;
        if (ImFont* bc_f = fonts::inter(11.f))
        {
            const float sz = fonts::size(bc_f);
            float bx = ha.Min.x + px;

            dl->AddText(bc_f, sz, { bx, bc_y },
                ImGui::GetColorU32(colors::text_muted), "NINMODS");
            const ImVec2 ats = bc_f->CalcTextSizeA(sz, FLT_MAX, 0.f, "NINMODS");
            bx += ats.x + 6.f;

            if (ImFont* icf = fonts::icon(9.f))
            {
                const float icsz = fonts::size(icf);
                const ImVec2 its = icf->CalcTextSizeA(icsz, FLT_MAX, 0.f, icons::chevron_r);
                dl->AddText(icf, icsz, { bx, bc_y + (sz - icsz) * 0.5f + 1.f },
                    ImGui::GetColorU32(colors::text_muted), icons::chevron_r);
                bx += its.x + 6.f;
            }

            dl->AddText(bc_f, sz, { bx, bc_y },
                ImGui::GetColorU32(colors::text_secondary), tab_name);
        }

        // Exit button
        const float btn_sz = layout::exit_btn_sz;
        const float center_y = ha.GetCenter().y;
        const ImRect exit_bb(
            { ha.Max.x - px - btn_sz, center_y - btn_sz * 0.5f },
            { ha.Max.x - px,          center_y + btn_sz * 0.5f });

        ImGui::PushID("##exit_btn");
        ImGuiID eid = ImGui::GetID("##ex");
        ImGui::ItemAdd(exit_bb, eid);
        bool eh = false, eheld = false;
        if (ImGui::ButtonBehavior(exit_bb, eid, &eh, &eheld) ||
            (eh && ImGui::IsMouseReleased(0)))
        {
            g_state.exit_pressed = true;
        }

        const ImVec4 bg = (eh || eheld) ? colors::exit_bg_hover : colors::exit_bg;
        dl->AddRectFilled(exit_bb.Min, exit_bb.Max,
            ImGui::GetColorU32(bg), layout::exit_btn_round);

        if (ImFont* icf = fonts::icon(14.f))
        {
            const float isz = fonts::size(icf);
            const ImVec2 its = icf->CalcTextSizeA(isz, FLT_MAX, 0.f, icons::exit_ico);
            dl->AddText(icf, isz,
                { exit_bb.GetCenter().x - its.x * 0.5f, exit_bb.GetCenter().y - its.y * 0.5f },
                ImGui::GetColorU32(colors::exit_icon), icons::exit_ico);
        }
        ImGui::PopID();
    }

    static void draw_tab_visuals(ImDrawList* dl, const ImRect& content_bb)
    {
        const float pad_x = layout::content_pad_x;
        const float pad_y = layout::content_pad_y;
        const float gap = layout::section_gap;

        const float avail_w = content_bb.GetWidth() - pad_x * 2.f;
        const float col_w = (avail_w - gap) * 0.5f;

        // Column 1: ESP Elements
        const ImRect c1_bb(
            { content_bb.Min.x + pad_x, content_bb.Min.y + pad_y },
            { content_bb.Min.x + pad_x + col_w, content_bb.Min.y + pad_y + 220.f });

        dl->AddRectFilled(c1_bb.Min, c1_bb.Max,
            ImGui::GetColorU32(colors::section_box), layout::section_round);
        draw_section_pattern(dl, c1_bb);

        const ImRect c1_hbb(c1_bb.Min, { c1_bb.Max.x, c1_bb.Min.y + layout::section_header_h });
        dl->AddRectFilled(c1_hbb.Min, c1_hbb.Max,
            ImGui::GetColorU32(colors::section_header), layout::section_round,
            ImDrawFlags_RoundCornersTop);

        if (ImFont* hf = fonts::inter(13.f, true))
        {
            const float sz = fonts::size(hf);
            dl->AddText(hf, sz,
                { c1_hbb.Min.x + layout::section_pad_x, c1_hbb.GetCenter().y - sz * 0.5f },
                ImGui::GetColorU32(colors::text_header), "ESP Elements");
        }

        float cur_y = c1_hbb.Max.y;
        const float rh = layout::section_row_h;

        // Row 1: Enable ESP (Master)
        ImRect r1({ c1_bb.Min.x, cur_y }, { c1_bb.Max.x, cur_y + rh });
        draw_checkbox(dl, "Enable ESP (Master)", &cfg::esp::enabled, r1, true);
        draw_row_separator(dl, r1);
        cur_y += rh;

        // Row 2: ESP Line
        ImRect r2({ c1_bb.Min.x, cur_y }, { c1_bb.Max.x, cur_y + rh });
        draw_checkbox(dl, "ESP Line (Thin)", &cfg::esp::tracer, r2);
        draw_row_separator(dl, r2);
        cur_y += rh;

        // Row 3: ESP Box
        ImRect r3({ c1_bb.Min.x, cur_y }, { c1_bb.Max.x, cur_y + rh });
        draw_checkbox(dl, "ESP Box", &cfg::esp::box, r3);
        draw_row_separator(dl, r3);
        cur_y += rh;

        // Row 4: ESP Skeleton
        ImRect r4({ c1_bb.Min.x, cur_y }, { c1_bb.Max.x, cur_y + rh });
        draw_checkbox(dl, "ESP Skeleton", &cfg::esp::skeleton, r4);

        // Column 2: Player Info & Range
        const ImRect c2_bb(
            { c1_bb.Max.x + gap, content_bb.Min.y + pad_y },
            { content_bb.Max.x - pad_x, content_bb.Min.y + pad_y + 276.f });

        dl->AddRectFilled(c2_bb.Min, c2_bb.Max,
            ImGui::GetColorU32(colors::section_box), layout::section_round);
        draw_section_pattern(dl, c2_bb);

        const ImRect c2_hbb(c2_bb.Min, { c2_bb.Max.x, c2_bb.Min.y + layout::section_header_h });
        dl->AddRectFilled(c2_hbb.Min, c2_hbb.Max,
            ImGui::GetColorU32(colors::section_header), layout::section_round,
            ImDrawFlags_RoundCornersTop);

        if (ImFont* hf = fonts::inter(13.f, true))
        {
            const float sz = fonts::size(hf);
            dl->AddText(hf, sz,
                { c2_hbb.Min.x + layout::section_pad_x, c2_hbb.GetCenter().y - sz * 0.5f },
                ImGui::GetColorU32(colors::text_header), "Player Information");
        }

        cur_y = c2_hbb.Max.y;

        // Row 1: ESP Health
        ImRect i1({ c2_bb.Min.x, cur_y }, { c2_bb.Max.x, cur_y + rh });
        draw_checkbox(dl, "ESP Health Bar", &cfg::esp::health, i1);
        draw_row_separator(dl, i1);
        cur_y += rh;

        // Row 2: ESP Name
        ImRect i2({ c2_bb.Min.x, cur_y }, { c2_bb.Max.x, cur_y + rh });
        draw_checkbox(dl, "ESP Name", &cfg::esp::name, i2);
        draw_row_separator(dl, i2);
        cur_y += rh;

        // Row 3: ESP Distance
        ImRect i3({ c2_bb.Min.x, cur_y }, { c2_bb.Max.x, cur_y + rh });
        draw_checkbox(dl, "ESP Distance", &cfg::esp::distance, i3);
        draw_row_separator(dl, i3);
        cur_y += rh;

        // Row 4: Crosshair
        ImRect i4({ c2_bb.Min.x, cur_y }, { c2_bb.Max.x, cur_y + rh });
        draw_checkbox(dl, "Screen Crosshair", &cfg::esp::crosshair, i4);
        draw_row_separator(dl, i4);
        cur_y += rh;

        // Row 5: Max Distance Slider
        const float s_rh = layout::section_slider_row_h;
        ImRect i5({ c2_bb.Min.x, cur_y }, { c2_bb.Max.x, cur_y + s_rh });
        draw_slider(dl, "Render Range", &cfg::esp::max_range, 50.f, 500.f, i5, "%.0fm");
    }

    static void draw_tab_aim(ImDrawList* dl, const ImRect& content_bb)
    {
        const float pad_x = layout::content_pad_x;
        const float pad_y = layout::content_pad_y;
        const float gap = layout::section_gap;

        const float avail_w = content_bb.GetWidth() - pad_x * 2.f;
        const float col_w = (avail_w - gap) * 0.5f;

        // Column 1: Silent Aim Switches
        const ImRect c1_bb(
            { content_bb.Min.x + pad_x, content_bb.Min.y + pad_y },
            { content_bb.Min.x + pad_x + col_w, content_bb.Min.y + pad_y + 160.f });

        dl->AddRectFilled(c1_bb.Min, c1_bb.Max,
            ImGui::GetColorU32(colors::section_box), layout::section_round);
        draw_section_pattern(dl, c1_bb);

        const ImRect c1_hbb(c1_bb.Min, { c1_bb.Max.x, c1_bb.Min.y + layout::section_header_h });
        dl->AddRectFilled(c1_hbb.Min, c1_hbb.Max,
            ImGui::GetColorU32(colors::section_header), layout::section_round,
            ImDrawFlags_RoundCornersTop);

        if (ImFont* hf = fonts::inter(13.f, true))
        {
            const float sz = fonts::size(hf);
            dl->AddText(hf, sz,
                { c1_hbb.Min.x + layout::section_pad_x, c1_hbb.GetCenter().y - sz * 0.5f },
                ImGui::GetColorU32(colors::text_header), "Silent Aim");
        }

        float cur_y = c1_hbb.Max.y;
        const float rh = layout::section_row_h;

        // Row 1: Enable Silent Aim
        ImRect r1({ c1_bb.Min.x, cur_y }, { c1_bb.Max.x, cur_y + rh });
        draw_checkbox(dl, "Enable Silent Aim", &AimSilentEnabled, r1, true);
        draw_row_separator(dl, r1);
        cur_y += rh;

        // Row 2: Nobita Mode
        ImRect r2({ c1_bb.Min.x, cur_y }, { c1_bb.Max.x, cur_y + rh });
        draw_checkbox(dl, "Nobita Mode", &AimSilentNobita, r2);

        // Column 2: Target Configuration
        const ImRect c2_bb(
            { c1_bb.Max.x + gap, content_bb.Min.y + pad_y },
            { content_bb.Max.x - pad_x, content_bb.Min.y + pad_y + 180.f });

        dl->AddRectFilled(c2_bb.Min, c2_bb.Max,
            ImGui::GetColorU32(colors::section_box), layout::section_round);
        draw_section_pattern(dl, c2_bb);

        const ImRect c2_hbb(c2_bb.Min, { c2_bb.Max.x, c2_bb.Min.y + layout::section_header_h });
        dl->AddRectFilled(c2_hbb.Min, c2_hbb.Max,
            ImGui::GetColorU32(colors::section_header), layout::section_round,
            ImDrawFlags_RoundCornersTop);

        if (ImFont* hf = fonts::inter(13.f, true))
        {
            const float sz = fonts::size(hf);
            dl->AddText(hf, sz,
                { c2_hbb.Min.x + layout::section_pad_x, c2_hbb.GetCenter().y - sz * 0.5f },
                ImGui::GetColorU32(colors::text_header), "Target Configuration");
        }

        cur_y = c2_hbb.Max.y;
        const float drh = layout::section_dropdown_row_h;

        static const char* bone_opts[] = { "Head", "Neck", "Chest", "Pelvis" };
        ImRect d1({ c2_bb.Min.x, cur_y }, { c2_bb.Max.x, cur_y + drh });
        draw_dropdown(dl, "Target Bone", &g_state.target_bone_idx, bone_opts, 4, d1);
        draw_row_separator(dl, d1);
        cur_y += drh;

        static const char* prio_opts[] = { "Closest Distance", "Lowest HP", "Crosshair FOV" };
        ImRect d2({ c2_bb.Min.x, cur_y }, { c2_bb.Max.x, cur_y + drh });
        draw_dropdown(dl, "Priority Target", &g_state.target_prio_idx, prio_opts, 3, d2);
    }

    static void draw_tab_main(ImDrawList* dl, const ImRect& content_bb)
    {
        const float pad_x = layout::content_pad_x;
        const float pad_y = layout::content_pad_y;
        const float gap = layout::section_gap;

        const float avail_w = content_bb.GetWidth() - pad_x * 2.f;
        const float col_w = (avail_w - gap) * 0.5f;

        // Card 1: System Info
        const ImRect c1_bb(
            { content_bb.Min.x + pad_x, content_bb.Min.y + pad_y },
            { content_bb.Min.x + pad_x + col_w, content_bb.Min.y + pad_y + 190.f });

        dl->AddRectFilled(c1_bb.Min, c1_bb.Max,
            ImGui::GetColorU32(colors::section_box), layout::section_round);
        draw_section_pattern(dl, c1_bb);

        const ImRect c1_hbb(c1_bb.Min, { c1_bb.Max.x, c1_bb.Min.y + layout::section_header_h });
        dl->AddRectFilled(c1_hbb.Min, c1_hbb.Max,
            ImGui::GetColorU32(colors::section_header), layout::section_round,
            ImDrawFlags_RoundCornersTop);

        if (ImFont* hf = fonts::inter(13.f, true))
        {
            const float sz = fonts::size(hf);
            dl->AddText(hf, sz,
                { c1_hbb.Min.x + layout::section_pad_x, c1_hbb.GetCenter().y - sz * 0.5f },
                ImGui::GetColorU32(colors::text_header), "Engine Dashboard");
        }

        float cur_y = c1_hbb.Max.y + 10.f;
        if (ImFont* f = fonts::inter(13.f))
        {
            const float sz = fonts::size(f);
            float tx = c1_bb.Min.x + layout::section_pad_x;

            dl->AddText(f, sz, { tx, cur_y }, ImGui::GetColorU32(colors::text_label), "Engine Status:");
            dl->AddText(f, sz, { c1_bb.Max.x - layout::section_pad_x - 50.f, cur_y },
                ImGui::GetColorU32(colors::accent), "ONLINE");
            cur_y += 28.f;

            char fps_buf[32];
            snprintf(fps_buf, sizeof(fps_buf), "%.1f FPS", ImGui::GetIO().Framerate);
            dl->AddText(f, sz, { tx, cur_y }, ImGui::GetColorU32(colors::text_label), "Render Rate:");
            dl->AddText(f, sz, { c1_bb.Max.x - layout::section_pad_x - 65.f, cur_y },
                ImGui::GetColorU32(colors::text), fps_buf);
            cur_y += 28.f;

            dl->AddText(f, sz, { tx, cur_y }, ImGui::GetColorU32(colors::text_label), "Client Edition:");
            dl->AddText(f, sz, { c1_bb.Max.x - layout::section_pad_x - 90.f, cur_y },
                ImGui::GetColorU32(colors::text_secondary), "Portfolio #16");
            cur_y += 28.f;

            dl->AddText(f, sz, { tx, cur_y }, ImGui::GetColorU32(colors::text_label), "Gating Mode:");
            dl->AddText(f, sz, { c1_bb.Max.x - layout::section_pad_x - 70.f, cur_y },
                ImGui::GetColorU32(colors::text_secondary), "100% Manual");
        }

        // Card 2: Quick Status Controls
        const ImRect c2_bb(
            { c1_bb.Max.x + gap, content_bb.Min.y + pad_y },
            { content_bb.Max.x - pad_x, content_bb.Min.y + pad_y + 160.f });

        dl->AddRectFilled(c2_bb.Min, c2_bb.Max,
            ImGui::GetColorU32(colors::section_box), layout::section_round);
        draw_section_pattern(dl, c2_bb);

        const ImRect c2_hbb(c2_bb.Min, { c2_bb.Max.x, c2_bb.Min.y + layout::section_header_h });
        dl->AddRectFilled(c2_hbb.Min, c2_hbb.Max,
            ImGui::GetColorU32(colors::section_header), layout::section_round,
            ImDrawFlags_RoundCornersTop);

        if (ImFont* hf = fonts::inter(13.f, true))
        {
            const float sz = fonts::size(hf);
            dl->AddText(hf, sz,
                { c2_hbb.Min.x + layout::section_pad_x, c2_hbb.GetCenter().y - sz * 0.5f },
                ImGui::GetColorU32(colors::text_header), "Master Overview");
        }

        cur_y = c2_hbb.Max.y;
        const float rh = layout::section_row_h;

        ImRect r1({ c2_bb.Min.x, cur_y }, { c2_bb.Max.x, cur_y + rh });
        draw_checkbox(dl, "ESP Master Switch", &cfg::esp::enabled, r1, true);
        draw_row_separator(dl, r1);
        cur_y += rh;

        ImRect r2({ c2_bb.Min.x, cur_y }, { c2_bb.Max.x, cur_y + rh });
        draw_checkbox(dl, "Silent Aim Master", &AimSilentEnabled, r2, true);
    }

    static void draw_tab_settings(ImDrawList* dl, const ImRect& content_bb)
    {
        const float pad_x = layout::content_pad_x;
        const float pad_y = layout::content_pad_y;
        const float gap = layout::section_gap;

        const float avail_w = content_bb.GetWidth() - pad_x * 2.f;
        const float col_w = (avail_w - gap) * 0.5f;

        // Card 1: Appearance
        const ImRect c1_bb(
            { content_bb.Min.x + pad_x, content_bb.Min.y + pad_y },
            { content_bb.Min.x + pad_x + col_w, content_bb.Min.y + pad_y + 160.f });

        dl->AddRectFilled(c1_bb.Min, c1_bb.Max,
            ImGui::GetColorU32(colors::section_box), layout::section_round);
        draw_section_pattern(dl, c1_bb);

        const ImRect c1_hbb(c1_bb.Min, { c1_bb.Max.x, c1_bb.Min.y + layout::section_header_h });
        dl->AddRectFilled(c1_hbb.Min, c1_hbb.Max,
            ImGui::GetColorU32(colors::section_header), layout::section_round,
            ImDrawFlags_RoundCornersTop);

        if (ImFont* hf = fonts::inter(13.f, true))
        {
            const float sz = fonts::size(hf);
            dl->AddText(hf, sz,
                { c1_hbb.Min.x + layout::section_pad_x, c1_hbb.GetCenter().y - sz * 0.5f },
                ImGui::GetColorU32(colors::text_header), "Appearance");
        }

        float cur_y = c1_hbb.Max.y;
        const float drh = layout::section_dropdown_row_h;

        static const char* color_opts[] = { "Electric Blue", "Neon Purple", "Emerald Green", "Crimson Red" };
        ImRect d1({ c1_bb.Min.x, cur_y }, { c1_bb.Max.x, cur_y + drh });
        draw_dropdown(dl, "Accent Palette", &g_state.accent_color_idx, color_opts, 4, d1);

        if (g_state.accent_color_idx == 0)
            colors::accent = { 90.f / 255.f, 160.f / 255.f, 255.f / 255.f, 1.f };
        else if (g_state.accent_color_idx == 1)
            colors::accent = { 168.f / 255.f, 85.f / 255.f, 247.f / 255.f, 1.f };
        else if (g_state.accent_color_idx == 2)
            colors::accent = { 52.f / 255.f, 211.f / 255.f, 153.f / 255.f, 1.f };
        else if (g_state.accent_color_idx == 3)
            colors::accent = { 248.f / 255.f, 113.f / 255.f, 113.f / 255.f, 1.f };
        colors::sync_accent_hover();

        // Card 2: Client Window
        const ImRect c2_bb(
            { c1_bb.Max.x + gap, content_bb.Min.y + pad_y },
            { content_bb.Max.x - pad_x, content_bb.Min.y + pad_y + 160.f });

        dl->AddRectFilled(c2_bb.Min, c2_bb.Max,
            ImGui::GetColorU32(colors::section_box), layout::section_round);
        draw_section_pattern(dl, c2_bb);

        const ImRect c2_hbb(c2_bb.Min, { c2_bb.Max.x, c2_bb.Min.y + layout::section_header_h });
        dl->AddRectFilled(c2_hbb.Min, c2_hbb.Max,
            ImGui::GetColorU32(colors::section_header), layout::section_round,
            ImDrawFlags_RoundCornersTop);

        if (ImFont* hf = fonts::inter(13.f, true))
        {
            const float sz = fonts::size(hf);
            dl->AddText(hf, sz,
                { c2_hbb.Min.x + layout::section_pad_x, c2_hbb.GetCenter().y - sz * 0.5f },
                ImGui::GetColorU32(colors::text_header), "Window Controls");
        }

        cur_y = c2_hbb.Max.y;
        const float rh = layout::section_row_h;

        ImRect r1({ c2_bb.Min.x, cur_y }, { c2_bb.Max.x, cur_y + rh });
        draw_checkbox(dl, "Show Floating Bar", &bar::g_open, r1);
        draw_row_separator(dl, r1);
        cur_y += rh;

        // Button to hide/minimize
        const float btn_w = 120.f, btn_h = 28.f;
        ImRect b_bb({ c2_bb.Min.x + layout::section_pad_x, cur_y + 10.f },
                    { c2_bb.Min.x + layout::section_pad_x + btn_w, cur_y + 10.f + btn_h });

        ImGui::PushID("##hide_menu");
        ImGuiID bid = ImGui::GetID("##b");
        ImGui::ItemAdd(b_bb, bid);
        bool bh = false, bheld = false;
        if (ImGui::ButtonBehavior(b_bb, bid, &bh, &bheld) ||
            (bh && ImGui::IsMouseReleased(0)))
        {
            bar::g_open = false;
        }

        dl->AddRectFilled(b_bb.Min, b_bb.Max,
            ImGui::GetColorU32(bh ? colors::accent_hover : colors::accent_dim), 4.f);
        if (ImFont* bf = fonts::inter(12.f, true))
        {
            const float sz = fonts::size(bf);
            const ImVec2 ts = bf->CalcTextSizeA(sz, FLT_MAX, 0.f, "Minimize Menu");
            dl->AddText(bf, sz,
                { b_bb.GetCenter().x - ts.x * 0.5f, b_bb.GetCenter().y - ts.y * 0.5f },
                ImGui::GetColorU32(colors::text), "Minimize Menu");
        }
        ImGui::PopID();
    }

    static void handle_touch_drag(const ImRect& header_bb)
    {
        const ImVec2 mp = ImGui::GetIO().MousePos;
        const float sw = ImGui::GetIO().DisplaySize.x;
        const float sh = ImGui::GetIO().DisplaySize.y;

        if (ImGui::IsMouseClicked(0) && header_bb.Contains(mp) && !ImGui::IsAnyItemHovered() && g_open_dd.id == 0)
        {
            g_dragging = true;
            g_drag_start = mp;
        }

        if (g_dragging && ImGui::IsMouseDown(0))
        {
            const float dx = mp.x - g_drag_start.x;
            const float dy = mp.y - g_drag_start.y;
            g_win_pos.x += dx;
            g_win_pos.y += dy;
            g_drag_start = mp;

            g_win_pos.x = ImClamp(g_win_pos.x, 5.f, sw - layout::window_w - 5.f);
            g_win_pos.y = ImClamp(g_win_pos.y, 5.f, sh - layout::window_h - 5.f);
        }

        if (g_dragging && ImGui::IsMouseReleased(0))
        {
            g_dragging = false;
        }
    }
}

namespace ui::menu
{
    void render()
    {
        const float sw = ImGui::GetIO().DisplaySize.x;
        const float sh = ImGui::GetIO().DisplaySize.y;

        // Render floating toggle bar if menu is closed
        bar::render();
        if (!bar::g_open)
        {
            return;
        }

        if (!g_pos_initialized && sw > 100.f && sh > 100.f)
        {
            g_win_pos = { (sw - layout::window_w) * 0.5f, (sh - layout::window_h) * 0.5f };
            if (g_win_pos.x < 10.f) g_win_pos.x = 10.f;
            if (g_win_pos.y < 10.f) g_win_pos.y = 10.f;
            g_pos_initialized = true;
        }

        colors::sync_accent_hover();

        const ImVec2 win_sz = { layout::window_w, layout::window_h };
        ImGui::SetNextWindowPos(g_win_pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(win_sz, ImGuiCond_Always);

        ImGui::Begin(layout::main_window_id, nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImRect panel(g_win_pos, { g_win_pos.x + win_sz.x, g_win_pos.y + win_sz.y });

        // Outer window background & border
        dl->AddRectFilled(panel.Min, panel.Max,
            ImGui::GetColorU32(colors::bg), layout::window_round);
        dl->AddRect(panel.Min, panel.Max,
            ImGui::GetColorU32(colors::accent_dim), layout::window_round, 0, 1.2f);

        const ImRect sidebar_bb(panel.Min, { panel.Min.x + layout::sidebar_w, panel.Max.y });
        const ImRect right_area({ sidebar_bb.Max.x, panel.Min.y }, panel.Max);
        const ImRect header_bb(right_area.Min, { right_area.Max.x, right_area.Min.y + layout::header_h });
        const ImRect content_bb({ right_area.Min.x, header_bb.Max.y }, right_area.Max);

        draw_sidebar(dl, sidebar_bb);
        draw_header(dl, header_bb);

        switch (g_state.active)
        {
            case tab::main:     draw_tab_main(dl, content_bb);     break;
            case tab::visuals:  draw_tab_visuals(dl, content_bb);  break;
            case tab::aim:      draw_tab_aim(dl, content_bb);      break;
            case tab::settings: draw_tab_settings(dl, content_bb); break;
        }

        // Draw open dropdown floating on top of all content
        draw_open_dropdown(dl, panel);

        if (g_state.exit_pressed)
        {
            bar::g_open = false;
            g_state.exit_pressed = false;
        }

        handle_touch_drag(header_bb);

        ImGui::End();
    }
}

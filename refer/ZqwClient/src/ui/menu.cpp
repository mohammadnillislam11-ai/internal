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
#include "Android_touch/Touch.hpp"
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
    enum class tab { visuals, aim, dashboard, settings };
    enum class ctrl_type { checkbox, checkbox_warn, slider, dropdown, color_swatch };

    struct setting_row
    {
        const char*        label;
        ctrl_type          type;
        bool*              bind_bool = nullptr;
        float*             bind_float = nullptr;
        int*               bind_int = nullptr;
        ImVec4*            bind_col = nullptr;
        const char* const* dd_opts = nullptr;
        float              v_min = 0.f;
        float              v_max = 1.f;
    };

    struct app_state
    {
        tab   active = tab::visuals;
        bool  exit_pressed = false;
        float scroll_y = 0.f;
        float max_scroll_y = 0.f;
        bool  is_scroll_dragging = false;

        int target_bone_idx = 0;
        int target_prio_idx = 0;
    };

    static app_state g_state;

    struct open_dd_t
    {
        ImGuiID            id = 0;
        ImRect             closed_bb;
        int*               index = nullptr;
        const char* const* opts = nullptr;
    };
    static open_dd_t g_open_dd;

    static bool   g_dragging = false;
    static ImVec2 g_drag_start = {};
    static ImVec2 g_win_pos(100.f, 80.f);
    static bool   g_pos_initialized = false;

    struct tab_def
    {
        tab         id;
        const char* label;
        const char* icon;
    };

    static const tab_def g_tabs[] = {
        { tab::visuals,   "Visuals",   icons::eye      },
        { tab::aim,       "Aim",       icons::mouse    },
        { tab::dashboard, "Dashboard", icons::home     },
        { tab::settings,  "Settings",  icons::settings },
    };
    static const int g_tab_count = (int)(sizeof(g_tabs) / sizeof(g_tabs[0]));

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

    static void draw_checkbox(ImDrawList* dl, const char* label, bool* value, const ImRect& row, bool warn)
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

        if (pressed || (hovered && ImGui::IsMouseReleased(0) && !ImGui::IsMouseDragging(0, 8.f)))
        {
            if (value) *value = !*value;
        }

        bool is_checked = value ? *value : false;
        static float anims[256] = {};
        const int slot = (int)(gid % 256);
        anims[slot] = ImLerp(anims[slot], is_checked ? 1.f : 0.f,
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
            const ImVec4 col = is_checked ? colors::text : colors::text_label;
            dl->AddText(font, sz,
                { tx, row.GetCenter().y - sz * 0.5f },
                ImGui::GetColorU32(col), label);
        }

        ImGui::PopID();
    }

    static void draw_slider(ImDrawList* dl, const char* label, float* value, float v_min, float v_max,
                            const ImRect& row, bool warn)
    {
        ImGui::PushID(label);
        ImGuiID gid = ImGui::GetID("##sl");

        const float pad = layout::section_pad_x;
        const float bar_left  = row.Min.x + pad;
        const float bar_right = row.Max.x - pad;
        const float bar_w     = bar_right - bar_left;

        const float top_y = row.Min.y + 8.f;
        float lx = bar_left;

        if (warn)
        {
            if (ImFont* wf = fonts::icon(12.f))
            {
                const float ws = fonts::size(wf);
                dl->AddText(wf, ws,
                    { lx, top_y + 1.f },
                    ImGui::GetColorU32(colors::warning), icons::warning);
                lx += ws + 6.f;
            }
        }

        if (ImFont* lf = fonts::inter(13.f))
        {
            const float lsz = fonts::size(lf);
            dl->AddText(lf, lsz,
                { lx, top_y },
                ImGui::GetColorU32(colors::text_label), label);
        }

        char val_buf[16];
        float cur_v = value ? *value : v_min;
        if (v_max > 50.f)
            snprintf(val_buf, sizeof(val_buf), "%.0fm", cur_v);
        else if (v_max > 5.f)
            snprintf(val_buf, sizeof(val_buf), "%.1f", cur_v);
        else
            snprintf(val_buf, sizeof(val_buf), "x%.2f", cur_v);

        if (ImFont* vf = fonts::inter(13.f, true))
        {
            const float vsz = fonts::size(vf);
            const ImVec2 vts = vf->CalcTextSizeA(vsz, FLT_MAX, 0.f, val_buf);
            dl->AddText(vf, vsz,
                { bar_right - vts.x, top_y },
                ImGui::GetColorU32(colors::text), val_buf);
        }

        const float bar_y = row.Max.y - 12.f;
        const float track_h = layout::slider_track_h;
        const float dot_r   = layout::slider_dot_r;

        const ImRect grab_area(
            { bar_left - 4.f,  bar_y - track_h * 0.5f - 6.f },
            { bar_right + 4.f, bar_y + track_h * 0.5f + 6.f });

        ImGui::ItemAdd(grab_area, gid);
        bool hovered = false, held = false;
        ImGui::ButtonBehavior(grab_area, gid, &hovered, &held);

        if (held && value)
        {
            g_state.is_scroll_dragging = false;
            float t = (ImGui::GetIO().MousePos.x - bar_left) / bar_w;
            t = ImClamp(t, 0.f, 1.f);
            *value = v_min + t * (v_max - v_min);
        }

        float pct = (cur_v - v_min) / (v_max - v_min);
        pct = ImClamp(pct, 0.f, 1.f);
        const float fill_x = bar_left + pct * bar_w;

        const float r = track_h * 0.5f;
        dl->AddRectFilled(
            { bar_left,  bar_y - track_h * 0.5f },
            { bar_right, bar_y + track_h * 0.5f },
            ImGui::GetColorU32(colors::slider_track), r);

        if (fill_x > bar_left + 0.5f)
        {
            dl->AddRectFilled(
                { bar_left, bar_y - track_h * 0.5f },
                { fill_x,   bar_y + track_h * 0.5f },
                ImGui::GetColorU32(held ? colors::accent_hover : colors::accent), r);

            dl->AddCircleFilled({ fill_x - r, bar_y }, dot_r,
                ImGui::GetColorU32(colors::bg));
        }

        ImGui::PopID();
    }

    static void draw_dropdown(ImDrawList* dl, const char* label, int* index, const char* const* opts,
                              const ImRect& row, bool warn)
    {
        ImGui::PushID(label);
        ImGuiID gid = ImGui::GetID("##dd");

        const float pad = layout::section_pad_x;
        const float box_h = layout::dropdown_h;

        float lx = row.Min.x + pad;
        if (warn)
        {
            if (ImFont* wf = fonts::icon(12.f))
            {
                const float ws = fonts::size(wf);
                dl->AddText(wf, ws,
                    { lx, row.Min.y + 6.f + 1.f },
                    ImGui::GetColorU32(colors::warning), icons::warning);
                lx += ws + 6.f;
            }
        }

        if (ImFont* font = fonts::inter(13.f))
        {
            const float sz = fonts::size(font);
            dl->AddText(font, sz,
                { lx, row.Min.y + 6.f },
                ImGui::GetColorU32(colors::text_label), label);
        }

        const ImRect box(
            { row.Min.x + pad, row.Max.y - 6.f - box_h },
            { row.Max.x - pad, row.Max.y - 6.f });

        ImGui::ItemAdd(box, gid);
        bool hovered = false, held = false;
        if (ImGui::ButtonBehavior(box, gid, &hovered, &held))
        {
            if (g_open_dd.id == gid)
            {
                g_open_dd.id = 0;
            }
            else
            {
                g_open_dd.id = gid;
                g_open_dd.closed_bb = box;
                g_open_dd.index = index;
                g_open_dd.opts = opts;
            }
        }

        const bool this_open = (g_open_dd.id == gid);

        static float hover_anims[64] = {};
        const int slot = (int)(gid % 64);
        hover_anims[slot] = ImLerp(hover_anims[slot], (hovered || this_open) ? 1.f : 0.f,
            ImMin(1.f, ImGui::GetIO().DeltaTime * 16.f));

        ImVec4 bg = ImLerp(colors::dropdown_bg, colors::section_header, hover_anims[slot] * 0.6f);
        dl->AddRectFilled(box.Min, box.Max, ImGui::GetColorU32(bg), layout::dropdown_round);

        int cur_idx = index ? *index : 0;
        const char* val = (opts && opts[cur_idx]) ? opts[cur_idx] : "-";
        if (ImFont* tf = fonts::inter(13.f))
        {
            const float tsz = fonts::size(tf);
            dl->AddText(tf, tsz,
                { box.Min.x + 10.f, box.GetCenter().y - tsz * 0.5f },
                ImGui::GetColorU32(colors::text), val);
        }

        if (ImFont* icf = fonts::icon(9.f))
        {
            const float isz = fonts::size(icf);
            const ImVec2 its = icf->CalcTextSizeA(isz, FLT_MAX, 0.f, icons::chevron_d);
            dl->AddText(icf, isz,
                { box.Max.x - its.x - 10.f, box.GetCenter().y - its.y * 0.5f },
                ImGui::GetColorU32(colors::text_secondary), icons::chevron_d);
        }

        ImGui::PopID();
    }

    static void draw_color_swatch(ImDrawList* dl, const char* label, ImVec4* col_var, const ImRect& row)
    {
        ImGui::PushID(label);
        ImGuiID gid = ImGui::GetID("##cs");

        const float pad = layout::section_pad_x;
        const float sw = layout::color_swatch_sz;
        const ImRect sw_bb(
            { row.Max.x - pad - sw, row.GetCenter().y - sw * 0.5f },
            { row.Max.x - pad,      row.GetCenter().y + sw * 0.5f });

        ImGui::ItemAdd(sw_bb, gid);
        bool hovered = false, held = false;
        if (ImGui::ButtonBehavior(sw_bb, gid, &hovered, &held))
            ImGui::OpenPopup("##accent_color_popup");

        ImVec4 cur_c = col_var ? *col_var : colors::accent;
        char hex_buf[12];
        snprintf(hex_buf, sizeof(hex_buf), "#%02X%02X%02X",
            (int)(cur_c.x * 255), (int)(cur_c.y * 255), (int)(cur_c.z * 255));

        if (ImFont* hf = fonts::inter(13.f, true))
        {
            const float sz = fonts::size(hf);
            const ImVec2 ts = hf->CalcTextSizeA(sz, FLT_MAX, 0.f, hex_buf);
            dl->AddText(hf, sz,
                { sw_bb.Min.x - ts.x - 10.f, row.GetCenter().y - sz * 0.5f },
                ImGui::GetColorU32(colors::text_secondary), hex_buf);
        }

        if (ImFont* icf = fonts::icon(16.f))
        {
            const float isz = fonts::size(icf);
            const ImVec2 its = icf->CalcTextSizeA(isz, FLT_MAX, 0.f, icons::art_gallery);
            dl->AddText(icf, isz,
                { sw_bb.GetCenter().x - its.x * 0.5f, sw_bb.GetCenter().y - its.y * 0.5f },
                ImGui::GetColorU32(cur_c), icons::art_gallery);
        }

        if (ImFont* font = fonts::inter(13.f))
        {
            const float sz = fonts::size(font);
            dl->AddText(font, sz,
                { row.Min.x + pad, row.GetCenter().y - sz * 0.5f },
                ImGui::GetColorU32(colors::text_label), label);
        }

        if (ImGui::BeginPopup("##accent_color_popup"))
        {
            ImGui::PushStyleColor(ImGuiCol_WindowBg, colors::dropdown_pop_bg);
            ImGui::Text("%s", label);
            ImGui::Separator();
            if (col_var && ImGui::ColorPicker3("##cp", &col_var->x,
                ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoLabel))
            {
                if (col_var == &colors::accent)
                    colors::sync_accent_hover();
            }
            ImGui::PopStyleColor();
            ImGui::EndPopup();
        }

        ImGui::PopID();
    }

    static void draw_section_box(ImDrawList* dl, const char* title, const char* icon_glyph,
        const setting_row rows[], int count, float& y, const ImRect& area)
    {
        const float round = layout::section_round;
        const float header_h = layout::section_header_h;

        float body_h = layout::section_pad_y * 2.f;
        for (int i = 0; i < count; ++i)
        {
            switch (rows[i].type)
            {
            case ctrl_type::slider:        body_h += layout::section_slider_row_h; break;
            case ctrl_type::dropdown:      body_h += layout::section_dropdown_row_h; break;
            case ctrl_type::color_swatch:  body_h += layout::section_color_row_h; break;
            default:                       body_h += layout::section_row_h; break;
            }
        }
        const float total_h = header_h + body_h;

        const ImRect box({ area.Min.x, y }, { area.Max.x, y + total_h });

        dl->AddRectFilled(box.Min, box.Max,
            ImGui::GetColorU32(colors::section_box), round);

        const ImRect header(box.Min, { box.Max.x, box.Min.y + header_h });
        dl->AddRectFilled(header.Min, header.Max,
            ImGui::GetColorU32(colors::section_header), round, ImDrawFlags_RoundCornersTop);

        const ImRect body({ box.Min.x, header.Max.y }, box.Max);
        draw_section_pattern(dl, body);

        float title_x = header.Min.x + layout::section_pad_x;
        if (icon_glyph)
        {
            if (ImFont* icf = fonts::icon(13.f))
            {
                const float isz = fonts::size(icf);
                const ImVec2 its = icf->CalcTextSizeA(isz, FLT_MAX, 0.f, icon_glyph);
                dl->AddText(icf, isz,
                    { title_x, header.GetCenter().y - its.y * 0.5f },
                    ImGui::GetColorU32(colors::icon_section), icon_glyph);
                title_x += its.x + 8.f;
            }
        }

        if (ImFont* hf = fonts::inter(13.f, true))
        {
            const float sz = fonts::size(hf);
            dl->AddText(hf, sz,
                { title_x, header.GetCenter().y - sz * 0.5f },
                ImGui::GetColorU32(colors::text_header), title);
        }

        float ry = box.Min.y + header_h + layout::section_pad_y;
        for (int i = 0; i < count; ++i)
        {
            const auto& r = rows[i];
            float row_h;
            switch (r.type)
            {
            case ctrl_type::slider:        row_h = layout::section_slider_row_h; break;
            case ctrl_type::dropdown:      row_h = layout::section_dropdown_row_h; break;
            case ctrl_type::color_swatch:  row_h = layout::section_color_row_h; break;
            default:                       row_h = layout::section_row_h; break;
            }

            const ImRect row({ box.Min.x, ry }, { box.Max.x, ry + row_h });

            switch (r.type)
            {
            case ctrl_type::checkbox:
                draw_checkbox(dl, r.label, r.bind_bool, row, false);
                break;
            case ctrl_type::checkbox_warn:
                draw_checkbox(dl, r.label, r.bind_bool, row, true);
                break;
            case ctrl_type::slider:
                draw_slider(dl, r.label, r.bind_float, r.v_min, r.v_max, row, false);
                break;
            case ctrl_type::dropdown:
                draw_dropdown(dl, r.label, r.bind_int, r.dd_opts, row, false);
                break;
            case ctrl_type::color_swatch:
                draw_color_swatch(dl, r.label, r.bind_col, row);
                break;
            }

            if (i < count - 1)
                draw_row_separator(dl, row);

            ry += row_h;
        }

        y += total_h + layout::section_gap;
    }

    static void draw_tab_content(ImDrawList* dl, const ImRect& area)
    {
        const float pad_x = layout::content_pad_x;
        const float pad_y = layout::content_pad_y;
        const float col_gap = 10.f;
        const float scroll_reserve = layout::scrollbar_w + 6.f;
        const float full_w = area.GetWidth() - pad_x * 2.f - scroll_reserve;
        const float col_w = (full_w - col_gap) * 0.5f;

        // Interactive mouse wheel and mobile touch-drag scrolling
        const ImVec2 mp = ImGui::GetIO().MousePos;
        if (area.Contains(mp) && !g_dragging && g_open_dd.id == 0)
        {
            float wheel = ImGui::GetIO().MouseWheel;
            if (wheel != 0.f)
            {
                g_state.scroll_y -= wheel * 35.f;
            }

            if (ImGui::IsMouseClicked(0))
            {
                g_state.is_scroll_dragging = true;
            }
        }

        if (g_state.is_scroll_dragging && ImGui::IsMouseDown(0))
        {
            float delta_y = ImGui::GetIO().MouseDelta.y;
            if (std::fabs(delta_y) > 0.01f)
            {
                g_state.scroll_y -= delta_y;
            }
        }

        if (ImGui::IsMouseReleased(0))
        {
            g_state.is_scroll_dragging = false;
        }

        g_state.scroll_y = ImClamp(g_state.scroll_y, 0.f, g_state.max_scroll_y);

        const ImRect left_col(
            { area.Min.x + pad_x, area.Min.y + pad_y },
            { area.Min.x + pad_x + col_w, area.Max.y - pad_y });
        const ImRect right_col(
            { area.Min.x + pad_x + col_w + col_gap, area.Min.y + pad_y },
            { area.Max.x - pad_x - scroll_reserve, area.Max.y - pad_y });

        dl->PushClipRect(area.Min, area.Max, true);

        float yl = left_col.Min.y - g_state.scroll_y;
        float yr = right_col.Min.y - g_state.scroll_y;

        switch (g_state.active)
        {
        case tab::visuals:
        {
            static const setting_row esp_elem_rows[] = {
                { "Enable ESP (Master)", ctrl_type::checkbox_warn, &cfg::esp::enabled,   nullptr, nullptr, nullptr, nullptr, 0.f, 0.f },
                { "ESP Line (Thin)",     ctrl_type::checkbox,      &cfg::esp::tracer,    nullptr, nullptr, nullptr, nullptr, 0.f, 0.f },
                { "ESP Box",             ctrl_type::checkbox,      &cfg::esp::box,       nullptr, nullptr, nullptr, nullptr, 0.f, 0.f },
                { "ESP Skeleton",        ctrl_type::checkbox,      &cfg::esp::skeleton,  nullptr, nullptr, nullptr, nullptr, 0.f, 0.f },
                { "Screen Crosshair",    ctrl_type::checkbox,      &cfg::esp::crosshair, nullptr, nullptr, nullptr, nullptr, 0.f, 0.f },
            };
            draw_section_box(dl, "ESP Elements", icons::eye, esp_elem_rows, 5, yl, left_col);

            static const char* box_style_opts[] = { "Corner Brackets", "Full Outline", nullptr };
            static const setting_row esp_cfg_rows[] = {
                { "ESP Health Bar",      ctrl_type::checkbox,      &cfg::esp::health,        nullptr, nullptr, nullptr, nullptr, 0.f, 0.f },
                { "ESP Name",            ctrl_type::checkbox,      &cfg::esp::name,          nullptr, nullptr, nullptr, nullptr, 0.f, 0.f },
                { "ESP Distance",        ctrl_type::checkbox,      &cfg::esp::distance,      nullptr, nullptr, nullptr, nullptr, 0.f, 0.f },
                { "Box Style",           ctrl_type::dropdown,      nullptr, nullptr, &cfg::esp::box_type, nullptr, box_style_opts, 0.f, 0.f },
                { "Render Range",        ctrl_type::slider,        nullptr, &cfg::esp::max_range,     nullptr, nullptr, nullptr, 50.f, 500.f },
                { "Box Thickness",       ctrl_type::slider,        nullptr, &cfg::esp::box_thickness, nullptr, nullptr, nullptr, 0.5f, 3.0f },
                { "Box Rounding",        ctrl_type::slider,        nullptr, &cfg::esp::box_rounding,  nullptr, nullptr, nullptr, 0.0f, 8.0f },
            };
            draw_section_box(dl, "Player Info & Range", icons::sliders, esp_cfg_rows, 7, yr, right_col);

            static const setting_row col_rows[] = {
                { "Box Color",           ctrl_type::color_swatch,  nullptr, nullptr, nullptr, &cfg::esp::box_col,     nullptr, 0.f, 0.f },
                { "Visible Color",       ctrl_type::color_swatch,  nullptr, nullptr, nullptr, &cfg::esp::box_visible, nullptr, 0.f, 0.f },
            };
            draw_section_box(dl, "Visual Colors", icons::color_pick, col_rows, 2, yl, left_col);
            break;
        }
        case tab::aim:
        {
            static const setting_row aim_master_rows[] = {
                { "Enable Silent Aim",   ctrl_type::checkbox_warn, &AimSilentEnabled, nullptr, nullptr, nullptr, nullptr, 0.f, 0.f },
                { "Nobita Mode",         ctrl_type::checkbox,      &AimSilentNobita,  nullptr, nullptr, nullptr, nullptr, 0.f, 0.f },
            };
            draw_section_box(dl, "Silent Aim", icons::mouse, aim_master_rows, 2, yl, left_col);

            static const char* bone_opts[] = { "Head", "Neck", "Chest", "Pelvis", nullptr };
            static const char* prio_opts[] = { "Closest Distance", "Lowest HP", "Crosshair FOV", nullptr };
            static const setting_row aim_cfg_rows[] = {
                { "Target Bone",         ctrl_type::dropdown,      nullptr, nullptr, &g_state.target_bone_idx, nullptr, bone_opts, 0.f, 0.f },
                { "Priority Target",     ctrl_type::dropdown,      nullptr, nullptr, &g_state.target_prio_idx, nullptr, prio_opts, 0.f, 0.f },
            };
            draw_section_box(dl, "Target Configuration", icons::keyboard, aim_cfg_rows, 2, yr, right_col);
            break;
        }
        case tab::dashboard:
        {
            static const setting_row dash_switches[] = {
                { "ESP Master Switch",   ctrl_type::checkbox_warn, &cfg::esp::enabled, nullptr, nullptr, nullptr, nullptr, 0.f, 0.f },
                { "Silent Aim Master",   ctrl_type::checkbox_warn, &AimSilentEnabled,  nullptr, nullptr, nullptr, nullptr, 0.f, 0.f },
            };
            draw_section_box(dl, "Master Switches", icons::warning, dash_switches, 2, yl, left_col);

            // System info card on right
            const float round = layout::section_round;
            const float total_h = layout::section_header_h + 120.f;
            const ImRect box({ right_col.Min.x, yr }, { right_col.Max.x, yr + total_h });

            dl->AddRectFilled(box.Min, box.Max, ImGui::GetColorU32(colors::section_box), round);
            const ImRect header(box.Min, { box.Max.x, box.Min.y + layout::section_header_h });
            dl->AddRectFilled(header.Min, header.Max, ImGui::GetColorU32(colors::section_header), round, ImDrawFlags_RoundCornersTop);
            draw_section_pattern(dl, ImRect({ box.Min.x, header.Max.y }, box.Max));

            if (ImFont* hf = fonts::inter(13.f, true))
            {
                const float sz = fonts::size(hf);
                dl->AddText(hf, sz, { header.Min.x + layout::section_pad_x, header.GetCenter().y - sz * 0.5f },
                    ImGui::GetColorU32(colors::text_header), "System Status");
            }

            float sy = header.Max.y + 10.f;
            if (ImFont* f = fonts::inter(13.f))
            {
                const float sz = fonts::size(f);
                float tx = box.Min.x + layout::section_pad_x;

                char fps_buf[32];
                snprintf(fps_buf, sizeof(fps_buf), "%.1f FPS", ImGui::GetIO().Framerate);
                dl->AddText(f, sz, { tx, sy }, ImGui::GetColorU32(colors::text_label), "Engine Rate:");
                dl->AddText(f, sz, { box.Max.x - layout::section_pad_x - 65.f, sy }, ImGui::GetColorU32(colors::text), fps_buf);
                sy += 26.f;

                dl->AddText(f, sz, { tx, sy }, ImGui::GetColorU32(colors::text_label), "Control Gating:");
                dl->AddText(f, sz, { box.Max.x - layout::section_pad_x - 70.f, sy }, ImGui::GetColorU32(colors::accent), "100% Manual");
                sy += 26.f;

                dl->AddText(f, sz, { tx, sy }, ImGui::GetColorU32(colors::text_label), "Touch Status:");
                dl->AddText(f, sz, { box.Max.x - layout::section_pad_x - 55.f, sy }, ImGui::GetColorU32(colors::text_secondary), "Active");
            }
            yr += total_h + layout::section_gap;
            break;
        }
        case tab::settings:
        {
            static const setting_row appear_rows[] = {
                { "Accent Color",        ctrl_type::color_swatch,  nullptr, nullptr, nullptr, &colors::accent, nullptr, 0.f, 0.f },
                { "Show Floating Bar",   ctrl_type::checkbox,      &bar::g_open, nullptr, nullptr, nullptr, nullptr, 0.f, 0.f },
            };
            draw_section_box(dl, "Appearance & Bar", icons::color, appear_rows, 2, yl, left_col);

            const float round = layout::section_round;
            const float total_h = layout::section_header_h + 100.f;
            const ImRect box({ right_col.Min.x, yr }, { right_col.Max.x, yr + total_h });

            dl->AddRectFilled(box.Min, box.Max, ImGui::GetColorU32(colors::section_box), round);
            const ImRect header(box.Min, { box.Max.x, box.Min.y + layout::section_header_h });
            dl->AddRectFilled(header.Min, header.Max, ImGui::GetColorU32(colors::section_header), round, ImDrawFlags_RoundCornersTop);
            draw_section_pattern(dl, ImRect({ box.Min.x, header.Max.y }, box.Max));

            if (ImFont* hf = fonts::inter(13.f, true))
            {
                const float sz = fonts::size(hf);
                dl->AddText(hf, sz, { header.Min.x + layout::section_pad_x, header.GetCenter().y - sz * 0.5f },
                    ImGui::GetColorU32(colors::text_header), "Controls & Input");
            }

            float cy = header.Max.y + 12.f;
            const float btn_w = 140.f, btn_h = 30.f;

            // Flip Orientation button
            ImRect b1_bb({ box.Min.x + layout::section_pad_x, cy },
                         { box.Min.x + layout::section_pad_x + btn_w, cy + btn_h });

            ImGui::PushID("##flip_ori");
            ImGuiID bid1 = ImGui::GetID("##fo");
            ImGui::ItemAdd(b1_bb, bid1);
            bool b1_h = false, b1_held = false;
            if (ImGui::ButtonBehavior(b1_bb, bid1, &b1_h, &b1_held) ||
                (b1_h && ImGui::IsMouseReleased(0)))
            {
                touch::toggleOrientation();
            }

            dl->AddRectFilled(b1_bb.Min, b1_bb.Max,
                ImGui::GetColorU32(b1_h ? colors::accent_hover : colors::accent_dim), 4.f);
            if (ImFont* bf = fonts::inter(12.f, true))
            {
                const float sz = fonts::size(bf);
                const ImVec2 ts = bf->CalcTextSizeA(sz, FLT_MAX, 0.f, "Flip Orientation");
                dl->AddText(bf, sz,
                    { b1_bb.GetCenter().x - ts.x * 0.5f, b1_bb.GetCenter().y - ts.y * 0.5f },
                    ImGui::GetColorU32(colors::text), "Flip Orientation");
            }
            ImGui::PopID();

            // Minimize Menu button
            ImRect b2_bb({ box.Min.x + layout::section_pad_x + btn_w + 10.f, cy },
                         { box.Min.x + layout::section_pad_x + btn_w * 2.f + 10.f, cy + btn_h });

            ImGui::PushID("##min_btn");
            ImGuiID bid2 = ImGui::GetID("##mb");
            ImGui::ItemAdd(b2_bb, bid2);
            bool b2_h = false, b2_held = false;
            if (ImGui::ButtonBehavior(b2_bb, bid2, &b2_h, &b2_held) ||
                (b2_h && ImGui::IsMouseReleased(0)))
            {
                bar::g_open = false;
            }

            dl->AddRectFilled(b2_bb.Min, b2_bb.Max,
                ImGui::GetColorU32(b2_h ? colors::accent_hover : colors::accent_dim), 4.f);
            if (ImFont* bf = fonts::inter(12.f, true))
            {
                const float sz = fonts::size(bf);
                const ImVec2 ts = bf->CalcTextSizeA(sz, FLT_MAX, 0.f, "Minimize Menu");
                dl->AddText(bf, sz,
                    { b2_bb.GetCenter().x - ts.x * 0.5f, b2_bb.GetCenter().y - ts.y * 0.5f },
                    ImGui::GetColorU32(colors::text), "Minimize Menu");
            }
            ImGui::PopID();

            yr += total_h + layout::section_gap;
            break;
        }
        }

        const float content_h_left = yl + g_state.scroll_y - left_col.Min.y;
        const float content_h_right = yr + g_state.scroll_y - right_col.Min.y;
        const float total_h = std::max(content_h_left, content_h_right);
        const float view_h = area.GetHeight() - pad_y * 2.f;
        g_state.max_scroll_y = std::max(0.f, total_h - view_h);

        dl->PopClipRect();
    }

    static void draw_scrollbar(ImDrawList* dl, const ImRect& area)
    {
        if (g_state.max_scroll_y <= 0.01f)
            return;

        const float w = layout::scrollbar_w;
        const float pad_y = 8.f;
        const ImRect sb(
            { area.Max.x - w - 4.f, area.Min.y + pad_y },
            { area.Max.x - 4.f,     area.Max.y - pad_y });

        dl->AddRectFilled(sb.Min, sb.Max,
            ImGui::GetColorU32(colors::scrollbar_bg), layout::scrollbar_round);

        const float track_h = sb.GetHeight();
        const float view_h = area.GetHeight() - layout::content_pad_y * 2.f;
        const float total_content_h = view_h + g_state.max_scroll_y;
        const float grab_h = ImClamp(track_h * (view_h / total_content_h), 24.f, track_h * 0.8f);

        // Interactive thumb dragging on mobile and PC
        const ImVec2 mp = ImGui::GetIO().MousePos;
        static bool s_sb_dragging = false;
        const ImRect hit_sb({ sb.Min.x - 8.f, sb.Min.y }, { sb.Max.x + 8.f, sb.Max.y });

        if (ImGui::IsMouseClicked(0) && hit_sb.Contains(mp) && g_open_dd.id == 0)
        {
            s_sb_dragging = true;
        }

        if (s_sb_dragging && ImGui::IsMouseDown(0))
        {
            float ratio = (mp.y - sb.Min.y - grab_h * 0.5f) / (track_h - grab_h);
            ratio = ImClamp(ratio, 0.f, 1.f);
            g_state.scroll_y = ratio * g_state.max_scroll_y;
        }

        if (ImGui::IsMouseReleased(0))
        {
            s_sb_dragging = false;
        }

        const float scroll_ratio = (g_state.max_scroll_y > 0.01f)
            ? (g_state.scroll_y / g_state.max_scroll_y) : 0.f;

        const float grab_y = sb.Min.y + scroll_ratio * (track_h - grab_h);
        dl->AddRectFilled(
            { sb.Min.x, grab_y },
            { sb.Max.x, grab_y + grab_h },
            ImGui::GetColorU32(colors::scrollbar_grab), layout::scrollbar_round);
    }

    static void draw_sidebar(ImDrawList* dl, const ImRect& sb)
    {
        const float btn_sz = layout::tab_btn_size;
        const float cx = sb.GetCenter().x;
        const float start_y = sb.Min.y + 14.f;
        const float dt = ImGui::GetIO().DeltaTime;

        static float sel_anim[8]   = {};
        static float hover_anim[8] = {};

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
            {
                g_state.active = g_tabs[i].id;
                g_state.scroll_y = 0.f;
            }

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
                        0.35f * sel_anim[i])),
                    layout::tab_round);
            }

            if (hover_anim[i] > 0.01f)
            {
                dl->AddRectFilled(tab_bb.Min, tab_bb.Max,
                    ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, 0.06f * hover_anim[i])),
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

        const char* socials[] = { icons::user, icons::youtube, icons::pin };
        const int social_count = 3;
        const float social_h = 26.f;
        const float social_start = sb.Max.y - 10.f - social_count * social_h;

        for (int i = 0; i < social_count; ++i)
        {
            const float sy = social_start + i * social_h;
            const ImRect sbb({ cx - 12.f, sy }, { cx + 12.f, sy + social_h - 4.f });

            if (ImFont* icf = fonts::icon(12.f))
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

        const char* tab_name = "Visuals";
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

        const float search_w = layout::search_w;
        const float search_h = layout::search_h;
        const float search_gap = 10.f;
        const ImRect search_bb(
            { exit_bb.Min.x - search_w - search_gap, center_y - search_h * 0.5f },
            { exit_bb.Min.x - search_gap,            center_y + search_h * 0.5f });

        dl->AddRectFilled(search_bb.Min, search_bb.Max,
            ImGui::GetColorU32(colors::search_bg), layout::search_round);

        if (ImFont* icf = fonts::icon(11.f))
        {
            const float isz = fonts::size(icf);
            dl->AddText(icf, isz,
                { search_bb.Min.x + 12.f, search_bb.GetCenter().y - isz * 0.5f },
                ImGui::GetColorU32(colors::text_muted), icons::search);
        }

        if (ImFont* sf = fonts::inter(12.f))
        {
            const float sz = fonts::size(sf);
            dl->AddText(sf, sz,
                { search_bb.Min.x + 32.f, search_bb.GetCenter().y - sz * 0.5f },
                ImGui::GetColorU32(colors::text_muted), "Search...");
        }
    }

    static void draw_open_dropdown(ImDrawList* dl, const ImRect& panel)
    {
        if (g_open_dd.id == 0 || !g_open_dd.opts || !g_open_dd.index) return;

        int count = 0;
        while (count < 8 && g_open_dd.opts[count]) ++count;
        if (count == 0) { g_open_dd.id = 0; return; }

        const float item_h = 26.f;
        const float pad_y = 5.f;
        const ImRect closed = g_open_dd.closed_bb;
        const float popup_h = pad_y * 2.f + count * item_h;

        const ImRect popup(
            { closed.Min.x, closed.Max.y + 3.f },
            { closed.Max.x, closed.Max.y + 3.f + popup_h });

        dl->AddRectFilled(popup.Min, popup.Max,
            ImGui::GetColorU32(colors::dropdown_pop_bg), layout::dropdown_round);

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
                    ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, 0.06f)), 3.f);

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

    static void handle_touch_drag(const ImRect& header_bb, const ImRect& sidebar_bb)
    {
        const ImVec2 mp = ImGui::GetIO().MousePos;
        const float sw = ImGui::GetIO().DisplaySize.x;
        const float sh = ImGui::GetIO().DisplaySize.y;

        const float safe_header_w = header_bb.GetWidth() - layout::exit_btn_sz - layout::search_w - 30.f;
        const ImRect drag_header(header_bb.Min, { header_bb.Min.x + safe_header_w, header_bb.Max.y });

        const float tabs_bottom = sidebar_bb.Min.y + 14.f + g_tab_count * (layout::tab_btn_size + layout::tab_gap);
        const float socials_top = sidebar_bb.Max.y - 10.f - 3 * 26.f;
        const ImRect drag_sidebar({ sidebar_bb.Min.x, tabs_bottom }, { sidebar_bb.Max.x, socials_top });

        const bool can_drag = drag_header.Contains(mp) || (drag_sidebar.GetHeight() > 20.f && drag_sidebar.Contains(mp));

        if (ImGui::IsMouseClicked(0) && can_drag && g_open_dd.id == 0)
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

        dl->AddRectFilled(panel.Min, panel.Max,
            ImGui::GetColorU32(colors::bg), layout::window_round);

        const ImRect sidebar_bb(panel.Min, { panel.Min.x + layout::sidebar_w, panel.Max.y });
        const ImRect right_area({ sidebar_bb.Max.x, panel.Min.y }, panel.Max);
        const ImRect header_bb(right_area.Min, { right_area.Max.x, right_area.Min.y + layout::header_h });
        const ImRect content_bb({ right_area.Min.x, header_bb.Max.y }, right_area.Max);

        draw_sidebar(dl, sidebar_bb);
        draw_header(dl, header_bb);
        draw_tab_content(dl, content_bb);
        draw_scrollbar(dl, content_bb);

        draw_open_dropdown(dl, panel);

        if (g_state.exit_pressed)
        {
            bar::g_open = false;
            g_state.exit_pressed = false;
        }

        handle_touch_drag(header_bb, sidebar_bb);

        ImGui::End();
    }
}

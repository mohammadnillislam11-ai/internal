#include "clipboard.h"
#include "imgui.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace clipboard {

namespace {
    static std::string s_clipboard_cache = "";

    const char* ImGui_GetClipboard(void* user_data) {
        s_clipboard_cache = get_text();
        return s_clipboard_cache.c_str();
    }

    void ImGui_SetClipboard(void* user_data, const char* text) {
        if (text) {
            set_text(text);
        }
    }
}

std::string get_text() {
    // 1. Try termux clipboard if available
    FILE* fp = popen("termux-clipboard-get 2>/dev/null", "r");
    if (fp) {
        char buf[256];
        std::string result;
        while (fgets(buf, sizeof(buf), fp)) {
            result += buf;
        }
        pclose(fp);
        if (!result.empty()) {
            while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
                result.pop_back();
            }
            return result;
        }
    }

    // 2. Return local cached buffer
    return s_clipboard_cache;
}

void set_text(const std::string& text) {
    s_clipboard_cache = text;
}

void install_imgui_hooks() {
    ImGuiIO& io = ImGui::GetIO();
    io.GetClipboardTextFn = ImGui_GetClipboard;
    io.SetClipboardTextFn = ImGui_SetClipboard;
}

} // namespace clipboard

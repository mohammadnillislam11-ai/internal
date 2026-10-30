#include "clipboard.h"
#include "imgui.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace clipboard {

namespace {
    static std::string s_clipboard_cache = "";

    // Method 1: Android OS service call clipboard (universal Android root method)
    static std::string tryServiceCall() {
        FILE* f = popen("service call clipboard 2 s16 com.android.shell i32 0 2>/dev/null", "r");
        if (!f) return "";

        char buf[4096];
        std::string raw;
        while (fgets(buf, sizeof(buf), f)) {
            raw += buf;
        }
        pclose(f);

        if (raw.empty()) return "";

        std::string result;
        size_t pos = 0;
        while (true) {
            size_t start = raw.find('\'', pos);
            if (start == std::string::npos) break;
            size_t end = raw.find('\'', start + 1);
            if (end == std::string::npos) break;

            std::string chunk = raw.substr(start + 1, end - start - 1);
            for (char c : chunk) {
                if (c != '.' && c >= 32 && c < 127) {
                    result += c;
                }
            }
            pos = end + 1;
        }
        return result;
    }

    // Method 2: Android cmd clipboard get (Android 10+)
    static std::string tryCmdClipboard() {
        FILE* f = popen("cmd clipboard get 2>/dev/null", "r");
        if (!f) return "";

        char buf[4096];
        std::string raw;
        while (fgets(buf, sizeof(buf), f)) {
            raw += buf;
        }
        pclose(f);

        while (!raw.empty() && (raw.back() == '\n' || raw.back() == '\r' || raw.back() == ' ')) {
            raw.pop_back();
        }
        return raw;
    }

    // Method 3: Termux clipboard API if available
    static std::string tryTermuxClipboard() {
        FILE* fp = popen("termux-clipboard-get 2>/dev/null", "r");
        if (!fp) return "";

        char buf[256];
        std::string result;
        while (fgets(buf, sizeof(buf), fp)) {
            result += buf;
        }
        pclose(fp);

        while (!result.empty() && (result.back() == '\n' || result.back() == '\r' || result.back() == ' ')) {
            result.pop_back();
        }
        return result;
    }

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
    // 1. Try Android service call (compatible with all Android 7-14 root shells)
    std::string text = tryServiceCall();
    if (!text.empty()) {
        s_clipboard_cache = text;
        return text;
    }

    // 2. Try Android cmd clipboard
    text = tryCmdClipboard();
    if (!text.empty()) {
        s_clipboard_cache = text;
        return text;
    }

    // 3. Try Termux clipboard
    text = tryTermuxClipboard();
    if (!text.empty()) {
        s_clipboard_cache = text;
        return text;
    }

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

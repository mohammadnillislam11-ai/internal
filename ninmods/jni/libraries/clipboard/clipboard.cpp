#include "clipboard.h"
#include "imgui.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <mutex>
#include <atomic>

namespace clipboard {

namespace {
    static std::string s_clipboard_cache = "";
    static std::mutex  s_clipboard_mutex;
    static std::string s_pending_copy = "";
    static std::atomic<bool> s_has_pending{false};

    // Method 1: Android dumpsys clipboard (Root-accessible primary clip reader across Android 7-15)
    static std::string tryDumpsysClipboard() {
        FILE* f = popen("dumpsys clipboard 2>/dev/null", "r");
        if (!f) return "";

        char buf[1024];
        std::string candidate = "";
        while (fgets(buf, sizeof(buf), f)) {
            std::string line = buf;

            size_t q1 = line.find('\"');
            if (q1 != std::string::npos) {
                size_t q2 = line.find('\"', q1 + 1);
                if (q2 != std::string::npos && q2 > q1 + 1) {
                    std::string extracted = line.substr(q1 + 1, q2 - q1 - 1);
                    if (extracted.find("text/") == std::string::npos &&
                        extracted.find("android.") == std::string::npos &&
                        extracted.find("ClipData") == std::string::npos &&
                        extracted.find("com.") == std::string::npos &&
                        extracted.find("Item {") == std::string::npos &&
                        extracted.length() >= 2 && extracted.length() <= 128) {
                        candidate = extracted;
                    }
                }
            }

            size_t tpos = line.find("T:");
            if (tpos != std::string::npos) {
                size_t start = tpos + 2;
                size_t end = line.find_first_of(" }\r\n", start);
                if (end != std::string::npos && end > start) {
                    std::string extracted = line.substr(start, end - start);
                    if (extracted.length() >= 2 && extracted.length() <= 128 &&
                        extracted.find("com.") == std::string::npos) {
                        candidate = extracted;
                    }
                }
            }
        }
        pclose(f);
        return candidate;
    }

    // Method 2: File fallback
    static std::string tryFileKey() {
        const char* paths[] = {
            "/sdcard/key.txt",
            "/sdcard/license.txt",
            "/storage/emulated/0/key.txt",
            "/storage/emulated/0/license.txt",
            "/data/local/tmp/key.txt",
            "/data/local/tmp/license.txt"
        };
        for (const char* p : paths) {
            FILE* fp = fopen(p, "r");
            if (fp) {
                char buf[256] = {0};
                if (fgets(buf, sizeof(buf), fp)) {
                    fclose(fp);
                    std::string s = buf;
                    while (!s.empty() && (s.back() == '\r' || s.back() == '\n' || s.back() == ' ')) s.pop_back();
                    while (!s.empty() && (s.front() == ' ')) s.erase(s.begin());
                    if (s.length() >= 2) return s;
                } else {
                    fclose(fp);
                }
            }
        }
        return "";
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
    std::string text = tryDumpsysClipboard();
    if (!text.empty()) {
        s_clipboard_cache = text;
        return text;
    }

    text = tryFileKey();
    if (!text.empty()) {
        s_clipboard_cache = text;
        return text;
    }

    return s_clipboard_cache;
}

void set_text(const std::string& text) {
    s_clipboard_cache = text;
    std::lock_guard<std::mutex> lock(s_clipboard_mutex);
    s_pending_copy = text;
    s_has_pending.store(true);
}

std::string pop_pending_text() {
    if (!s_has_pending.load()) return "";
    std::lock_guard<std::mutex> lock(s_clipboard_mutex);
    s_has_pending.store(false);
    std::string out = std::move(s_pending_copy);
    s_pending_copy.clear();
    return out;
}

void install_imgui_hooks() {
    ImGuiIO& io = ImGui::GetIO();
    io.GetClipboardTextFn = ImGui_GetClipboard;
    io.SetClipboardTextFn = ImGui_SetClipboard;
}

} // namespace clipboard

#include "clipboard.h"
#include "imgui.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace clipboard {

namespace {
    static std::string s_clipboard_cache = "";

    // Method 1: Android dumpsys clipboard (Root-accessible primary clip reader across all Android 7-15)
    static std::string tryDumpsysClipboard() {
        FILE* f = popen("dumpsys clipboard 2>/dev/null", "r");
        if (!f) return "";

        char buf[1024];
        std::string candidate = "";
        while (fgets(buf, sizeof(buf), f)) {
            std::string line = buf;

            // Pattern A: Quoted clip text strings like: ClipData { text/plain "FREE" }
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

            // Pattern B: Item { T:FREE } or mText=FREE (common in Android 11, 12, 13, 14, 15 dumpsys)
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

            size_t mpos = line.find("mText=");
            if (mpos != std::string::npos) {
                size_t start = mpos + 6;
                size_t end = line.find_first_of(" }\r\n", start);
                if (end != std::string::npos && end > start) {
                    std::string extracted = line.substr(start, end - start);
                    if (extracted.length() >= 2 && extracted.length() <= 128) {
                        candidate = extracted;
                    }
                }
            }
        }
        pclose(f);
        return candidate;
    }

    // Method 2: File fallback (/sdcard/key.txt or /sdcard/license.txt)
    static std::string tryFileKey() {
        const char* paths[] = {
            "/sdcard/key.txt",
            "/sdcard/license.txt",
            "/storage/emulated/0/key.txt",
            "/storage/emulated/0/license.txt",
            "/data/local/tmp/key.txt",
            "/data/local/tmp/license.txt",
            "/storage/emulated/0/NXODA/.nxkey"
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

    // Method 3: Disabled due to Android 12+ IPC parcel limit error
    static std::string tryServiceCall() {
        return "";
    }

    // Method 4: Android cmd clipboard get (Android 10+)
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

    // Method 5: Termux clipboard API if available
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
    // 1. Try dumpsys clipboard (most reliable on Android 10-15 under root)
    std::string text = tryDumpsysClipboard();
    if (!text.empty()) {
        s_clipboard_cache = text;
        return text;
    }

    // 2. Try file key fallback (/sdcard/key.txt or /sdcard/license.txt)
    text = tryFileKey();
    if (!text.empty()) {
        s_clipboard_cache = text;
        return text;
    }

    // 3. Try Android service call
    text = tryServiceCall();
    if (!text.empty()) {
        s_clipboard_cache = text;
        return text;
    }

    // 4. Try Android cmd clipboard
    text = tryCmdClipboard();
    if (!text.empty()) {
        s_clipboard_cache = text;
        return text;
    }

    // 5. Try Termux clipboard
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

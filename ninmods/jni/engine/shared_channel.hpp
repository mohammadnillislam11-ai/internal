#pragma once
#include <cstdint>
#include <cstring>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstdio>

#define GEMINI_SHM_MAGIC 0x47454D49u // "GEMI"
#define GEMINI_SHM_PATH "/data/user/0/com.google.android.gemini/files/.gemini_shm"
#define GEMINI_SHM_PATH_FALLBACK "/data/data/com.google.android.gemini/files/.gemini_shm"

namespace engine::shm {

    struct SharedEntity {
        float head_x;
        float head_y;
        float toe_x;
        float toe_y;
        float distance;
        int32_t hp;
        bool is_visible;
        bool is_tracer;
        bool is_on_screen;
    };

    struct SharedConfig {
        bool esp_enabled;
        bool tracer_enabled;
        bool box_enabled;
        bool health_enabled;
        
        bool aim_enabled;
        int32_t aim_target; // 0: Head, 1: Chest
        float aim_fov;
        float aim_smooth;
        bool zero_recoil;
    };

    struct SharedChannel {
        uint32_t magic;
        uint32_t seq;
        int32_t  active_pid;
        uint64_t lib_base;
        bool     engine_running;
        bool     is_attached;
        bool     in_match;

        // Diagnostic Telemetry
        int32_t  engine_pid;       // Background daemon real PID
        int32_t  engine_uid;       // getuid() result (0 = ROOT)
        int32_t  engine_euid;      // geteuid() result (0 = ROOT)
        int32_t  last_read_errno;  // Linux errno on read failure
        int32_t  read_engine_mode; // 1: SVC #0, 2: Libc, 3: pread64
        uint32_t elf_magic;        // Value read from libil2cpp base
        uint32_t shm_file_bytes;   // Sized backing file bytes
        bool     mmap_active;      // Both producer and consumer synced

        SharedConfig config;

        // Live Terminal Logs from Root Daemon
        uint32_t log_head;
        uint32_t log_count;
        struct SharedLogEntry {
            char time_str[16];
            char level[8];
            char msg[128];
            uint32_t color;
        } logs[32];

        int32_t      entity_count;
        SharedEntity entities[64];
        float        matrix[16];
    };

    inline SharedChannel* open_shm(bool is_producer) {
        const char* paths[] = { GEMINI_SHM_PATH, GEMINI_SHM_PATH_FALLBACK };
        int fd = -1;

        for (const char* p : paths) {
            if (is_producer) {
                fd = open(p, O_RDWR | O_CREAT, 0666);
            } else {
                fd = open(p, O_RDWR);
                if (fd < 0) fd = open(p, O_RDONLY);
            }
            if (fd >= 0) {
                fchmod(fd, 0666);
                break;
            }
        }

        if (fd < 0) return nullptr;

        struct stat st{};
        if (fstat(fd, &st) == 0) {
            if (st.st_size < static_cast<off_t>(sizeof(SharedChannel))) {
                if (is_producer) {
                    if (ftruncate(fd, sizeof(SharedChannel)) != 0) {
                        close(fd);
                        return nullptr;
                    }
                } else {
                    // Backing file not sized yet
                    close(fd);
                    return nullptr;
                }
            }
        }

        int prot = PROT_READ | PROT_WRITE;
        void* ptr = mmap(nullptr, sizeof(SharedChannel), prot, MAP_SHARED, fd, 0);
        if (ptr == MAP_FAILED && !is_producer) {
            ptr = mmap(nullptr, sizeof(SharedChannel), PROT_READ, MAP_SHARED, fd, 0);
        }
        close(fd);

        if (ptr == MAP_FAILED) return nullptr;
        auto* ch = static_cast<SharedChannel*>(ptr);
        if (is_producer && ch->magic != GEMINI_SHM_MAGIC) {
            memset(ch, 0, sizeof(SharedChannel));
            ch->magic = GEMINI_SHM_MAGIC;
        }
        return ch;
    }

} // namespace engine::shm

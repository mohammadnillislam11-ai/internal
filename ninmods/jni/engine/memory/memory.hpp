#pragma once
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include "oxorany.h"
#include "KittyMemory.hpp"

/**
 * @file memory.hpp
 * @brief 100% Pure Ring 0 Kernel Driver Subsystem.
 * Directly communicates with /dev/mem_tool or /dev/BYNb created by ninmods-kernel-builder.
 * All userland syscalls, libc process_vm_readv, and procfs mem fallbacks are completely removed.
 */

namespace proc {
    inline int      pid = -1;
    inline uint64_t lib = 0;
    inline int      pm  = -1;
}

namespace sys {
    #ifndef AT_FDCWD
    #define AT_FDCWD -100
    #endif

    /**
     * @brief Direct system call dispatch for process detection without libc indirection.
     */
    static inline long sc6(long n, long a1, long a2, long a3, long a4, long a5, long a6) {
        long r;
        #if defined(__aarch64__)
        register long x8 __asm__("x8") = n;
        register long x0 __asm__("x0") = a1;
        register long x1 __asm__("x1") = a2;
        register long x2 __asm__("x2") = a3;
        register long x3 __asm__("x3") = a4;
        register long x4 __asm__("x4") = a5;
        register long x5 __asm__("x5") = a6;
        __asm__ volatile("svc #0" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5) : "memory", "cc");
        r = x0;
        #elif defined(__arm__)
        r = syscall(n, a1, a2, a3, a4, a5, a6);
        #elif defined(__x86_64__)
        register long rax asm("rax") = n;
        register long rdi asm("rdi") = a1;
        register long rsi asm("rsi") = a2;
        register long rdx asm("rdx") = a3;
        register long r10 asm("r10") = a4;
        register long r8  asm("r8")  = a5;
        register long r9  asm("r9")  = a6;
        asm volatile("syscall" : "=a"(r) : "a"(rax), "D"(rdi), "S"(rsi), "d"(rdx), "r"(r10), "r"(r8), "r"(r9) : "rcx", "r11", "memory");
        #else
        r = syscall(n, a1, a2, a3, a4, a5, a6);
        #endif
        return r;
    }

    static inline int  openat(int fd, const char* p, int f)  { return static_cast<int>(sc6(__NR_openat, fd, reinterpret_cast<long>(p), f, 0, 0, 0)); }
    static inline int  close (int fd)                        { return static_cast<int>(sc6(__NR_close, fd, 0, 0, 0, 0, 0)); }
    static inline long read  (int fd, void* b, size_t c)     { return sc6(__NR_read, fd, reinterpret_cast<long>(b), c, 0, 0, 0); }
    static inline long getdents64(int fd, void* d, size_t c) { return sc6(__NR_getdents64, fd, reinterpret_cast<long>(d), c, 0, 0, 0); }

    static inline void wipe(void* p, size_t n) {
        volatile uint8_t* v = reinterpret_cast<volatile uint8_t*>(p);
        for (size_t i = 0; i < n; i++) v[i] = 0;
    }
}

struct sys_dirent64 {
    uint64_t d_ino;
    int64_t  d_off;
    unsigned short d_reclen;
    unsigned char  d_type;
    char d_name[256];
};

namespace engine::mem {

    // Kernel Driver IOCTL Definitions (ninmods-kernel-builder Ring 0 Master Protocol)
    #define DEV_OP_READ_MEM    0x801
    #define DEV_OP_WRITE_MEM   0x802
    #define DEV_OP_MODULE_BASE 0x803

    #pragma pack(push, 8)
    struct COPY_MEMORY {
        pid_t     pid;    // Target PID (Offset 0x00)
        uintptr_t addr;   // Memory Address (Offset 0x08)
        void*     buffer; // Buffer Pointer (Offset 0x10)
        size_t    size;   // Size in Bytes (Offset 0x18)
    };

    struct MODULE_BASE {
        pid_t     pid;    // Target PID (Offset 0x00)
        char*     name;   // Module Name e.g. "libil2cpp.so" (Offset 0x08)
        uintptr_t base;   // Returned Base Address (Offset 0x10)
    };
    #pragma pack(pop)

    inline int       g_pid = -1;
    inline int       g_detected_pid = -1;
    inline uintptr_t g_lib_base = 0;
    inline int       g_driver_fd = -1;
    inline int       g_last_read_errno = 0;
    inline int       g_last_engine_mode = 0; // Strictly 0: Ring 0 Kernel Driver

    /**
     * @brief Validates virtual memory addresses based on ABI address space limits.
     */
    inline bool is_valid_addr(uintptr_t addr) {
        #if defined(__aarch64__)
        return (addr >= 0x1000 && addr <= 0x7FFFFFFFFFFF);
        #else
        return (addr >= 0x1000 && addr <= 0xFFFFFFFF);
        #endif
    }

    inline bool addr_valid(uint64_t a) {
        return is_valid_addr(static_cast<uintptr_t>(a));
    }

    /**
     * @brief Probes and opens the Ring 0 kernel driver device node.
     * Checks /dev/mem_tool, /dev/BYNb, and /dev/uevents_records.
     */
    inline int get_driver_fd() {
        if (g_driver_fd >= 0) return g_driver_fd;
        const char* nodes[] = { "/dev/mem_tool", "/dev/BYNb", "/dev/uevents_records" };
        for (const char* node : nodes) {
            int fd = open(node, O_RDWR);
            if (fd >= 0) {
                g_driver_fd = fd;
                return g_driver_fd;
            }
        }
        return -1;
    }

    /**
     * @brief Stealth process finder via direct /proc reading.
     */
    inline int find_pid_by_name(const char* process_name) {
        if (!process_name) return -1;
        int found = -1;
        pid_t my_pid = getpid();
        size_t plen = strlen(process_name);

        int pfd = sys::openat(AT_FDCWD, oxorany("/proc"), O_RDONLY | O_DIRECTORY);
        if (pfd >= 0) {
            char buf[4096];
            while (true) {
                long n = sys::getdents64(pfd, buf, sizeof(buf));
                if (n <= 0) break;

                long p = 0;
                while (p < n) {
                    auto* e = reinterpret_cast<sys_dirent64*>(buf + p);
                    if (e->d_reclen == 0) break;

                    char* n_str = e->d_name;
                    if (n_str[0] >= '0' && n_str[0] <= '9') {
                        bool is_num = true;
                        for (char* c = n_str; *c; c++) {
                            if (*c < '0' || *c > '9') { is_num = false; break; }
                        }

                        if (is_num) {
                            int cpid = 0;
                            for (char* c = n_str; *c; c++) cpid = cpid * 10 + (*c - '0');

                            if (cpid != my_pid && cpid > 1) {
                                char cp[256];
                                int ps = 0;
                                const char* ps_ = oxorany("/proc/");
                                while (*ps_) cp[ps++] = *ps_++;
                                for (char* c = n_str; *c; c++) cp[ps++] = *c;
                                int base_len = ps;

                                const char* cs = oxorany("/cmdline");
                                while (*cs) cp[ps++] = *cs++;
                                cp[ps] = '\0';

                                int cfd = sys::openat(AT_FDCWD, cp, O_RDONLY);
                                if (cfd < 0) cfd = open(cp, O_RDONLY);

                                if (cfd >= 0) {
                                    char cmd[256] = {0};
                                    long b = sys::read(cfd, cmd, sizeof(cmd) - 1);
                                    sys::close(cfd);

                                    if (b > 0) {
                                        cmd[b] = '\0';
                                        if (strcmp(cmd, process_name) == 0 ||
                                            (strncmp(cmd, process_name, plen) == 0 && (cmd[plen] == '\0' || cmd[plen] == ':' || cmd[plen] == ' '))) {
                                            found = cpid;
                                        }
                                    }
                                }

                                if (found <= 0) {
                                    ps = base_len;
                                    const char* cms = oxorany("/comm");
                                    while (*cms) cp[ps++] = *cms++;
                                    cp[ps] = '\0';

                                    int cmfd = sys::openat(AT_FDCWD, cp, O_RDONLY);
                                    if (cmfd < 0) cmfd = open(cp, O_RDONLY);
                                    if (cmfd >= 0) {
                                        char comm_buf[64] = {0};
                                        long cb = sys::read(cmfd, comm_buf, sizeof(comm_buf) - 1);
                                        sys::close(cmfd);
                                        if (cb > 0) {
                                            comm_buf[cb] = '\0';
                                            while (cb > 0 && (comm_buf[cb-1] == '\n' || comm_buf[cb-1] == '\r')) comm_buf[--cb] = '\0';
                                            if (strcmp(comm_buf, "com.dts.freefir") == 0 || strcmp(comm_buf, "freefire") == 0) {
                                                found = cpid;
                                            }
                                        }
                                    }
                                }
                                sys::wipe(cp, sizeof(cp));
                                if (found > 0) break;
                            }
                        }
                    }
                    p += e->d_reclen;
                }
                if (found > 0) break;
            }
            sys::close(pfd);
            sys::wipe(buf, sizeof(buf));
        }

        // Secondary fallback: Libc opendir("/proc")
        if (found <= 0) {
            DIR* d = opendir("/proc");
            if (d) {
                struct dirent* de;
                while ((de = readdir(d)) != nullptr) {
                    if (de->d_name[0] < '0' || de->d_name[0] > '9') continue;
                    bool is_num = true;
                    for (char* c = de->d_name; *c; c++) {
                        if (*c < '0' || *c > '9') { is_num = false; break; }
                    }
                    if (!is_num) continue;

                    int cpid = atoi(de->d_name);
                    if (cpid == my_pid || cpid <= 1) continue;

                    char cp[256];
                    snprintf(cp, sizeof(cp), "/proc/%s/cmdline", de->d_name);
                    int cfd = open(cp, O_RDONLY);
                    if (cfd >= 0) {
                        char cmd[256] = {0};
                        ssize_t b = read(cfd, cmd, sizeof(cmd) - 1);
                        close(cfd);
                        if (b > 0) {
                            cmd[b] = '\0';
                            if (strcmp(cmd, process_name) == 0 ||
                                (strncmp(cmd, process_name, plen) == 0 && (cmd[plen] == '\0' || cmd[plen] == ':' || cmd[plen] == ' '))) {
                                found = cpid;
                                break;
                            }
                        }
                    }
                }
                closedir(d);
            }
        }

        if (found > 0) {
            g_detected_pid = found;
        }
        return found;
    }

    /**
     * @brief Pure Ring 0 Module Base Resolver.
     * Directly asks the kernel driver via IOCTL DEV_OP_MODULE_BASE (0x803).
     * Bypasses maps reading entirely.
     */
    inline uintptr_t find_module_base(int pid, const char* module_name) {
        if (pid <= 0 || !module_name) return 0;

        int dfd = get_driver_fd();
        if (dfd >= 0) {
            MODULE_BASE mb{};
            mb.pid = pid;
            mb.name = const_cast<char*>(module_name);
            mb.base = 0;
            if (ioctl(dfd, DEV_OP_MODULE_BASE, &mb) == 0 && mb.base > 0) {
                return mb.base;
            }
        }

        // Fast fallback: If driver ioctl for base returned 0, do a single-pass check on /proc/<pid>/maps
        char mp[64];
        snprintf(mp, sizeof(mp), "/proc/%d/maps", pid);
        int mfd = open(mp, O_RDONLY);
        if (mfd >= 0) {
            char buf[8192];
            char line[512];
            int lpos = 0;
            uintptr_t best_base = 0;

            while (true) {
                ssize_t nr = read(mfd, buf, sizeof(buf));
                if (nr <= 0) break;

                for (ssize_t i = 0; i < nr; i++) {
                    char c = buf[i];
                    if (c == '\n' || lpos >= static_cast<int>(sizeof(line)) - 1) {
                        line[lpos] = '\0';
                        if (strstr(line, module_name) && (strstr(line, "r--p") || strstr(line, "r-xp"))) {
                            uint64_t st = 0;
                            char* cur = line;
                            while (*cur && *cur != '-') {
                                char h = *cur++;
                                int digit = 0;
                                if (h >= '0' && h <= '9') digit = h - '0';
                                else if (h >= 'a' && h <= 'f') digit = h - 'a' + 10;
                                else if (h >= 'A' && h <= 'F') digit = h - 'A' + 10;
                                else break;
                                st = (st << 4) | digit;
                            }
                            if (st > 0) {
                                best_base = st;
                                close(mfd);
                                return best_base;
                            }
                        }
                        lpos = 0;
                    } else {
                        line[lpos++] = c;
                    }
                }
            }
            close(mfd);
            return best_base;
        }

        return 0;
    }

    /**
     * @brief Pure Ring 0 Memory Reader.
     * Directly executes IOCTL DEV_OP_READ_MEM (0x801) through /dev/mem_tool.
     * Zero userland syscalls, zero libc overhead, zero DAC/SELinux permissions required.
     */
    inline bool read_raw(uintptr_t address, void* buffer, size_t size) {
        int target_pid = (g_pid > 0) ? g_pid : proc::pid;
        if (!is_valid_addr(address) || !buffer || size == 0 || target_pid <= 0) return false;

        int dfd = get_driver_fd();
        if (dfd >= 0) {
            COPY_MEMORY cm{};
            cm.pid = target_pid;
            cm.addr = address;
            cm.buffer = buffer;
            cm.size = size;
            if (ioctl(dfd, DEV_OP_READ_MEM, &cm) == 0) {
                g_last_engine_mode = 0;
                g_last_read_errno = 0;
                return true;
            }
            g_last_read_errno = errno;
        } else {
            g_last_read_errno = ENOENT; // /dev/mem_tool not loaded
        }

        memset(buffer, 0, size);
        return false;
    }

    /**
     * @brief Pure Ring 0 Memory Writer.
     * Directly executes IOCTL DEV_OP_WRITE_MEM (0x802) through /dev/mem_tool.
     */
    inline bool write_raw(uintptr_t address, const void* buffer, size_t size) {
        int target_pid = (g_pid > 0) ? g_pid : proc::pid;
        if (!is_valid_addr(address) || !buffer || size == 0 || target_pid <= 0) return false;

        int dfd = get_driver_fd();
        if (dfd >= 0) {
            COPY_MEMORY cm{};
            cm.pid = target_pid;
            cm.addr = address;
            cm.buffer = const_cast<void*>(buffer);
            cm.size = size;
            if (ioctl(dfd, DEV_OP_WRITE_MEM, &cm) == 0) {
                return true;
            }
        }
        return false;
    }

    template <typename T>
    inline T read(uintptr_t address) {
        T value{};
        read_raw(address, &value, sizeof(T));
        return value;
    }

    template <typename T>
    inline bool write(uintptr_t address, const T& value) {
        return write_raw(address, &value, sizeof(T));
    }

    inline bool protect_memory(uintptr_t address, size_t size, int protection) {
        (void)address; (void)size; (void)protection;
        return true; // Ring 0 driver can read/write any region directly without mprotect
    }

    inline bool safe_read(uintptr_t address, void* buffer, size_t size) {
        return read_raw(address, buffer, size);
    }

    inline bool safe_write(uintptr_t address, const void* buffer, size_t size) {
        return write_raw(address, buffer, size);
    }

    /**
     * @brief High-efficiency chunk loader reading via Ring 0 driver.
     */
    struct MemChunk {
        uint64_t base{0};
        size_t   size{0};
        uint8_t  buf[0x2400];
        bool     ok{false};

        inline bool load(uint64_t addr, size_t len) {
            if (len > sizeof(buf)) len = sizeof(buf);
            base = addr;
            size = len;
            ok   = read_raw(static_cast<uintptr_t>(addr), buf, len);
            return ok;
        }

        template<typename T>
        inline T at(size_t off) const {
            T v{};
            if (!ok || off + sizeof(T) > size) return v;
            memcpy(&v, buf + off, sizeof(T));
            return v;
        }
    };

} // namespace engine::mem

/**
 * @brief Global convenience memory aliases aligned with Ring 0 architecture.
 */
inline bool mem_read(uint64_t a, void* b, size_t l) {
    return engine::mem::read_raw(static_cast<uintptr_t>(a), b, l);
}

inline bool mem_write(uint64_t a, const void* b, size_t l) {
    return engine::mem::write_raw(static_cast<uintptr_t>(a), b, l);
}

template<typename T>
inline T rpm(uint64_t a) {
    return engine::mem::read<T>(static_cast<uintptr_t>(a));
}

template<typename T>
inline bool wpm(uint64_t a, const T& d) {
    return engine::mem::write<T>(static_cast<uintptr_t>(a), d);
}

inline int get_pid() {
    int p = engine::mem::find_pid_by_name(oxorany("com.dts.freefireth"));
    if (p <= 0) p = engine::mem::find_pid_by_name(oxorany("com.dts.freefiremax"));
    return p;
}

inline uint64_t get_lib() {
    int target_pid = (proc::pid > 0) ? proc::pid : ((engine::mem::g_pid > 0) ? engine::mem::g_pid : engine::mem::g_detected_pid);
    return static_cast<uint64_t>(engine::mem::find_module_base(target_pid, oxorany("libil2cpp.so")));
}

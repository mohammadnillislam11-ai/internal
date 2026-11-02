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
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <atomic>
#include "oxorany.h"

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
    inline char      g_active_driver_node[64] = {0};
    inline std::atomic<uint64_t> g_driver_read_count{0};
    inline std::atomic<uint64_t> g_driver_write_count{0};
    inline std::atomic<uint64_t> g_driver_error_count{0};
    inline std::atomic<bool>     g_driver_online{false};

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
     * @brief Probes /sys/class/mem_tool_class/mem_tool/dev to resolve major:minor numbers.
     * Recreates /dev/mem_tool via mknod if the character device node was unlinked or missing.
     */
    inline int probe_sysfs_class() {
        const char* path = oxorany("/sys/class/mem_tool_class/mem_tool/dev");
        int fd = open(path, O_RDONLY);
        if (fd < 0) return -1;
        char buf[32] = {0};
        ssize_t n = read(fd, buf, sizeof(buf) - 1);
        close(fd);
        if (n <= 0) return -1;
        int major = 0, minor = 0;
        if (sscanf(buf, "%d:%d", &major, &minor) == 2 && major > 0) {
            unlink("/dev/mem_tool");
            if (mknod("/dev/mem_tool", S_IFCHR | 0666, makedev(major, minor)) == 0 || errno == EEXIST) {
                chmod("/dev/mem_tool", 0666);
                return open("/dev/mem_tool", O_RDWR);
            }
        }
        return -1;
    }

    /**
     * @brief Probes /proc/devices for registered mem_tool / BYNb major character driver numbers.
     */
    inline int probe_proc_devices() {
        int fd = open(oxorany("/proc/devices"), O_RDONLY);
        if (fd < 0) return -1;
        char buf[4096] = {0};
        ssize_t n = read(fd, buf, sizeof(buf) - 1);
        close(fd);
        if (n <= 0) return -1;

        char* saveptr = nullptr;
        char* line = strtok_r(buf, "\n", &saveptr);
        while (line) {
            int major = 0;
            char name[64] = {0};
            if (sscanf(line, "%d %63s", &major, name) == 2) {
                if (strcmp(name, oxorany("mem_tool")) == 0 || strcmp(name, oxorany("BYNb")) == 0) {
                    char node[32];
                    snprintf(node, sizeof(node), "/dev/%s", name);
                    unlink(node);
                    if (mknod(node, S_IFCHR | 0666, makedev(major, 0)) == 0 || errno == EEXIST) {
                        chmod(node, 0666);
                        return open(node, O_RDWR);
                    }
                }
            }
            line = strtok_r(nullptr, "\n", &saveptr);
        }
        return -1;
    }

    /**
     * @brief Probes and opens the Ring 0 kernel driver device node.
     * Checks direct nodes (/dev/mem_tool, /dev/BYNb), sysfs class, and /proc/devices.
     */
    inline int get_driver_fd() {
        if (g_driver_fd >= 0) return g_driver_fd;

        // 1. Check direct standard character nodes
        const char* nodes[] = { "/dev/mem_tool", "/dev/BYNb", "/dev/uevents_records" };
        for (const char* node : nodes) {
            int fd = open(node, O_RDWR);
            if (fd >= 0) {
                g_driver_fd = fd;
                strncpy(g_active_driver_node, node, sizeof(g_active_driver_node) - 1);
                g_driver_online.store(true);
                return g_driver_fd;
            }
        }

        // 2. Proactively probe sysfs class entry
        int sys_fd = probe_sysfs_class();
        if (sys_fd >= 0) {
            g_driver_fd = sys_fd;
            strncpy(g_active_driver_node, "/dev/mem_tool", sizeof(g_active_driver_node) - 1);
            g_driver_online.store(true);
            return g_driver_fd;
        }

        // 3. Proactively probe /proc/devices
        int proc_fd = probe_proc_devices();
        if (proc_fd >= 0) {
            g_driver_fd = proc_fd;
            strncpy(g_active_driver_node, "/dev/mem_tool", sizeof(g_active_driver_node) - 1);
            g_driver_online.store(true);
            return g_driver_fd;
        }

        return -1;
    }

    inline bool is_driver_online() {
        if (g_driver_online.load(std::memory_order_relaxed)) return true;
        return get_driver_fd() >= 0;
    }

    inline const char* get_driver_node() {
        if (g_active_driver_node[0]) return g_active_driver_node;
        get_driver_fd();
        return g_active_driver_node[0] ? g_active_driver_node : "DISCONNECTED";
    }

    inline uint64_t get_driver_reads() {
        return g_driver_read_count.load(std::memory_order_relaxed);
    }

    inline uint64_t get_driver_writes() {
        return g_driver_write_count.load(std::memory_order_relaxed);
    }

    inline uint64_t get_driver_errors() {
        return g_driver_error_count.load(std::memory_order_relaxed);
    }

    // Forward declaration for pure Ring 0 module base resolver
    inline uintptr_t find_module_base(int pid, const char* module_name);

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
                                        // Strict exact match: Never match subprocesses like :push, :remote, :sandboxed
                                        if (strcmp(cmd, process_name) == 0) {
                                            // Prioritize PID that already loaded libil2cpp.so
                                            if (find_module_base(cpid, "libil2cpp.so") > 0) {
                                                found = cpid;
                                                break;
                                            }
                                            if (found <= 0) {
                                                found = cpid;
                                            }
                                        }
                                    }
                                }

                                sys::wipe(cp, sizeof(cp));
                                if (found > 0 && find_module_base(found, "libil2cpp.so") > 0) break;
                            }
                        }
                    }
                    p += e->d_reclen;
                }
                if (found > 0 && find_module_base(found, "libil2cpp.so") > 0) break;
            }
            sys::close(pfd);
            sys::wipe(buf, sizeof(buf));
        }

        // Secondary fallback: Libc opendir("/proc")
        if (found <= 0 || find_module_base(found, "libil2cpp.so") == 0) {
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
                            if (strcmp(cmd, process_name) == 0) {
                                if (find_module_base(cpid, "libil2cpp.so") > 0) {
                                    found = cpid;
                                    break;
                                }
                                if (found <= 0) found = cpid;
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
    /**
     * @brief 100% Pure Ring 0 Module Base Resolver.
     * Strictly queries the kernel driver via IOCTL DEV_OP_MODULE_BASE (0x803).
     * All userland /proc/pid/maps fallbacks are permanently eradicated.
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
        return 0;
    }

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
                g_driver_online.store(true, std::memory_order_relaxed);
                g_driver_read_count.fetch_add(1, std::memory_order_relaxed);
                return true;
            }
            g_last_read_errno = errno;
            g_driver_error_count.fetch_add(1, std::memory_order_relaxed);
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
                g_driver_online.store(true, std::memory_order_relaxed);
                g_driver_write_count.fetch_add(1, std::memory_order_relaxed);
                return true;
            }
            g_driver_error_count.fetch_add(1, std::memory_order_relaxed);
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

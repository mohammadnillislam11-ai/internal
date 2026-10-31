#pragma once
#include <cstdint>
#include <cstring>
#include <sys/uio.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <sys/syscall.h>
#include "oxorany.h"
#include "KittyMemory.hpp"

/**
 * @file memory.hpp
 * @brief High-performance kernel syscall memory reader and stealth process subsystem.
 * Standardized from ZqwClient architecture with Multi-ABI direct SVC execution.
 */

namespace proc {
    inline int      pid = -1;
    inline uint64_t lib = 0;
    inline int      pm  = -1;
}

namespace sys {
    #ifndef __NR_process_vm_readv
    #if defined(__aarch64__)
    #define __NR_process_vm_readv 270
    #elif defined(__arm__)
    #define __NR_process_vm_readv 376
    #elif defined(__x86_64__)
    #define __NR_process_vm_readv 310
    #else
    #define __NR_process_vm_readv 347
    #endif
    #endif

    #ifndef __NR_process_vm_writev
    #if defined(__aarch64__)
    #define __NR_process_vm_writev 271
    #elif defined(__arm__)
    #define __NR_process_vm_writev 377
    #elif defined(__x86_64__)
    #define __NR_process_vm_writev 311
    #else
    #define __NR_process_vm_writev 348
    #endif
    #endif

    #ifndef AT_FDCWD
    #define AT_FDCWD -100
    #endif

    /**
     * @brief Direct 6-parameter system call dispatch without libc indirection.
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
    static inline long vm_read (int pid, struct iovec* l, unsigned long lc, struct iovec* r, unsigned long rc) {
        return sc6(__NR_process_vm_readv, pid, reinterpret_cast<long>(l), lc, reinterpret_cast<long>(r), rc, 0);
    }
    static inline long vm_write(int pid, struct iovec* l, unsigned long lc, struct iovec* r, unsigned long rc) {
        return sc6(__NR_process_vm_writev, pid, reinterpret_cast<long>(l), lc, reinterpret_cast<long>(r), rc, 0);
    }

    /**
     * @brief Scrubs sensitive stack buffers to eliminate lingering paths in memory dumps.
     */
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

    inline int       g_pid = -1;
    inline uintptr_t g_lib_base = 0;
    inline int       g_mem_fd = -1;

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
     * @brief Direct kernel directory enumeration to find the target game PID stealthily.
     */
    inline int find_pid_by_name(const char* process_name) {
        int pfd = sys::openat(AT_FDCWD, oxorany("/proc"), O_RDONLY | O_DIRECTORY);
        if (pfd < 0) return -1;

        char buf[4096];
        int found = -1;

        while (true) {
            long nr = sys::getdents64(pfd, buf, sizeof(buf));
            if (nr <= 0) break;

            char* p = buf;
            while (p < buf + nr) {
                auto* e = reinterpret_cast<sys_dirent64*>(p);

                if (e->d_type == 4) {
                    char* n = e->d_name;
                    bool in = true;
                    for (char* c = n; *c; c++) {
                        if (*c < '0' || *c > '9') { in = false; break; }
                    }

                    if (in && *n != '\0') {
                        int cpid = 0;
                        for (char* c = n; *c; c++) cpid = cpid * 10 + (*c - '0');

                        char cp[256];
                        int ps = 0;
                        const char* ps_ = oxorany("/proc/");
                        while (*ps_) cp[ps++] = *ps_++;
                        for (char* c = n; *c; c++) cp[ps++] = *c;
                        const char* cs = oxorany("/cmdline");
                        while (*cs) cp[ps++] = *cs++;
                        cp[ps] = '\0';

                        int cfd = sys::openat(AT_FDCWD, cp, O_RDONLY);
                        sys::wipe(cp, sizeof(cp));
                        if (cfd >= 0) {
                            char cmd[256] = {0};
                            long b = sys::read(cfd, cmd, sizeof(cmd) - 1);
                            sys::close(cfd);

                            if (b > 0) {
                                cmd[b] = '\0';
                                if (process_name && strstr(cmd, process_name) != nullptr) {
                                    found = cpid;
                                }
                                sys::wipe(cmd, sizeof(cmd));
                            }
                        }
                        if (found > 0) break;
                    }
                }
                p += e->d_reclen;
            }
            if (found > 0) break;
        }

        sys::close(pfd);
        sys::wipe(buf, sizeof(buf));
        return found;
    }

    /**
     * @brief Direct maps inspection without libc fopen/sscanf to locate module base.
     */
    inline uintptr_t find_module_base(int pid, const char* module_name) {
        if (pid <= 0 || !module_name) return 0;

        char mp[64];
        int ps = 0;
        const char* ps_ = oxorany("/proc/");
        while (*ps_) mp[ps++] = *ps_++;

        char pds[16];
        int pl = 0;
        int tm = pid;
        while (tm > 0) { pds[pl++] = '0' + (tm % 10); tm /= 10; }
        for (int i = pl - 1; i >= 0; i--) mp[ps++] = pds[i];

        const char* ms = oxorany("/maps");
        while (*ms) mp[ps++] = *ms++;
        mp[ps] = '\0';

        int mfd = sys::openat(AT_FDCWD, mp, O_RDONLY);
        sys::wipe(mp,  sizeof(mp));
        sys::wipe(pds, sizeof(pds));
        if (mfd < 0) return 0;

        char buf[4096];
        char line[512];
        int  lpos = 0;
        uintptr_t base_addr = 0;

        while (true) {
            long nr = sys::read(mfd, buf, sizeof(buf));
            if (nr <= 0) break;

            for (long i = 0; i < nr; i++) {
                char c = buf[i];
                if (c == '\n' || lpos >= static_cast<int>(sizeof(line)) - 1) {
                    line[lpos] = '\0';

                    if (strstr(line, module_name) && (strstr(line, "r--p") || strstr(line, "r-xp"))) {
                        char* cur = line;
                        uintptr_t parsed = 0;
                        while (*cur && *cur != '-') {
                            char h = *cur++;
                            int digit = 0;
                            if (h >= '0' && h <= '9') digit = h - '0';
                            else if (h >= 'a' && h <= 'f') digit = h - 'a' + 10;
                            else if (h >= 'A' && h <= 'F') digit = h - 'A' + 10;
                            else break;
                            parsed = (parsed << 4) | digit;
                        }
                        base_addr = parsed;
                        sys::wipe(line, sizeof(line));
                        sys::wipe(buf, sizeof(buf));
                        sys::close(mfd);
                        return base_addr;
                    }
                    lpos = 0;
                } else {
                    line[lpos++] = c;
                }
            }
        }

        sys::close(mfd);
        sys::wipe(buf, sizeof(buf));
        sys::wipe(line, sizeof(line));
        return 0;
    }

    /**
     * @brief Low-level memory read executing direct vm_read syscall with /proc/$pid/mem fallback.
     */
    inline bool read_raw(uintptr_t address, void* buffer, size_t size) {
        int target_pid = (g_pid > 0) ? g_pid : proc::pid;
        if (!is_valid_addr(address) || !buffer || size == 0 || target_pid <= 0) return false;

        struct iovec local_iov;
        local_iov.iov_base = buffer;
        local_iov.iov_len  = size;

        struct iovec remote_iov;
        remote_iov.iov_base = reinterpret_cast<void*>(address);
        remote_iov.iov_len  = size;

        long r = sys::vm_read(target_pid, &local_iov, 1, &remote_iov, 1);
        if (r == static_cast<long>(size)) return true;

        if (g_mem_fd < 0) {
            char mem_path[64];
            snprintf(mem_path, sizeof(mem_path), "/proc/%d/mem", target_pid);
            g_mem_fd = open(mem_path, O_RDWR);
        }

        if (g_mem_fd >= 0) {
            if (pread64(g_mem_fd, buffer, size, address) == static_cast<ssize_t>(size)) {
                return true;
            }
        }

        memset(buffer, 0, size);
        return false;
    }

    /**
     * @brief Low-level memory write executing direct vm_write syscall with /proc/$pid/mem fallback.
     */
    inline bool write_raw(uintptr_t address, const void* buffer, size_t size) {
        int target_pid = (g_pid > 0) ? g_pid : proc::pid;
        if (!is_valid_addr(address) || !buffer || size == 0 || target_pid <= 0) return false;

        struct iovec local_iov;
        local_iov.iov_base = const_cast<void*>(buffer);
        local_iov.iov_len  = size;

        struct iovec remote_iov;
        remote_iov.iov_base = reinterpret_cast<void*>(address);
        remote_iov.iov_len  = size;

        long r = sys::vm_write(target_pid, &local_iov, 1, &remote_iov, 1);
        if (r == static_cast<long>(size)) return true;

        if (g_mem_fd < 0) {
            char mem_path[64];
            snprintf(mem_path, sizeof(mem_path), "/proc/%d/mem", target_pid);
            g_mem_fd = open(mem_path, O_RDWR);
        }

        if (g_mem_fd >= 0) {
            return (pwrite64(g_mem_fd, buffer, size, address) == static_cast<ssize_t>(size));
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
        return KittyMemory::memProtect(address, size, protection);
    }

    /**
     * @brief High-efficiency chunk loader reading up to 0x2400 bytes in a single syscall.
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
 * @brief Global convenience memory aliases aligned with ZqwClient architecture.
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
    return engine::mem::find_pid_by_name(oxorany("com.dts.freefireth"));
}

inline uint64_t get_lib() {
    return static_cast<uint64_t>(engine::mem::find_module_base(proc::pid > 0 ? proc::pid : engine::mem::g_pid, oxorany("libil2cpp.so")));
}

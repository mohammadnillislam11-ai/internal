#pragma once
#include <cstdint>
#include <cstring>
#include <sys/uio.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <cstdio>
#include "oxorany.h"
#include "KittyMemory.hpp"

/**
 * @file memory.hpp
 * @brief High-performance remote process memory reader and writer.
 * Multi-ABI support for ARM64 and ARMv7 with process_vm_readv and /proc/$pid/mem fallback.
 */

namespace engine::mem {

    inline int       g_pid = -1;
    inline uintptr_t g_lib_base = 0;
    inline int       g_mem_fd = -1;

    inline bool is_valid_addr(uintptr_t addr) {
        #if defined(__aarch64__)
        return (addr >= 0x100000 && addr <= 0x7FFFFFFFFFFF);
        #else
        return (addr >= 0x10000 && addr <= 0xFFFFFFFF);
        #endif
    }

    inline int find_pid_by_name(const char* process_name) {
        DIR* dir = opendir("/proc");
        if (!dir) return -1;

        int found_pid = -1;
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_type != DT_DIR) continue;

            int pid = atoi(entry->d_name);
            if (pid <= 0) continue;

            char cmdline_path[64];
            snprintf(cmdline_path, sizeof(cmdline_path), "/proc/%d/cmdline", pid);

            int fd = open(cmdline_path, O_RDONLY);
            if (fd >= 0) {
                char cmdline[256] = {0};
                ssize_t bytes_read = read(fd, cmdline, sizeof(cmdline) - 1);
                close(fd);

                if (bytes_read > 0 && strstr(cmdline, process_name) != nullptr) {
                    found_pid = pid;
                    break;
                }
            }
        }
        closedir(dir);
        return found_pid;
    }

    inline uintptr_t find_module_base(int pid, const char* module_name) {
        if (pid <= 0) return 0;

        char maps_path[64];
        snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", pid);

        FILE* fp = fopen(maps_path, "r");
        if (!fp) return 0;

        char line[512];
        uintptr_t base_addr = 0;

        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, module_name) != nullptr && strstr(line, "r-xp") != nullptr) {
                #if defined(__aarch64__)
                sscanf(line, "%lx-%*lx", &base_addr);
                #else
                sscanf(line, "%x-%*x", (unsigned int*)&base_addr);
                #endif
                break;
            }
        }
        fclose(fp);
        return base_addr;
    }

    inline bool read_raw(uintptr_t address, void* buffer, size_t size) {
        if (!is_valid_addr(address) || !buffer || size == 0 || g_pid <= 0) return false;

        struct iovec local_iov;
        local_iov.iov_base = buffer;
        local_iov.iov_len  = size;

        struct iovec remote_iov;
        remote_iov.iov_base = reinterpret_cast<void*>(address);
        remote_iov.iov_len  = size;

        ssize_t bytes = process_vm_readv(g_pid, &local_iov, 1, &remote_iov, 1, 0);
        if (bytes == static_cast<ssize_t>(size)) return true;

        // Fallback: /proc/<pid>/mem if process_vm_readv is restricted
        if (g_mem_fd < 0) {
            char mem_path[64];
            snprintf(mem_path, sizeof(mem_path), "/proc/%d/mem", g_pid);
            g_mem_fd = open(mem_path, O_RDWR);
        }

        if (g_mem_fd >= 0) {
            if (lseek64(g_mem_fd, address, SEEK_SET) != -1) {
                return (read(g_mem_fd, buffer, size) == static_cast<ssize_t>(size));
            }
        }

        return false;
    }

    inline bool write_raw(uintptr_t address, const void* buffer, size_t size) {
        if (!is_valid_addr(address) || !buffer || size == 0 || g_pid <= 0) return false;

        struct iovec local_iov;
        local_iov.iov_base = const_cast<void*>(buffer);
        local_iov.iov_len  = size;

        struct iovec remote_iov;
        remote_iov.iov_base = reinterpret_cast<void*>(address);
        remote_iov.iov_len  = size;

        ssize_t bytes = process_vm_writev(g_pid, &local_iov, 1, &remote_iov, 1, 0);
        if (bytes == static_cast<ssize_t>(size)) return true;

        if (g_mem_fd < 0) {
            char mem_path[64];
            snprintf(mem_path, sizeof(mem_path), "/proc/%d/mem", g_pid);
            g_mem_fd = open(mem_path, O_RDWR);
        }

        if (g_mem_fd >= 0) {
            if (lseek64(g_mem_fd, address, SEEK_SET) != -1) {
                return (write(g_mem_fd, buffer, size) == static_cast<ssize_t>(size));
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
        return KittyMemory::memProtect(address, size, protection);
    }

} // namespace engine::mem

# NINMODS — Memory Architecture & Systems Interface (`docs/MEMORY.md`)

This document specifies the low-level memory reading and IPC architecture used across `ninmods` (Kernel Driver Mode) and `ninlite` (Root Daemon Mode), enriched by verified physical device logs (`ninmods.log`).

---

## 1. High-Level Architecture Overview

```
+-------------------------------------------------------------------------+
|                              Android UI                                 |
|         (NinLiteFloatingMenuView, EspOverlayService, MenuController)    |
+-------------------------------------------------------------------------+
                                    |
                                    v  JNI Bridge (Java_com_ninmods_main_*)
+-------------------------------------------------------------------------+
|                           Native C++ Core                               |
|       (Client.cpp, FeatureTable, AimCore, EspEngine, MemoryIO)          |
+-------------------------------------------------------------------------+
                    |                                 |
                    | (ninmods: Ring-0)               | (ninlite: Root IPC)
                    v                                 v
+---------------------------------------+   +-----------------------------+
|    Genuine Linux Kernel Driver        |   |    Root Daemon (ninlite)    |
|         (/dev/BYNb Character Node)    |   |  UNIX Socket: @ninlite_ipc  |
|    IOCTL_RT_READ  (0x801)             |   +-----------------------------+
|    IOCTL_RT_WRITE (0x802)             |                 |
|    IOCTL_RT_BASE  (0x803)             |                 | (process_vm_readv)
+---------------------------------------+                 v
                    |                      +-----------------------------+
                    +--------------------->|  Target Game Process Memory |
                                           |    (Unity / libil2cpp.so)   |
                                           +-----------------------------+
```

---

## 2. Kernel Driver Mode (`ninmods` - RT Channel)

`ninmods` operates exclusively through a genuine Linux kernel module character device driver.

### Character Device Nodes Probed
The native engine probes the following character device nodes in order:
1. `/dev/BYNb` (Primary standard node)
2. `/dev/mem_tool`
3. `/dev/nh`
4. `/dev/wanbai`
5. `/proc/uevents_records`

### IOCTL Command Definitions
Located in `Includes/MemoryIO.h`:
```c
#define IOCTL_RT_READ   0x801
#define IOCTL_RT_WRITE  0x802
#define IOCTL_RT_BASE   0x803
```

### IOCTL Data Structures
```c
// 32-byte memory copy structure for IOCTL_RT_READ and IOCTL_RT_WRITE
struct COPY_MEMORY {
    pid_t pid;          // Target game process PID
    uintptr_t addr;     // Virtual memory address in target process
    void *buffer;       // Destination buffer in current process
    size_t size;        // Number of bytes to read/write
};

// 24-byte module lookup structure for IOCTL_RT_BASE
struct MODULE_BASE {
    pid_t pid;          // Target game process PID
    char *name;         // Null-terminated module name (e.g. "libil2cpp.so")
    uintptr_t base;     // Resolved virtual base address returned by driver
};
```

### SELinux & Node Permissions
Before communication, root privileges apply permissions:
```sh
chmod 666 /dev/BYNb* /dev/mem_tool* /dev/nh* /dev/wanbai* /proc/uevents_records
setenforce 0
```

---

## 3. Root Daemon IPC Mode (`ninlite` - QX Channel)

`ninlite` is designed for high-performance zero-injection memory inspection using a standalone root daemon process (`ninlite_daemon`):
- **Communication Channel**: Abstract UNIX domain socket `@ninlite_ipc`.
- **System Memory Operation**: Official Linux kernel syscall `process_vm_readv` (Syscall number 270 on AArch64, 376 on ARMv7).
- **Security Boundary**: Zero library injection into the game process address space. Both anti-cheat and process integrity remain completely untouched.

### Verified Physical Device Evidence (`ninmods.log`)
From live device testing on target Free Fire MAX (`com.dts.freefiremax`):
- **Target PID**: Correctly detected and hooked (`PID 11271`).
- **Base Address Resolution**: `libil2cpp.so` resolved at virtual address `0x76d4cc9000`.
- **ELF Magic Verification**: First 4 bytes read from `0x76d4cc9000` through `process_vm_readv` verified as `\x7f E L F` (genuine ELF header).
- **Socket Connectivity**: Native client connected to `@ninlite_ipc` with zero packet loss and low-latency round-trip.

---

## 4. Master ESP Gating & Lobby Memory Pause

To minimize CPU utilization, eliminate battery drain, and prevent anti-cheat memory scan heuristics when players are idling in the lobby, the engine implements Master ESP Gating:

### Pipeline Execution
1. **Menu Control**: The user toggles "ESP Master" in `NinLiteFloatingMenuView` (`FeatureCatalog.ESP_MASTER`, native ID 1).
2. **JNI Transmission**: `NativeBridge.setFeatureState(1, state)` invokes `NinFeatureSetState(1, state)` in `Client.cpp`.
3. **IPC Dispatch**: `SyncWithNinLiteDaemon(Ninlite::MODE_MASTER, state, 0)` sends a non-blocking request to the root daemon.
4. **Daemon Gating (`ninlite_daemon.cpp`)**:
   ```cpp
   static void CreateDataList(Ninlite::Response& res) {
       res.success = 1;
       res.playerCount = 0;
       if (!Actived.activar) return; // ZERO MEMORY SCAN IN LOBBY
       // ... scan entity pointers ...
   }
   ```
5. **Overlay Gating (`Client.cpp`)**:
   ```cpp
   void DrawESP() {
       if (!Actived.activar) return; // ZERO DRAWING OVERHEAD
       // ... render visual elements ...
   }
   ```
6. **Sub-feature State Preservation**: Turning the Master switch OFF does **not** erase sub-feature configurations (Box, Line, Distance, Name). When re-enabled inside a match, all previously active visuals immediately resume without needing reconfiguration.

---

## 5. Single Central Offsets Architecture (`gameoffsets.h`)

All virtual memory offsets (pointers, matrices, entity coordinates, health, bone matrices) are strictly defined in **exactly one file**:
`app/src/main/jni/src/Includes/gameoffsets.h`

Reference projects (`ZqwClient`, `Bull_Team`, `treasure`) are used for pattern comparison, but runtime offsets are unified in `gameoffsets.h` to ensure full multi-version compatibility across standard and MAX Free Fire releases.

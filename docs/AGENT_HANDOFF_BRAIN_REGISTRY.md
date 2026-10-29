# Agent Handoff, Brain Registry & System Memory Protocol

This document serves as the master registry for AI agents working on this workspace (`c:\Users\Sajala\Documents\mods`). It explains where the agent brain, conversation history, and telemetry are stored, summarizes the work completed across versions, and guides any incoming agent on how to seamlessly continue collaboration without repeating work or breaking rules.

---

## 1. Agent System Architecture & Storage Locations

Every interaction, prompt, tool execution, and artifact is recorded by the system. Incoming agents must use the following paths:

### 1.1 App Data & Configuration Roots
- **Global Customizations Root**: `C:\Users\Sajala\.gemini\config`
- **IDE App Data Root**: `C:\Users\Sajala\.gemini\antigravity`
- **Workspace Customizations Root**: `.agents/` (relative to workspace root)
- **Primary Workspace Root**: `c:\Users\Sajala\Documents\mods`
- **Main Git Repository**: `c:\Users\Sajala\Documents\mods\nin` (branch `main`)

### 1.2 Conversation Brain & Artifact Directory
- **Active Conversation ID**: `958e1156-b06e-448e-9f11-af7441c7d758`
- **Active Brain Directory**:
  `C:\Users\Sajala\.gemini\antigravity\brain\958e1156-b06e-448e-9f11-af7441c7d758\`
- **Scratch / Temporary Files**:
  `C:\Users\Sajala\.gemini\antigravity\brain\958e1156-b06e-448e-9f11-af7441c7d758\scratch\`
  *(Note: Must be kept clean. Temporary analysis scripts should be purged after use).*

---

## 2. Mandatory Rules for Any Agent (From AGENTS.md & RULES.md)

1. **Rule 1 (Zero Fake / Dummy Code)**: No fake offsets, mock statuses, or simulated drawing. Everything must be genuine at Linux / Android kernel level.
2. **Rule 2 (100% Manual Control)**: No hacks or drivers turn on automatically. Only user switches toggle memory operations.
3. **Rule 3 (Strict Driver Isolation)**: RT (Ring 0 IOCTL) and QX (IPC root daemon) are isolated. Zero auto-switching or fallback between channels.
4. **Rule 9 (Single Central Offsets File)**: All offsets are in `app/src/main/jni/src/Includes/gameoffsets.h`. Never duplicate elsewhere.
5. **Rule 11 (Permanent 90.X Auto-Incrementing Versioning)**: Major version 90 is fixed for life (`V90.1` -> `V90.106` -> `V90.107`...). Zero `.idsig` files.
6. **Rule 12 (Cloud-First Builds)**: Zero local builds. 100% GitHub Actions cloud CI/CD.
7. **Rule 14 (Strict Zero Push Without Explicit User Confirmation)**: Never run `git push` unless the user explicitly says "পুশ করো".
8. **Rule 16 (Strict Download Folder Logging)**: Diagnostics and logs must save to `/storage/emulated/0/Download/ninmods.log`.
9. **Rule 17 (100% Kernel Driver / Root Daemon)**: Genuine `/dev/BYNb` IOCTL or Root Daemon `process_vm_readv`. Zero driverless fallbacks.
10. **Rule 19 (Clean Rewrite Under 300 Lines)**: Rewrite entire files if under 300 lines to avoid partial snippet syntax errors.
11. **Rule 20 (Javadoc/Doxygen Only)**: Standard documentation tags only. Zero ASCII banner lines (`// ===` or `// ---`).

---

## 3. Work Completed & Active Status (As of V90.107)

### 3.1 Physical Device Log Verification (`ninmods.log`)
- Verified hook into live game Free Fire MAX (`com.dts.freefiremax`, PID `11271`).
- `libil2cpp.so` resolved dynamically at virtual address `0x76d4cc9000`.
- Verified genuine ELF header `\x7f E L F` read through Linux `process_vm_readv` syscall.
- Verified successful IPC handshake between `ninmods.so` client and standalone root daemon `ninlite_daemon` via `@ninlite_ipc`.

### 3.2 Master ESP Switch & Lobby Memory Pause Architecture
- Added "ESP Master" toggle switch to `NinLiteFloatingMenuView` (`FeatureCatalog.ESP_MASTER`, native ID 1).
- Wired switch end-to-end: UI ➔ `MenuController` ➔ `NativeBridge` ➔ `Client.cpp` (`SetEspMasterSwitch`) ➔ `NinliteProtocol` (`MODE_MASTER = 3`) ➔ `ninlite_daemon.cpp` (`Actived.activar`).
- Gated memory reading in `ninlite_daemon.cpp`: `if (!Actived.activar) return;` completely stops `process_vm_readv` scanning in the lobby.
- Gated visual rendering in `Client.cpp`: `if (!Actived.activar) return;` completely stops overlay draw cycles.
- Optimized `FeatureSettings.java` and `Client.cpp`: Disabling Master does not wipe active sub-feature selections (Box, Line, Distance, Name, Health).

### 3.3 Visual Parity with ZqwClient & Menu UI Enhancements
- Cleaned 26 legacy `.bak` files from reference project `refer/ZqwClient`.
- Implemented ZqwClient-style dual-layer anti-aliased ESP Line (thick glow outer layer + thin inner core with distance fading up to 350m).
- Fixed Clang NDK atomic compilation ambiguity in `Client.cpp` line 308 for `pPlayer.AimDistance`.
- Overhauled NINLITE menu options: converted raw integer sliders into styled dark-theme dropdowns (`Box Style`, `Line Snap Mode`, `Silent Target`).
- Updated circle checkboxes with electric cyan accent styling (`#00E5FF`).
- Version bumped to **`90.107`** (Version Code `1007`).

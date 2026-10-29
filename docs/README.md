# NINMODS Documentation Index (`docs/README.md`)

Welcome to the central documentation directory for the NINMODS project. All AI agents, contributors, and developers must read and strictly follow the documents located here before modifying any code.

---

## Documentation Structure

1. **[`docs/RULES.md`](file:///c:/Users/Sajala/Documents/mods/docs/RULES.md)**:
   - Mandatory engineering standards and non-negotiable rules.
   - Javadoc/Doxygen comment style specification (prohibition of ASCII banner comments).
   - Mandatory Package & Import Header Audit Rule (verifying `package` declaration and all `import` statements).
   - Mandatory end-to-end holistic code review for all AI agents.
   - Kernel driver only policy for `ninmods` (RT Ring-0 IOCTL) and root daemon IPC for `ninlite` (QX `process_vm_readv`).
   - Single central offsets architecture (`gameoffsets.h`).
   - Permanent 90.X auto-incrementing version protocol (`V90.107`+).
   - Zero local builds and strict GitHub Actions CI/CD rules.
   - User explicit confirmation required before any `git push` ("পুশ করো").

2. **[`docs/MEMORY.md`](file:///c:/Users/Sajala/Documents/mods/docs/MEMORY.md)**:
   - Low-level Linux/Android memory reading architecture and verified device log analysis (`ninmods.log`).
   - Ring-0 Character Device IOCTL communication (`/dev/BYNb`, `IOCTL_RT_READ 0x801`, `IOCTL_RT_WRITE 0x802`, `IOCTL_RT_BASE 0x803`).
   - `COPY_MEMORY` and `MODULE_BASE` struct specifications.
   - `ninlite` IPC architecture using root daemon (`ninlite_daemon`), UNIX domain sockets (`@ninlite_ipc`), and official Linux syscall `process_vm_readv`.
   - Verified memory dump analysis: `libil2cpp.so` ELF base verification at `0x76d4cc9000` for Free Fire MAX (PID 11271).
   - Master ESP gating (`Actived.activar` & `MODE_MASTER`): lobby scan pause and entity lifecycle gating.

3. **[`docs/GEMINI.md`](file:///c:/Users/Sajala/Documents/mods/docs/GEMINI.md)**:
   - Directives and operating guidelines for Gemini, Claude, and Antigravity AI agents.
   - Deep investigation workflows, system impact mapping, import verification, and end-to-end verification procedures.
   - Common C++ Clang gotchas (e.g., `std::atomic<float>` ternary expression ambiguity).
   - Multi-UI observer synchronization via `MenuController` and `FeatureSettings`.

4. **[`docs/AGENT_HANDOFF_BRAIN_REGISTRY.md`](file:///c:/Users/Sajala/Documents/mods/docs/AGENT_HANDOFF_BRAIN_REGISTRY.md)**:
   - Master registry for AI agents working on this workspace (`c:\Users\Sajala\Documents\mods`).
   - Version history up to `V90.107` (Code 1007).
   - Real-device telemetry and log evidence (`ninmods.log`).
   - ESP dual-layer glow tracer implementation (ZqwClient reference parity).
   - Protocol mapping: Java UI ➔ `MenuController` ➔ `NativeBridge` ➔ `Client.cpp` ➔ `NinliteProtocol` ➔ `ninlite_daemon`.

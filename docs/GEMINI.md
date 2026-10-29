# NINMODS — AI Agent Directives & Operating Protocol (`docs/GEMINI.md`)

This document governs the behavior, investigation procedures, and implementation standards for all AI agents (Gemini, Claude, Antigravity) working in this workspace.

---

## 1. Deep Investigation & Holistic Code Review Before Any Edit

1. **Never Start Editing Immediately:**
   - Every AI agent MUST understand the complete system lifecycle before modifying any code.
   - Inspect existing architecture, manifests, configuration, and interfaces.
   - Search globally for affected symbols, usages, and callers across all layers.

2. **Mandatory Full Code Review (Holistic Audit):**
   - Trace the complete execution path:
     `UI Entry Point` ➔ `MenuController` ➔ `NativeBridge` ➔ `Client.cpp` ➔ `NinliteProtocol` ➔ `ninlite_daemon.cpp` ➔ `Overlay Surface`
   - Review all callers and callees. Ensure changes in Java UI or C++ JNI preserve binary signatures and IPC message structures.

---

## 2. Mandatory Package & Import Header Verification

* Whenever editing or reviewing any file, the agent **MUST** inspect the file header:
  - Verify that the `package` declaration is present and exact (e.g., `package com.ninmods.main.menu.ninlite;`).
  - Verify all necessary `import` statements are present (e.g., `FeatureSettings`, `FeatureCatalog`, `NativeBridge`, etc.).
  - Remove broken, stale, or unused imports.
  - Never allow compilation errors due to missing imports or package declarations.

---

## 3. Mandatory Code Commenting Standard

* **Zero ASCII Banners:**
  Strictly do not write comment banners such as:
  `// =========================================================================`
  `// -------------------------------------------------------------------------`
* **Javadoc / Doxygen Style Required:**
  Always document functions, methods, and classes using standard Javadoc / Doxygen format with `@param`, `@return`, and `@throws` tags.

---

## 4. Native C++ Clang Nuances & Compiler Rules

1. **Atomic Type Ambiguity:**
   - Clang NDK compiler will fail with `conditional expression is ambiguous; 'std::atomic<float>' can be converted to 'float' and vice versa` if an atomic variable is used directly in ternary operators (e.g., `(pPlayer.AimDistance > 50.0f) ? pPlayer.AimDistance : 350.0f`).
   - Always load the atomic value explicitly first:
     ```cpp
     float aimDist = pPlayer.AimDistance.load(std::memory_order_relaxed);
     float maxEspDist = (aimDist > 50.0f) ? aimDist : 350.0f;
     ```

2. **IPC Struct Alignment:**
   - Ensure `Ninlite::Request` and `Ninlite::Response` structures in `NinliteProtocol.h` match identical byte offsets and padding across both Client (`ninmods.so`) and Daemon (`ninlite_daemon`).

---

## 5. Strict Operating Guardrails

1. **Zero Push Without Explicit User Command:**
   - Never run `git push` or trigger cloud builds autonomously.
   - Wait for the user to explicitly type or say "পুশ করো".

2. **Zero Local Machine Compilation:**
   - Never run Gradle or NDK compilers locally. 100% of APK compilation occurs in GitHub Actions.

3. **Single Central Offsets File:**
   - Offsets exist exclusively in `Includes/gameoffsets.h`. Zero duplication in Kotlin or Java.

4. **Single Kotlin UI Overlay:**
   - All memory calculations and WorldToScreen projections run 100% in C++. Kotlin only has `EspOverlayService.kt` for canvas rendering.

5. **Files Under 300 Lines:**
   - Files under 300 lines must be rewritten completely and cleanly rather than edited in partial snippets.

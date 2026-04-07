# Windows enablement slice 1 — 2026-04-06

This note records the first step where the Windows runtime-target path became more than an architectural placeholder.

---

# What changed

## 1. Platform-target selection now controls source wiring
`engine-cef/CMakeLists.txt` now conditionally selects host/runner implementation sources based on:
- `ENGINE_CEF_RUNTIME_TARGET_PLATFORM=linux|windows|auto`

### Linux target wiring
Includes:
- `cef_osr_host_gtk.*`
- `cef_linux_main_runner.*`
- GTK dependency wiring
- Linux-only sandbox helper wiring

### Windows target wiring
Includes scaffold sources:
- `src/win/cef_runtime_window_host_win.*`
- `src/win/cef_runtime_runner_win.*`

This means the Windows path is now part of the target source-selection story, not just dormant files next to Linux code.

---

## 2. Windows target configuration now succeeds in the current workspace
A configure pass with:
- `-DENGINE_CEF_RUNTIME_TARGET_PLATFORM=windows`

now succeeds in this workspace instead of hard-failing immediately.

That is important because it proves:
- the platform-selection path is real
- the Windows host/runner scaffolds are accepted by CMake target wiring
- the project is no longer structurally assuming Linux-only source selection at configure time

---

## 3. Linux remains the stable active lane
The existing Linux runtime-host/browser lane still configures and builds successfully after this change.

That means the Windows-target wiring did not destabilize the current working path.

---

# What this slice does **not** mean

This does **not** mean BRIDGE now has a working Windows runtime-host build.

The Windows path is still scaffold-level because:
- Win32 host implementation is not done
- Windows runner implementation is not done
- Windows-specific packaging/bootstrap is not done
- browser-level runtime behavior still needs actual Windows host work underneath the seams

So this slice should be read as:

> Windows is now a real compile-time/runtime-target concept in source wiring, but not yet a functional runtime-host implementation.

---

# Why this matters

Before this slice:
- Windows existed in docs, seams, and scaffold files
- but not yet as a real source-selection path in the runtime-host build

After this slice:
- Windows exists in seams
- Windows exists in scaffolds
- Windows exists in factory selection
- Windows exists in CMake source wiring
- Windows-target configuration succeeds

That is the first honest point where the Windows path becomes an actual engineering track rather than just preparation work.

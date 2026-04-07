# Platform selection slice — 2026-04-06

This note records the first explicit platform-selection wiring pass for the `engine-cef` runtime-host architecture.

---

# What changed

## 1. Runtime target platform is now explicit in CMake
A new cache variable was introduced:
- `ENGINE_CEF_RUNTIME_TARGET_PLATFORM`

Allowed values:
- `auto`
- `linux`
- `windows`

Current behavior:
- `auto` resolves to `linux` on the current supported bootstrap path
- non-Linux bootstrap is still rejected for now
- selecting `windows` currently fails early with a clear message because the Windows path is still scaffold-only

This is intentional.
The goal of this slice is to make platform selection explicit without pretending Windows is already build-wired.

---

## 2. Host/runner factories now branch on platform-selection macros
The runtime host and runtime runner factories now branch on explicit compile-time platform-target definitions:
- `ENGINE_CEF_RUNTIME_TARGET_PLATFORM_LINUX`
- `ENGINE_CEF_RUNTIME_TARGET_PLATFORM_WINDOWS`

That means the factories now read like true selection points rather than “Linux implementation with future stubs nearby.”

---

## 3. Current status of platform targets
### Linux
- fully wired as the active runtime-host platform target
- selected through the new platform-selection path

### Windows
- host scaffold exists
- runner scaffold exists
- factory branches know about the Windows types
- build wiring is still intentionally blocked pending real source/target integration work

---

# Why this matters

Before this slice, platform seams existed, but the factories still behaved like quiet Linux defaults.

After this slice:
- platform target selection is explicit in build configuration
- the codebase has named selection points for runtime host and runtime runner
- Windows is represented as a real platform concept in that selection story, even though it is not yet enabled

That is a better setup for the next platform-enablement steps.

---

# What remains

This slice did not yet do full platform enablement.
Still remaining:
- CMake source-list wiring for Windows host/runner sources
- actual Win32 runtime-window host implementation
- actual Windows runtime runner implementation
- broader non-Linux bootstrap support in `engine-cef`

So the current state is:
- explicit platform selection architecture
- Linux wired
- Windows scaffolded but still gated

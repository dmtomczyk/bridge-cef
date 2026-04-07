# Runtime seam tightening note — 2026-04-06

This note records a small but important cleanup pass after the first runtime host/runner seams landed.

---

# What was tightened

## 1. Factory naming now reflects platform intent, not fallback/default intent
Renamed:
- `CreateDefaultRuntimeWindowHost()` → `CreatePlatformRuntimeWindowHost()`
- `CreateDefaultRuntimeRunner()` → `CreatePlatformRuntimeRunner()`

Why this matters:
- `default` reads like "Linux for now"
- `platform` reads like "the implementation selected for the current build/runtime target"

That is a better fit for the architecture direction.

## 2. Generic runner options no longer expose an X11-specific name
Renamed in `CefRuntimeRunnerOptions`:
- `force_x11_backend_for_osr` → `force_platform_osr_backend`

Why this matters:
- the old name leaked Linux/X11 assumptions into the generic runtime-runner seam
- the new name keeps the shared seam platform-neutral
- Linux can still map that generic intent to its X11-specific behavior internally

---

# What did not change

This pass did **not** remove Linux-specific implementation details from the Linux runner itself.
That is fine.

The point was only to stop the shared seam from speaking in Linux/X11-specific terms where it did not need to.

Linux runner internals still legitimately own:
- X11 backend forcing
- GTK initialization timing
- Linux cache/bootstrap details

---

# Why this pass matters

Once a seam exists, naming becomes architecture.

If shared seams keep Linux-shaped names, the codebase still *feels* Linux-first even when implementation boundaries are improving.
This pass makes the new runtime seams read more like intentional multi-platform architecture and less like Linux code with nearby Windows stubs.

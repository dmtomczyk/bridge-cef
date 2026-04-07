# Windows runner scaffold note — 2026-04-06

This note records the first explicit Windows runtime-runner landing zone added after decoupling `cef_runtime_entry.*` from direct Linux runner ownership.

Created scaffold files:
- `src/win/cef_runtime_runner_win.h`
- `src/win/cef_runtime_runner_win.cc`

These files are intentionally not wired into the build yet.
They exist to make the future Windows runtime-launch path concrete.

---

# What changed architecturally

Before this slice, `cef_runtime_entry.h` directly included `cef_linux_main_runner.h`, and runtime entry config directly referenced Linux runner options.

After this slice:
- `cef_runtime_entry.*` depends on a platform-neutral runtime-runner interface
- platform runner selection happens through `CreatePlatformRuntimeRunner()`
- Linux is the current implementation behind that factory
- Windows now has an explicit runner scaffold beside the Windows host scaffold

---

# What the Windows runner will eventually need to own

A real Windows runner will need to handle:
- Windows/CEF process bootstrap
- Windows-specific message-loop expectations if they differ materially
- any Win32-specific environment or runtime setup needed before `CreateInitialBrowser()`
- platform-correct shutdown/exit semantics

At the moment the Windows runner stub simply returns a failure code.
That is intentional.

---

# Why this matters

This slice does for runtime launch what the earlier host seam did for runtime hosting:
- it stops the shared runtime entry layer from being Linux-shaped
- it gives Windows a real place to plug in later

That makes the remaining multi-platform work more explicit and less tangled.

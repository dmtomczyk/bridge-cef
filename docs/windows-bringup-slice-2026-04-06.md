# Windows bring-up slice — 2026-04-06

This note records the first small step from pure Windows scaffolding toward a more coherent bring-up path.

---

# What changed

## 1. Windows runtime window host now initializes into a coherent placeholder state
`CefRuntimeWindowHostWin::Initialize()` no longer immediately fails.

Instead, it now:
- marks the host initialized
- sets a default placeholder view size (`1280x800`)
- resets frame bookkeeping
- leaves `parent_handle_` at `0` for now

This does **not** create a real native window.
But it does make the host behave more like an incomplete platform implementation and less like a hardcoded stub.

## 2. Windows host geometry/state now tracks placeholder runtime state
The Windows host now maintains:
- view size
- last presented frame dimensions/stride
- whether a frame has been seen
- normalized tab-count minimum behavior

Again, this is not real Win32 hosting yet.
But it gives the host a more internally consistent shape for future implementation.

## 3. Windows runner now fails explicitly with a useful runtime error
`CefRuntimeRunnerWin::Run(...)` now sets a concrete runtime error on `CefAppHost` before returning failure:

> Windows runtime runner scaffold is selected, but Win32 runtime bootstrap is not implemented yet

That is much better than a silent generic `return 1`, because it tells future debugging exactly what is missing.

---

# Why this matters

The goal of this slice was not to fake a working Windows runtime.
The goal was to improve the first failure mode and make the Windows path more coherent.

That matters because once platform selection and source wiring exist, the next bottleneck becomes:
- whether the Windows path fails in informative, structurally honest ways
- whether the Windows host state shape is ready for incremental implementation

This slice improves both.

---

# What still remains before Windows runtime-host bring-up is real

Still missing:
- actual Win32 native window creation
- actual parent handle ownership for OSR
- frame presentation to a native surface
- Windows event/message loop integration
- keyboard/mouse/wheel input routing
- Windows-native dialog/external-open implementations
- real runner/bootstrap logic

So this should still be read as:

> better Windows bring-up scaffolding, not a working Windows runtime-host implementation.

# Windows native host skeleton — 2026-04-06

This note records the next Windows host bring-up step after the earlier placeholder-state improvements.

---

# What changed

## 1. The Windows host now contains a real Win32-native skeleton under `_WIN32`
`CefRuntimeWindowHostWin` now includes guarded Win32 window-management code for actual Windows builds.

Under `_WIN32`, the host now contains:
- a native window class name
- `WindowProc(...)`
- `RegisterClassExW(...)`
- `CreateWindowExW(...)`
- `ShowWindow(...)`
- `UpdateWindow(...)`
- client-rect-based view-size syncing
- title updates via `SetWindowTextW(...)`
- `ClientToScreen(...)`-based screen-point translation
- native window destruction in the host destructor

That means the Windows host is no longer only a data placeholder.
It now has the beginnings of a real native host lifecycle for actual Windows builds.

---

## 2. Non-Windows builds still keep the placeholder path
Because this workspace is validating the Windows target path from a Linux machine, the Win32 code is guarded behind `_WIN32` and the non-Windows placeholder behavior remains available.

That preserves two useful properties at once:
- Linux-based validation of the Windows-target source path still works
- real Windows builds now have a more meaningful host implementation shape to grow into

---

# What this does **not** do yet

This is still only a host skeleton.
It does not yet provide:
- real OSR frame blitting/presentation into the window
- Win32 input/event forwarding into CEF
- a real message-loop/bootstrap integration path
- native dialogs or external URL handoff
- full browser-shell usability on Windows

So the right reading is:

> the Windows host now has a native window lifecycle skeleton, but not yet a functioning browser runtime host.

---

# Why this matters

This is the first point where the Windows host begins to look like a real platform implementation instead of just a placeholder state container.

That makes the next steps much more concrete:
- event/message loop integration
- browser/OSR attach lifecycle
- frame presentation
- input plumbing

Those are still hard, but they now have a native-window foundation to build on.

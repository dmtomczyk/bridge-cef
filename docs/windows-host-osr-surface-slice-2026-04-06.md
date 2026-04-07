# Windows host OSR surface slice — 2026-04-06

This note records the next step after the native-window skeleton: the Windows host now has a very early OSR/frame presentation path and basic browser lifecycle notifications.

---

# What changed

## 1. Win32 paint path now exists
Under `_WIN32`, `CefRuntimeWindowHostWin` now handles:
- `WM_PAINT`
- a basic `PaintFrameToWindow(HDC)` path
- `StretchDIBits(...)` blitting of the last ARGB frame buffer into the native window

This is intentionally minimal and not yet polished.
But it is the first real frame-to-window path in the Windows host.

## 2. Last-frame pixels are now stored
`PresentFrame(...)` now stores the last ARGB frame buffer rather than just recording dimensions.
That gives the Windows host something real to paint when the window invalidates.

## 3. Resize/focus lifecycle now notifies CEF browser host state
Under `_WIN32`, the host now reacts to:
- `WM_SIZE`
- `WM_SETFOCUS`
- `WM_KILLFOCUS`

and forwards basic state changes through:
- `WasResized()`
- `NotifyScreenInfoChanged()`
- `SetFocus(...)`
- `SendCaptureLostEvent()`

That is still a very early integration layer, but it is more realistic than a pure shell window.

## 4. Browser attachment now triggers initial host-state sync
When a browser is attached, the Windows host now performs a first resize/focus notification pass.

---

# What this means

This slice does **not** make Windows runtime-host usable yet.
But it does move the Windows host from:
- native window skeleton only

to:
- native window + first crude OSR presentation path + first CEF host lifecycle signals

That is a meaningful change in kind.

---

# What still remains

Still missing or incomplete:
- real event/message loop integration nuances
- mouse/keyboard/wheel input forwarding
- robust paint/resize pacing
- native dialogs/external-open behavior
- correctness testing on an actual Windows machine
- browser-shell UX beyond the raw host surface

So this should be read as:

> first crude Windows OSR host surface, not yet a polished or validated Windows browser runtime.

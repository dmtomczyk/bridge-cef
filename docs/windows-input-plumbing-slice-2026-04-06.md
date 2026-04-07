# Windows input plumbing slice — 2026-04-06

This note records the first Win32 input-forwarding path added to the Windows runtime host.

---

# What changed

Under `_WIN32`, `CefRuntimeWindowHostWin` now translates and forwards a first basic set of native window messages into the attached CEF browser host.

## Mouse input
Handled messages now include:
- `WM_MOUSEMOVE`
- `WM_MOUSELEAVE`
- `WM_LBUTTONDOWN` / `UP` / `DBLCLK`
- `WM_MBUTTONDOWN` / `UP` / `DBLCLK`
- `WM_RBUTTONDOWN` / `UP` / `DBLCLK`
- `WM_MOUSEWHEEL`
- `WM_MOUSEHWHEEL`

Forwarded via:
- `SendMouseMoveEvent(...)`
- `SendMouseClickEvent(...)`
- `SendMouseWheelEvent(...)`

Also included:
- mouse-leave tracking via `TrackMouseEvent(...)`
- basic capture/release on mouse down/up
- current modifier/button flag synthesis from Win32 key state

## Keyboard input
Handled messages now include:
- `WM_KEYDOWN`
- `WM_SYSKEYDOWN`
- `WM_KEYUP`
- `WM_SYSKEYUP`
- `WM_CHAR`
- `WM_SYSCHAR`

Forwarded via:
- `SendKeyEvent(...)`

with first-pass population of:
- event type
- Windows key code
- native key code
- modifier flags
- system-key flag

---

# Why this matters

Before this slice, the Windows host had:
- native window lifecycle
- crude OSR paint path
- basic resize/focus lifecycle notifications

But it still had no meaningful user-input path.

After this slice, the Windows host now has the first real path for:
- pointer movement
- clicking
- wheel scrolling
- key events

That is a necessary step toward anything resembling an actually interactive browser surface.

---

# What this still does not guarantee

This is still an early input layer, not a complete or validated Windows browser UX.
Still missing or uncertain:
- correctness tuning on a real Windows machine
- IME/text-input nuance
- shortcut behavior validation
- pointer capture edge cases
- drag/select/context-menu behavior
- richer browser-shell UI behavior in the Windows host

So the right reading is:

> the Windows runtime host now has its first basic native input plumbing, but it still needs real validation and refinement.

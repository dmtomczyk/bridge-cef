# Windows shortcut correctness slice — 2026-04-06

This note records the first Windows runtime-host accelerator pass.

---

# What changed

Under `_WIN32`, `CefRuntimeWindowHostWin` now intercepts and handles a first useful set of keyboard accelerators before forwarding generic key events.

## Implemented accelerators
- `F5` / `Ctrl+R` → reload
- `Alt+Left` → back
- `Alt+Right` → forward
- `Ctrl+T` → new tab callback
- `Ctrl+W` → close active tab callback
- `Ctrl+Shift+T` → reopen closed tab callback
- `Ctrl+Tab` / `Ctrl+Shift+Tab` → tab advance callback
- `Alt+1..9` → direct tab activation callback

These are handled in `HandleAccelerator(...)` and consume the event when matched.

---

# What did not change

This slice did **not** attempt to fake Windows-host features that do not yet exist, such as:
- a real omnibox/address-bar UI target for `Ctrl+L`
- tab-strip UI parity in the native Windows host
- broader shortcut correctness validation on an actual Windows machine

So the shortcut layer is intentionally pragmatic, not complete.

---

# Why this matters

Before this slice, the Windows host had:
- native input forwarding
- rendering/presentation skeleton
- lifecycle glue

But it still lacked the browser-style keyboard commands that make the runtime-host feel like a browser shell rather than a raw embedded surface.

After this slice, the Windows runtime-host path now has the first meaningful browser accelerators wired to real browser/tab actions.

That is a useful correctness/UX step even before the Windows host has full chrome parity.

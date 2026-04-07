# Windows lifecycle correctness slice — 2026-04-06

This note records a small but important Windows runtime-host correctness pass focused on close/shutdown behavior.

---

# What changed

## 1. Window close now prefers browser-driven shutdown when a browser exists
On `WM_CLOSE`, the Windows host now:
- marks close as requested
- asks the attached CEF browser host to close via `CloseBrowser(false)` when available
- only destroys the native window immediately if there is no browser host to close

This is a better lifecycle shape than unconditionally destroying the native window first.

## 2. Browser-close completion now tears down the native window when appropriate
When `NotifyBrowserClosing(...)` runs after a close was requested, the host now destroys the native window.

That creates a more coherent sequence:
- user closes window
- browser gets a chance to close cleanly
- native window is destroyed after browser-close flow progresses

## 3. Message-loop quit is now tied to native window destruction
On `WM_DESTROY`, the Windows host now:
- clears its native-window state
- requests `CefQuitMessageLoop()` exactly once

That gives the Win32 runtime path a much more predictable shutdown point.

## 4. Initialization now resets lifecycle state explicitly
`Initialize()` now resets:
- `close_requested_`
- `quit_requested_`
- cached frame buffer state

That makes repeated host construction/bring-up semantics cleaner and less accidental.

---

# Why this matters

Once a platform host starts having real window, paint, and input paths, the next easiest way to become flaky is lifecycle mismatch.

This slice improves that by making the Windows host more deliberate about:
- who initiates close
- when the native window disappears
- when the runtime message loop exits

It is still not production-hardened, but it is a more believable platform lifecycle now.

---

# Current reading of the Windows path

At this point the Windows runtime-host lane has:
- native window skeleton
- runner/bootstrap skeleton
- crude OSR paint path
- first input plumbing
- first lifecycle/close correctness pass

That is still not a validated Windows browser runtime.
But it is no longer just architecture scaffolding.

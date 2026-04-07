# Windows runner bootstrap skeleton — 2026-04-06

This note records the step where the Windows runtime runner stopped being a pure stub and gained a real `_WIN32` bootstrap skeleton.

---

# What changed

Under `_WIN32`, `CefRuntimeRunnerWin::Run(...)` now has a real runtime bootstrap shape:
- builds `CefMainArgs` from `GetModuleHandleW(nullptr)`
- calls `CefExecuteProcess(...)`
- sets up CEF settings
- creates a runtime cache directory
- enables windowless rendering when requested
- calls `CefInitialize(...)`
- calls `app->CreateInitialBrowser()`
- enters `CefRunMessageLoop()`
- calls `CefShutdown()` on exit

That means the Windows runner is no longer just a `return 1` placeholder on actual Windows builds.

---

# What still remains

This is still only a bootstrap skeleton, not a working Windows browser runtime.
It still depends on incomplete lower layers, especially:
- Windows host implementation details
- actual OSR/native presentation behavior
- Win32 input/event integration
- real packaging/bootstrap nuances for Windows distributions

Also, on non-Windows builds, the runner still intentionally fails with a clear runtime error when the Windows path is selected.
That is correct for current Linux-based validation.

---

# Why this matters

With this slice, both major Windows runtime pieces now have real `_WIN32` skeletons:
- host lifecycle skeleton
- runner/bootstrap skeleton

That shifts the remaining work away from architecture/setup and more toward concrete platform implementation details.

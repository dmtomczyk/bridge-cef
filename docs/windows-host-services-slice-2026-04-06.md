# Windows host services slice — 2026-04-06

This note records the first Windows-native implementation pass for host-owned platform services behind the runtime host seam.

---

# What changed

## 1. External URL handoff now has a Win32 implementation
Under `_WIN32`, `OpenUrlExternally(...)` now uses:
- `ShellExecuteW(..., L"open", ...)`

That means popup/external-open behavior now has a native Windows implementation path instead of an automatic stub failure.

## 2. File dialog handling now has a first Win32 implementation
Under `_WIN32`, `RunFileDialog(...)` now supports:
- file open
- file open multiple
- file save
- folder selection

Implemented via Win32-native APIs:
- `GetOpenFileNameW(...)`
- `GetSaveFileNameW(...)`
- `SHBrowseForFolderW(...)`
- `SHGetPathFromIDListW(...)`

This includes:
- UTF-8 ↔ wide-string conversion
- basic filter construction from accepted extensions/descriptions
- callback `Continue(...)` / `Cancel()` behavior through the existing seam contract

---

# Why this matters

Earlier seam work moved dialogs and external-open behavior out of shared browser/runtime code and into host-owned services.
This slice gives the Windows host its first actual implementations of those services.

That is important because it means the Windows path now covers more than:
- window
- paint
- input
- lifecycle

It now also covers two concrete platform-service responsibilities that real browser behavior depends on.

---

# What still remains

This is still an early Windows host implementation and not yet validated on an actual Windows machine.
Remaining likely issues/unknowns include:
- edge cases in Win32 common-dialog behavior
- multi-select/path parsing robustness
- richer Windows shell-integration nuances
- correctness testing in real runtime-host flows

So the right reading is:

> the Windows host now has first native service implementations for dialogs and external-open, but still needs real platform validation.

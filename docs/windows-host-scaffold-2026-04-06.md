# Windows host scaffold note — 2026-04-06

This note records the first explicit Windows host landing zone added after the runtime-host seam work.

Created scaffold files:
- `src/win/cef_runtime_window_host_win.h`
- `src/win/cef_runtime_window_host_win.cc`

These files are intentionally **not** wired into the build yet.
They exist to make the next Windows work concrete instead of abstract.

---

# What the scaffold is for

The new `CefRuntimeWindowHost` seam is now real enough that Linux host behavior no longer has to be the only currently wired implementation shape.

The Windows scaffold provides:
- a named future Windows host type
- a place to implement Win32/native host behavior
- a checklist-by-shape of what the Windows host must eventually satisfy

It currently contains only placeholder state and stub methods.
That is intentional.

---

# What remains before it can be used

## 1. Build-system wiring
The Windows host scaffold is not yet in CMake target source lists.
That should happen only once the platform-selection story is explicit enough to avoid muddy partial wiring.

## 2. Native window implementation
The Windows host still needs:
- Win32 window creation
- message loop integration
- parent handle management for OSR
- focus/resize lifecycle

## 3. Frame presentation
The Windows host stub does not yet present frames to a native surface.
That will be one of the first real implementation tasks.

## 4. Native input plumbing
The Windows host still needs native keyboard/mouse/wheel translation.

## 5. Platform services
The seam now expects hosts to own:
- file dialog behavior
- external URL handoff behavior

The Windows host currently returns `false` for both.

---

# Why this is still useful

Even as a stub, this scaffold is valuable because it changes the work from:
- “someday create a Windows host somehow”

to:
- “fill in this host implementation against the already-existing seam”

That is a healthier next step for a real Windows port.

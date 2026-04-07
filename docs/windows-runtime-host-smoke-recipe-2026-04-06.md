# Windows runtime-host smoke recipe — 2026-04-06

This recipe is for the **first real Windows smoke runs** of the BRIDGE runtime-host path.

It assumes the recent Windows work is present:
- platform target selection
- Windows host scaffold/source wiring
- Windows runner bootstrap skeleton
- native window skeleton
- crude OSR paint path
- first input plumbing
- first lifecycle handling
- native dialog/external-open implementations
- Windows-path observability logs

This is **not** a polished QA plan.
It is a practical bring-up checklist for the first few runs on an actual Windows machine.

---

# 1. Goal of the first smoke run

The first Windows smoke run is **not** trying to prove feature parity.
It is trying to answer these questions:

1. Does the Windows-target build launch at all?
2. Does a native window appear?
3. Does the runtime get through initialize/bootstrap/browser-create?
4. Does the first frame arrive?
5. Does the message loop stay alive long enough to observe behavior?
6. Do close/shutdown logs happen in the expected order?

If those answers are mostly yes, the smoke run is a success even if the browser is still rough.

---

# 2. What to build on Windows

Use the browser build with the Windows runtime target selected.

## Configure
Use the normal browser configure flow, but set:
- `-DENGINE_CEF_RUNTIME_TARGET_PLATFORM=windows`
- `-DBRIDGE_ENABLED_ENGINES=custom;cef`

For the current Windows runtime-host smoke path, do **not** leave Chromium enabled implicitly via the default engine list.
The active target here is the hybrid `custom + cef` path.

## Build targets
At minimum build:
- `engine_cef_runtime_host`
- `browser_cef_runtime_probe`
- `browser_cef_runtime_browser`
- `browser`

---

# 3. First-run order

Do the first smoke runs in this order.

## Run 1 — minimal startup smoke
Goal:
- confirm runtime starts
- confirm native window appears
- confirm logs show bootstrap progression

Expected good outcomes:
- process launches
- native window appears
- logs show initialize/bootstrap milestones
- if browser create fails, the failure is explicit and localized

## Run 2 — simple page load smoke
Goal:
- confirm browser create path works far enough to request first paint

Recommended target:
- BRIDGE Home or a very simple local/file/data page

Expected good outcomes:
- window appears
- first-frame log appears
- crude paint path shows something visible

## Run 3 — interaction smoke
Goal:
- confirm pointer + key forwarding basically works

Try:
- click in page
- scroll wheel
- basic key input
- reload shortcut
- back/forward if navigation exists

## Run 4 — close/shutdown smoke
Goal:
- confirm lifecycle path is sane

Try:
- close window normally
- watch whether logs show browser-driven close → destroy → quit-loop sequence

---

# 4. What logs to watch for

The new Windows-path observability should emit useful breadcrumbs.

## From `CefRuntimeRunnerWin`
Good signs:
- `Run begin`
- cache-root log
- `CefInitialize succeeded`
- `initial browser created`
- later: `message loop exited`
- later: `shutdown complete`

Bad signs:
- `CefInitialize failed`
- `CreateInitialBrowser failed ...`
- process exits with no useful breadcrumb after `Run begin`

## From `CefRuntimeWindowHostWin`
Good signs:
- `Initialize begin`
- `native window created and shown`
- `Initialize success`
- `browser attached to runtime window host`
- `first frame received: ...`
- on shutdown: `WM_CLOSE received`, `browser closing notification received`, `WM_DESTROY received`, `requesting CefQuitMessageLoop from WM_DESTROY`

Bad signs:
- window never appears after initialize logs
- no browser-attach log after runner says browser created
- no first-frame log even though browser should be alive
- close logs happen in a confusing or partial order

---

# 5. What counts as a GOOD failure

A good first-run failure is one that tells us where the next work is.

Examples:
- native window appears, but no first frame arrives
- browser create fails with a clear runtime error
- first frame arrives, but input is broken
- input works somewhat, but close path is flaky
- dialog/external-open path is invoked but behaves incorrectly

These are useful because they localize the next task.

---

# 6. What counts as a BAD failure

Bad failures are the ones that still leave us blind.

Examples:
- immediate silent exit
- no useful logs at all
- hard crash before any runner/host breadcrumbs
- inconsistent close behavior with no trace
- native window never appears and there is no bootstrap error

If one of these happens, fix observability or launch-path clarity before chasing polish.

---

# 7. First interaction checklist

Once the window appears and first frame arrives, test these in order:

1. Click into the page
2. Move mouse around
3. Scroll wheel
4. Type into a focused page field if available
5. `F5`
6. `Ctrl+R`
7. `Alt+Left` / `Alt+Right`
8. `Ctrl+T`
9. `Ctrl+W`
10. `Ctrl+Shift+T`
11. `Ctrl+Tab` / `Ctrl+Shift+Tab`
12. `Alt+1..9`
13. Close window

Do **not** treat every failure equally.
The key thing is to note the *first* broken layer:
- launch
- create
- paint
- input
- accelerators
- lifecycle

---

# 8. Suggested note-taking template for the first real smoke run

Use a short template like this:

## Environment
- Windows version:
- compiler/toolchain:
- CEF package used:
- configure command:
- build command:
- executable launched:

## Observed
- window appeared? yes/no
- first-frame log? yes/no
- browser visible? yes/no
- click input? yes/no
- wheel input? yes/no
- key input? yes/no
- reload shortcut? yes/no
- close path sane? yes/no

## Logs
- runner milestones reached:
- host milestones reached:
- final runtime error (if any):

## First clear blocker
- one sentence summary:

This keeps the first runs focused and comparable.

---

# 9. Recommended triage priority after the first Windows run

When the first real Windows smoke data comes back, prioritize fixes in this order:

1. launch/bootstrap failure
2. browser create failure
3. no first frame
4. close/shutdown crashes or hangs
5. no input
6. broken shortcuts
7. dialogs/external-open quirks
8. polish issues

That ordering prevents us from wasting time polishing a path that still has deeper runtime problems.

---

# 10. Bottom line

A successful first Windows smoke run does **not** need full browser polish.
It only needs to prove that the runtime-host lane now behaves like a real Windows engineering track:
- launchable
- observable
- debuggable
- incrementally fixable

If the first run gives us a visible window, clear logs, and a localized blocker, that is already a win.

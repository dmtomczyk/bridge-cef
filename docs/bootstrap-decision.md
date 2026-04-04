# Bootstrap decision for the first embedder spike

## Decision

Use the **CEF binary distribution** and a **small CMake host app** as the first serious long-term Chromium spike.

## Runtime style preference

Prefer **Alloy-style runtime** for the first browser window if the bring-up cost remains reasonable.

### Why Alloy-style first

- it is based on the Chromium content layer rather than the full Chrome UI layer
- it better matches the project direction where the shell owns browser chrome/UI decisions
- it avoids baking in assumptions that the full Chrome runtime UI should be part of the final product shape

### Practical caveat

If Alloy-style runtime creates unnecessary bring-up friction for the *very first* windowed proof, a temporary fallback to the default runtime style is acceptable for bootstrap only.

That fallback is allowed only to get the first window and page visible quickly. The spike should still record Alloy-style runtime as the intended target unless new evidence clearly argues otherwise.

## Rendering mode

Start with **windowed rendering**, not off-screen rendering.

### Why

- fastest path to a native embedded browser proof
- avoids rebuilding the same off-screen/frame-ownership complexity too early
- lets the team evaluate whether the more native integration already solves the main responsiveness problem

## Distribution strategy

Start from the official-style CEF binary distribution / sample-project workflow rather than building Chromium source.

### Why

- much healthier feedback loop than Chromium source modification/rebuild
- better fit for the current spike objective
- lower setup cost for the first proof

## What we are explicitly *not* doing in Phase 1

- patching Chromium source
- building a custom Chromium binary
- implementing a custom raw-frame transport
- migrating the whole shell
- reproducing the current screenshot backend under a different name

## Exit criteria for this bootstrap decision

This bootstrap choice stands unless one of these happens:

1. the binary distribution route is operationally broken on this machine/project
2. Alloy-style runtime creates hard blockers for first-window bring-up
3. the embedder route proves clearly incompatible with the shell architecture we want

If any of those become true, record it explicitly before changing course.

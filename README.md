# engine-cef

CEF-backed browser engine repo for Bridge.

## Purpose

This repo is the active long-term target backend for Chromium-based rendering after the MVP/reference `engine-chromium` bring-up succeeded but proved too tied to the `headless_shell` + DevTools + screenshot path.

## Current status

Early bootstrap, but already past the purely speculative stage:

- builds against a Linux x64 CEF binary distribution
- launches a native browser window
- loads `https://example.com`
- supports basic manual clicking

See:
- `BUILD.md`
- `docs/first-native-proof.md`
- `docs/bootstrap-decision.md`
- `docs/cef-distribution-bootstrap.md`
- `docs/client-integration-bridge.md`
- `docs/cross-repo-contract-v1.md`

## Dependency model

`engine-cef` depends on an external CEF binary distribution via `CEF_ROOT`.

The CEF SDK/runtime should stay **outside** the repo, for example:

- `~/code/sdks/cef/cef_binary_<version>_linux64/`

or similar.

## Near-term goal

Get from the first native proof to a stable backend module that can be integrated into the Bridge shell deliberately, without reintroducing the screenshot/PNG architecture from the old reference backend.

## Current code shape

`engine-cef` now has two layers:

- `engine_cef_proof` — the standalone native CEF proof app used for bring-up
- `engine_cef_core` — the backend core/contract plus a small integration bridge that a future client integration can talk to

See:
- `docs/initial-backend-contract.md`
- `docs/migration-slices.md`

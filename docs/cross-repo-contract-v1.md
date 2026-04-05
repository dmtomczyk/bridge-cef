# Cross-repo contract v1 (`client` <-> `engine-cef`)

## Purpose

Define the first narrow public API that `client` can eventually consume from `engine-cef`.

This contract is intentionally small and is exposed via public headers under:

- `include/engine_cef/types.h`
- `include/engine_cef/integration_bridge.h`

## Public types

- `InitParams`
- `PageState`
- `LoadState`
- `BackendSnapshot`

## Public interface

- `IIntegrationBridgeObserver`
- `IIntegrationBridge`
- `create_integration_bridge()`

## Why this is the first useful contract

It gives `client` something concrete to target without importing proof-specific classes or screenshot-era assumptions.

The client can eventually:

- create a bridge
- initialize it
- drive navigation commands
- observe snapshot changes
- query the latest snapshot/debug summary

## Explicit non-goals

This v1 contract does **not** settle:

- tab/window ownership
- advanced input/focus strategy
- final backend factory plumbing in the client
- whether additional native-view integration helpers are needed later

## Current role

This contract is the first deliberately stable handshake between the `client` repo and the `engine-cef` repo.

## Follow-on note

The next seam after this v1 control/state bridge is described in:

- `docs/presentation-seam-v2.md`

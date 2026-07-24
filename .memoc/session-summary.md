---
memoc: true
type: state
scope: project-memory
updated: 2026-07-24T16:18:00+09:00
status: active
tags:
  - memoc
  - memoc/state
created: 2026-07-24T01:26:27
---
# Session Summary

- `BuildCookDeployFinal.bat` builds/cooks Shipping client and server into separate versioned folders under `Saved/FinalDeploy`.
- It blocks dirty releases by default, stages prerequisites, verifies executable/container files plus `BP_RunPortal`, and preserves incomplete output for diagnostics.
- Shipping server logging uses a unique build environment; release launch omits `-AllowLobbyForceStart`.
- Shipping and Development preflight checks pass. The full build/cook has not run.
- Eight pre-existing user asset/VillageLayoutDirector changes remain uncommitted and are excluded from this deployment-script commit.

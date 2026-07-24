---
memoc: true
type: state
scope: project-memory
updated: 2026-07-24T11:24:00+09:00
status: active
tags:
  - memoc
  - memoc/state
created: 2026-07-24T01:26:27
---
# Session Summary

- Village layout snapshots replicate exact seven-instance layouts; packaged clients stream matching levels and generate visual PCG locally.
- `UnrealEditor.exe -game` can crash while animation PostLoad calls an editor transaction with null `GEditor`; use the new cooked Game-client scripts instead.
- Game/Editor targets and the Win64 client package build successfully. A packaged null-RHI client connected to the cooked landscape server, applied revision 1, and completed PCG 7/7 without the crash.
- The package is under `Project_Eden/Saved/DedicatedClient/Windows`.
- Remaining cook warning: `M_StateMask` exceeds the SM5 16-sampler limit; SM6 is valid.

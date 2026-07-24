---
memoc: true
type: state
scope: project-memory
updated: 2026-07-24T11:54:00+09:00
status: active
tags:
  - memoc
  - memoc/state
created: 2026-07-24T01:26:27
---
# Session Summary

- Packaged Game clients safely stream the replicated seven-village layout and complete visual PCG 7/7; avoid `UnrealEditor.exe -game` for this flow.
- Development dedicated-server solo start is restored behind `-AllowLobbyForceStart`. Local cooked server/client scripts add the flag; Shipping and unflagged launches remain blocked.
- Lobby force-start policy test and Editor/Game/Server builds pass.
- Existing cooked client/server inner binaries were refreshed from the verified builds.
- Remaining cook warning: `M_StateMask` exceeds the SM5 16-sampler limit; SM6 is valid.

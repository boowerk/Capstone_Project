---
memoc: true
type: state
scope: project-memory
updated: 2026-07-24T15:38:00+09:00
status: active
tags:
  - memoc
  - memoc/state
created: 2026-07-24T01:26:27
---
# Session Summary

- `PCG_Vegetation_Global` corrects the tree-only Difference flow while retaining village exclusion and removes its RegionID texture dependency.
- WindowsServer strips texture pixel payloads even with CPU Availability, so server PCG must use server-cookable graph/world data.
- User verified vegetation now appears while connected to the dedicated server. The proposed `AGP_GameMode` server-disable code was discarded; the graph remains active on both targets.

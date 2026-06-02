---
memoc: true
type: state
scope: project-memory
created: 2026-06-01T04:41:15
updated: 2026-06-01T04:41:15
status: active
tags:
  - memoc
  - memoc/state
updated: 2026-05-31T15:20:00+09:00
created: 2026-05-28T12:18:30
---
# Session Summary
Last: 2026-06-02T20:51:00+09:00
Replace, do not append. Keep <800B.
History: worklog. Resume risks: 04-handoff.md.

## Status
- Primary melee has short lock-on style auto-facing/camera interpolation without enabling `bIsLockOn`.
- Dash now snapshots active `UGP_Primary` before cancellation; during primary melee it plays `SourceDodgeMontages.Left_RM/Right_RM` instead of roll. Normal dash still uses roll/source roll.
- Live Coding stays enabled, but project plugin preloading is disabled so `McpAutomationBridge` should be lazy-loaded instead of enabled for Live Coding rebuilds. `Project_EdenEditor Win64 Development` built successfully with UBT after closing the editor.

## Open Tasks
- PIE-test primary melee auto-facing and melee dodge feel.

## Resume
- Local project user settings include `[/Script/LiveCoding.LiveCodingSettings] bEnabled=True`, `bPreloadProjectModules=True`, `bPreloadProjectPluginModules=False`.

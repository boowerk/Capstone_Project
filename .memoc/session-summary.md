---
memoc: true
type: state
scope: project-memory
created: 2026-06-01T04:41:15
updated: 2026-06-03T04:12:00+09:00
status: active
tags:
  - memoc
  - memoc/state
---
# Session Summary
Last: 2026-06-03T04:48:00+09:00
Replace, do not append. Keep <800B.
History: worklog. Resume risks: 04-handoff.md.

## Status
- No UBT/build per user while editor open.
- Primary melee uses source fallback A-D; PDA target light montages are None. Source `_Rec` recovery resolves by name for A/B/C; no D Rec found.
- User manually appended recovery sequences into source attack montages. Primary no longer resolves or plays separate `_Rec` montages; no recovery hold/blend timers remain.
- `ActionEnd` only opens queued-combo branching. No input lets the current attack montage continue through its embedded recovery and complete normally. Source fallback completion timer uses full montage duration again.
- Combo action-motion policy: only index `2` uses action/root-motion tracking; 1/2/4 use lower-body MM blend + movement assist.

## Open
- Need PIE/editor visual test after Live Coding/C++ refresh; no build was run.

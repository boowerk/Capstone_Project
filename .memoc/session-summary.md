---
memoc: true
type: state
scope: project-memory
status: active
tags:
  - memoc
  - memoc/state
updated: 2026-06-03T19:04:50+09:00
created: 2026-05-28T12:18:30
---
# Session Summary
Last: 2026-06-03T19:04:50+09:00
Replace, do not append. Keep <800B.

## Status
- Augment `TargetSkillTags` empty now means all skills.
- `RequiredElementTag` gates augment modifiers by current tech element; `GrantedElementTag` still changes tech.
- `CooldownMultiplier` affects SkillBase cooldowns and DashSlash fallback cooldown.
- `ProjectileCountBonus` affects SplitShot, NetTestProjectile, and ThrownBurst.

## Verify
- `git diff --check` passed for touched source files (CRLF warnings only).
- Full `Project_EdenEditor Win64 Development` build succeeded; only pre-existing warnings.

---
memoc: true
type: state
scope: project-memory
status: active
tags:
  - memoc
  - memoc/state
updated: 2026-06-02T00:00:00+09:00
created: 2026-05-28T12:18:30
---
# Session Summary
Last: 2026-06-02T00:00:00+09:00
Replace, do not append. Keep <800B.

## Status
- Augment DA now has `EGP_SkillAugmentType`: BasicAttack, Skill, Ultimate, Passive.
- `UGP_AugmentSelectWidget` maps augment type to card backgrounds, with defaults Dawn/Dusk/Midnight/Zenith.
- `WBP_TestAugmentSelect_1` card bg widgets renamed to `Image_CardBg0/1/2`.
- Missing augment icons now use `Hidden`, not `Collapsed`, so text does not move upward.

## Verify
- WBP rename/save succeeded through Unreal Python.
- UBT/editor rebuild not run: Unreal MCP build/query calls were rejected by automatic approval review.

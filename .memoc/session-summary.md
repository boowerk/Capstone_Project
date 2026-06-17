---
memoc: true
type: state
scope: project-memory
created: 2026-06-17T04:47:15
updated: 2026-06-18T00:27:00+09:00
status: active
tags: [memoc, memoc/state]
---
# Session Summary
Last: 2026-06-18T00:27:00+09:00
Replace, do not append. Keep <800B.

## Status
- Matador body floats/does nothing; active decoy owns melee, bull lure/redirect/counter loop.
- `BP_Boss_Matador` uses `BP_MatadorDecoy`.

## Changed
- Bull decoy pass now moves by cubic curve delta (`EvaluateCurve(next)-EvaluateCurve(prev)`) instead of free-steering, so the path actually reaches the player-facing exit.
- Decoy-to-player redirect stores `LockedRedirectDirection`, then turns toward it via `PlayerRedirectTurnInterpSpeed` instead of snapping; yellow debug line marks the fixed direction.
- Decoy/near-decoy blocking now treats decoy-owned/attached/nearby hits as redirect-safe via `IsBlockingHitSafeNearDecoy`, reducing bull deletion during lure/pass.
- Bull spawn now uses decoy-player axis with side offset 450cm; curve side strengths tuned to P1=0.32/P2=0.16 for slight decoy pass without large miss.
- Bull player impact no longer destroys/finishes bull. It applies damage once per hit actor, launches player along charge direction, sends hit-react event, and bull keeps charging.
- Player attack counter no longer depends on valid GE/ASC; if hit detects bull it calls `TryRedirectTowardDecoy`, logs success/reject reason, and locks bull direction to decoy.
- Body collision remains damage/knockback only.

## Resume
- Compile in editor. Test bull body collision, primary counter, and decoy orbit feel.

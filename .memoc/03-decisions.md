---
memoc: true
type: state
scope: project-memory
created: 2026-05-21T07:03:24
updated: 2026-06-23T04:05:00+09:00
status: active
tags:
  - memoc
  - memoc/state
---
# Decisions

Durable project decisions live here. Keep entries short, dated, and useful to future agents.

## Decision Log

### 2026-05-20
- Use `UEFNSourceMesh` as the runtime animation-driving source and let `CharacterMesh0`/MaskMan retarget from it.
- Prefer existing UEFN mannequin pose-search assets before building new databases.
- Keep chooser-driven database selection as the long-term direction, but keep explicit graph DB/state variables in `UGP_CharacterAnimInstance` because the current ABP graph uses fixed DB variables, not `RuntimePoseSearchDatabase` alone.
- Use the Blueprint `CharacterTrajectoryComponent` path for motion-matching trajectory when available.

### 2026-05-23
- Use `GP_CharacterAnimInstance` as the chooser context type instead of a specific ABP class so source/target anim instances do not type-mismatch.
- Treat MaskMan default movement speed around `500` as run-family motion and sprint around `700` as sprint-family motion.
- Use a dedicated `SprintSpeedThreshold` instead of broad run threshold reuse.

### 2026-06-04
- Add crouch locomotion by reusing existing `Stance` and crouch PSDs in `CHT_MM_MaskMan_Root_OriginalStyle`.
- Missing sparse crouch idle/TIP assets may fall back to Dense or Extreme Sparse PSDs.
- Hold crouch uses `IA_Crouch` on `C`, controller `Crouch()`/`UnCrouch()`, and character `bCanCrouch`.
- For crouch/uncrouch, do not patch visible dipping by stacking mesh component Z corrections. Leave mesh components alone first, then identify whether movement comes from capsule/root, attachment hierarchy, Retarget Pose From Mesh, or animation/root offsets.
- Prefer `Capsule -> UEFNSourceMesh` and `Capsule -> CharacterMesh0` sibling hierarchy over `Capsule -> UEFNSourceMesh -> CharacterMesh0`; preserve retargeting by connecting `ABP_MaskMan_Player` Retarget Pose From Mesh `SourceMeshComponent` to `RetargetSourceMesh`, filling it from C++, and ticking `UEFNSourceMesh` before `CharacterMesh0`.

### 2026-06-18
- Supersede the dedicated Crystal Seraph attack/idle tree: enforce `BT_BossCommon`/`BB_BossCommon` so patrol, chase, and reposition behavior stays shared while GAS scoring specializes attacks.
- Gate Crystal Seraph damage/teleport patterns through one boss-owned minimum cadence, but let the groggy state reaction bypass it.

### 2026-06-19
- Keep Sans Ground Hands in the shared boss GAS selector: the ability owns wave scheduling and cooldown, while replicated strike actors own telegraph, motion, damage, and launch presentation.

### 2026-06-20
- Enemy death follows the GAS boundary: `UGP_AttributeSet` detects terminal Health, `UGP_EnemyDeathAbility` orchestrates the server transition, and `AGP_EnemyCharacter` owns AI shutdown, collision/movement disablement, replication, and despawn.
- Keep death presentation separate from death rules. `BP_OnDeathStarted` and `OnEnemyDeathStarted` are optional hooks; no Blueprint setup is required for death correctness.
- Regular enemies own one native screen-space health bar through `AGP_EnemyCharacter`; `UGP_WidgetComponent` binds the shared Health/MaxHealth pair. Bosses remain on the dedicated HUD boss bar to avoid duplicate UI.

### 2026-06-21
- Keep Dark Armor Knight on shared Boss_Common navigation/targeting while its state component owns guard/parry/groggy truth, AI only scores ability tags, and GAS abilities/pattern actors own damage and collision. Use Blueprintable Engine-shape actors as replaceable art seams.
- Superseded by `93ac0b15`: crossing `ReturnHomeDistance` starts anchor return even with an active target. A visible player can re-engage after the enemy returns inside 75% of the outer distance, preventing boundary oscillation.
- Minimap FollowTarget alone owns periodic capture; FullMap capture is event-driven. Use orthographic FinalColorLDR for UMG-safe opaque alpha, then exclude lighting, shadows, translucent/particle VFX, decals, and debug overlays for 2D readability.
- Never swap or rebind the minimap RenderTarget sampled by UMG during periodic capture. Keep one stable display target, capture into an isolated back buffer, then GPU-copy completed pixels to the display target; hide the followed player because the HUD arrow already represents it.
- Advance the minimap transfer through capture-fence and display-copy-fence stages. Each RHI fence must have issued its write, drained pending writes, and passed `Poll()`; while incomplete, preserve the last displayed pixels and do not reuse the back buffer.
- Crystal Seraph reflection is the three-stage break mechanic. The third reflection enters groggy and falls; only the first player hit after landing starts the recovery delay, while Boss_Common remains paused.
- Keep Boss_Common assets shared, but enforce air/leash safety natively: flying vector MoveTo bypasses ground pathfinding while preserving altitude, Patrol yields when ReturnHome is active, and Crystal Seraph tactical teleports stay inside the anchor margin.
- Supersede periodic FollowTarget minimap capture with one post-PCG full-map capture. Keep the resulting RenderTarget fixed, disable SceneCapture after its fenced GPU copy, and move pan/zoom plus player/enemy marker presentation into C++/UMG.

### 2026-06-22
- Resolve player SkillData and actor-owned boss Niagara through the same cue/element specificity function. Pattern actors own persistent effect lifetime; authoritative gameplay events multicast cosmetic one-shots.

### 2026-06-23
- Reinterpret the RegionState system as biome-type selection, not gameplay life/death/corruption status. Values should represent biome categories; GameMode terms like `AliveRegionState` / `DeadRegionState` are legacy naming to rename or replace when implementation resumes.

---
memoc: true
type: raw
scope: project-memory
created: 2026-05-21T06:58:20
updated: 2026-05-21T06:58:20
status: active
tags:
  - memoc
  - memoc/system
  - memoc/raw
---
# AI System

## Purpose

Current inventory of enemy AI C++ and Behavior Tree/EQS assets.

## C++ Core

- `AEnemyAIController`: owns AI perception, target selection, behavior tree/blackboard initialization, leash return-home state, and LLM/archetype evaluation application to Blackboard.
- `AGP_EnemyCharacter`: provides behavior anchor, AI ranges, perception values, BT/Blackboard overrides, enemy archetype DataAsset/row, and boss metadata.
- `UEnemyArchetypeData`: `UPrimaryDataAsset` wrapping `FEnemyArchetypeTuning`.
- `FEnemyArchetypeTuning`: includes base `FEnemyLLMEvaluation`, personality variation settings, and seed offset.
- `FEnemyLLMEvaluation`: tuning/evaluation structs and enums used by AI controller and services.

## Behavior Tree and Blackboard Assets

- `/Game/Characters/EnemyCharacter/BT/Common/BT_EnemyCommon`
- `/Game/Characters/EnemyCharacter/BT/Common/BB_EnemyCommon`
- `/Game/Characters/EnemyCharacter/BT/Boss/BT_BossCommon`
- `/Game/Characters/EnemyCharacter/BT/Boss/BB_BossCommon`
- `/Game/Characters/EnemyCharacter/BT/DA_EnemyArchetypeData`

## EQS Assets

- `/Game/Characters/EnemyCharacter/EQS/EQS_FindCombatPosition`
- `/Game/Characters/EnemyCharacter/EQS/EQS_FindRetreatPosition`
- `/Game/Characters/EnemyCharacter/EQS/EQS_PatrolLocation`
- `/Game/Characters/EnemyCharacter/EQS/EQS_RepositionLocation`
- `/Game/Characters/EnemyCharacter/EQS/EQS_RetreatLocation`

## Enemy Blueprints

- `/Game/Characters/EnemyCharacter/BP_Enemy_Base`
- `/Game/Characters/EnemyCharacter/BP_Enemy_Melee`
- `/Game/Characters/EnemyCharacter/BP_Enemy_Ranged`
- `/Game/Characters/EnemyCharacter/Animation/ABP_Enemy_Ranged`

## C++ Task/Service Areas

- BT tasks include patrol, reposition, retreat, execute attack, run EQS query, and boss-specific attack/phase/summon tasks.
- BT services include enemy tactics and boss tactics updates.
- EQS contexts include enemy home location and enemy target actor.
- Debug support includes enemy AI range visualization.

## Verification Limits

- BT graphs, Blackboard keys, EQS query internals, and placed enemy settings were not opened in Unreal Editor.
- AI runtime behavior was not tested.

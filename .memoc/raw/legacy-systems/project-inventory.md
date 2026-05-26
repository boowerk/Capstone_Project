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
# Project Inventory

## Purpose

High-level inventory of the current project structure. This page is based on live files, not backed-up docs.

## Repository Shape

- Repository root: `C:\Users\dyk66\Desktop\Capstone_Project`
- Unreal project root: `Project_Eden/`
- UProject: `Project_Eden/Project_Eden.uproject`
- Content root: `Project_Eden/Content/`
- Source root: `Project_Eden/Source/Project_Eden/`

## Content Top-Level Folders

- `Actors`
- `Asset`
- `Characters`
- `Collections`
- `Developers`
- `Fab`
- `GAS_Pattern`
- `Items`
- `Maps`
- `ParagonGideon`
- `PCG`
- `RegionSystem`
- `Skills`
- `UI`
- `__ExternalActors__`

## Asset Counts

- `.uasset`: 4472
- `.umap`: 18

## Config Defaults

Verified from `Project_Eden/Config/DefaultEngine.ini`:

- `GameDefaultMap=/Game/Maps/DemoMap/RegionMap.RegionMap`
- `ServerDefaultMap=/Game/Maps/DemoMap/ServerEmptyTest.ServerEmptyTest`
- `GlobalDefaultGameMode=/Game/GAS_Pattern/Game/BP_ProjectEden_Gamemode.BP_ProjectEden_Gamemode_C`
- `EditorStartupMap=/Game/Maps/DemoMap/BiomeBlendingTestMap.BiomeBlendingTestMap`

## Maps Under `/Game/Maps`

- `/Game/Maps/DemoMap/BiomeBlendingTestMap`
- `/Game/Maps/DemoMap/BuildingTestMap`
- `/Game/Maps/DemoMap/CityMap`
- `/Game/Maps/DemoMap/GAS_TestMap`
- `/Game/Maps/DemoMap/PathfindingMap`
- `/Game/Maps/DemoMap/RegionMap`
- `/Game/Maps/DemoMap/RegionMap_LargeTest`
- `/Game/Maps/DemoMap/RegionMap_LargeTest_PCGSaveTest`
- `/Game/Maps/DemoMap/ServerEmptyTest`
- `/Game/Maps/DemoMap/TestMap`
- `/Game/Maps/EventMap/EventMap`
- `/Game/Maps/MainMap/L_MainMap`

## Source Areas

- `AbilitySystem`: GAS attributes, abilities, damage execution.
- `Actors`: projectile and water puddle actors.
- `AI`: AI controller, BT tasks/services, EQS contexts, LLM/archetype data.
- `Animation`: animation instances, animation setup data, gameplay event notify.
- `Characters`: base, player, enemy character classes.
- `Commandlets`: female animation setup commandlet.
- `GameplayTags`: gameplay tag definitions.
- `Interfaces`: summonable interface.
- `Items`: weapon item types.
- `PCG`: PCG controller actor and biome/item DataAssets.
- `Player`: player controller and player state.
- `UI`: HUD, attributes, damage number, widget component classes.
- `Utils`: blueprint and animation setup libraries.

## Verification Limits

- Blueprint structure, selected DataAsset values, selected Material instance parameters, and selected map actor placement were checked through the running Unreal Editor MCP/Python bridge.
- Full visual Blueprint graphs, PCG graph internals, Material graph nodes, generated PCG output, Blueprint compile, PIE, and C++ build remain unverified.
- See `.memoc/systems/final-inspection.md` for the deeper read-only inspection.

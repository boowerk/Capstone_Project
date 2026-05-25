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
# Unreal Project

## Purpose

Durable high-level map for the Unreal project layout. This page records only facts verified from current files.

## Current State

- Repository root: `C:\Users\dyk66\Desktop\Capstone_Project`
- Unreal project root: `Project_Eden/`
- UProject: `Project_Eden/Project_Eden.uproject`
- Engine association: `ProjectEden_Engine`
- Runtime module: `Project_Eden`
- Additional dependency listed in the module entry: `GameplayAbilities`

## Main Directories

- `Project_Eden/Source/`: Unreal C++ source.
- `Project_Eden/Config/`: Unreal config files.
- `Project_Eden/Content/`: Unreal content/assets.
- `Project_Eden/Plugins/`: project plugins; currently includes `McpAutomationBridge`.
- `Project_Eden/Scripts/`: project scripts.

## Enabled Plugins

- `ModelingToolsEditorMode` for Editor target only.
- `PCG`
- `PCGGeometryScriptInterop`
- `StateTree`
- `GameplayAbilities`
- `PCGBiomeCore`
- `PCGBiomeSample`
- `PCGExtendedToolkit`
- `Landmass`
- `AnimationWarping`

## Source Areas Observed

- `AbilitySystem`
- `Actors`
- `AI`
- `Animation`
- `Characters`
- `Commandlets`
- `GameplayTags`
- `Interfaces`
- `Items`
- `PCG`
- `Player`
- `UI`
- `Utils`

## Caution

- Do not assume old backed-up documentation is correct.
- Inspect current source, configs, Blueprint references, PCG graphs, DataAssets, and Material parameters before making changes.
- Unreal asset moves or renames should be handled through Unreal Editor workflows when references matter.

## Verification

- Verified by reading `Project_Eden/Project_Eden.uproject`.
- Verified by listing `Project_Eden/Source`, `Project_Eden/Config`, and `Project_Eden/Plugins`.
- Unreal Editor/build/PIE/Blueprint/PCG/Material verification has not been run.

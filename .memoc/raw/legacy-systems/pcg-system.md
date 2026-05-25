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
# PCG System

## Purpose

Inventory of the non-RegionSystem PCG assets and C++ support currently visible in the project.

## C++ Support

Observed source files:

- `Project_Eden/Source/Project_Eden/Public/PCG/PcgControllerActor.h`
- `Project_Eden/Source/Project_Eden/Public/PCG/PcgDataTypes.h`
- `Project_Eden/Source/Project_Eden/Public/PCG/PCGStructs.h`
- `Project_Eden/Source/Project_Eden/Public/PCG/PDA_Biome.h`
- `Project_Eden/Source/Project_Eden/Public/PCG/PDA_PCGItemGroup.h`

Key verified roles from headers:

- `APcgControllerActor` owns an `OpenAIRequester` and exposes `ApplyPcgParameters(const FPCGScatterParams& Params)` as a BlueprintImplementableEvent.
- `UPDA_Biome` is a `UPrimaryDataAsset` with `BiomeName` and a map of item groups.
- `UPDA_PCGItemGroup` is a `UPrimaryDataAsset` containing `FPCGItemDetails` entries.
- `FPCGItemDetails` stores mesh, spawn weight, scale range, rotation range, and offset range.

## PCG Asset Groups

Test graphs:

- `/Game/PCG/Graphs/Test/BP_PCG_BiomeController`
- `/Game/PCG/Graphs/Test/PCG_Graph_BiomeMaster`
- `/Game/PCG/Graphs/Test/PCG_Test_A`
- `/Game/PCG/Graphs/Test/PCG_Test_B`

Biome blending:

- `/Game/PCG/PCG_BiomeBlend/BP_SplineBiome`
- `/Game/PCG/PCG_BiomeBlend/BP_SplineRoad`
- `/Game/PCG/PCG_BiomeBlend/PCG_BiomeSystem`
- `/Game/PCG/PCG_BiomeBlend/PCG_SplineBiome`
- `/Game/PCG/PCG_BiomeBlend/PCG_SplineRoad`
- `/Game/PCG/PCG_BiomeBlend/Data/PDA_Biome`
- `/Game/PCG/PCG_BiomeBlend/Data/DA_Desert`
- `/Game/PCG/PCG_BiomeBlend/Data/DA_Forest`
- `/Game/PCG/PCG_BiomeBlend/Data/DA_RockyConifer`
- `/Game/PCG/PCG_BiomeBlend/Data/S_Biome`
- `/Game/PCG/PCG_BiomeBlend/FalloffCurves/C_Linear`
- `/Game/PCG/PCG_BiomeBlend/FalloffCurves/C_LinearDown`
- `/Game/PCG/PCG_BiomeBlend/FalloffCurves/C_LinearUP`

Building generation:

- `/Game/PCG/PCG_Building_Generation/ExtractMeshInfo`
- `/Game/PCG/PCG_Building_Generation/PCG_BuildingSample`
- `/Game/PCG/PCG_Building_Generation/PCG_PrimitiveCrossSection_Grammar`

Pathfinding:

- `/Game/PCG/PCG_PathFinding/BP_Building`
- `/Game/PCG/PCG_PathFinding/DA_Road`
- `/Game/PCG/PCG_PathFinding/PCGEx_Pathfinding`
- `/Game/PCG/PCG_PathFinding/PCG_Building`
- `/Game/PCG/PCG_PathFinding/PCG_RodomSeed`

## Verification Limits

- PCG graph asset paths and supporting C++ headers were inspected.
- Biome DataAsset `BiomeInfo` counts were inspected through Unreal Python; see `.memoc/systems/final-inspection.md`.
- PCG graph nodes, pins, graph parameters, and generated output were not fully opened or run.
- A broad MCP graph-analysis call for a PCG graph timed out; use focused graph inspection in Unreal Editor for behavior-level work.
- Verify graph internals in Unreal Editor before changing PCG behavior.

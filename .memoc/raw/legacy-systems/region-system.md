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
# Region System

## Purpose

Tracks the currently observed RegionSystem assets and likely responsibilities.

## Verified Asset Groups

Blueprints:

- `/Game/RegionSystem/Blueprints/BP_RegionSeed`
- `/Game/RegionSystem/Blueprints/BP_RegionStateManager`
- `/Game/RegionSystem/Blueprints/BP_VegetationSpawner`

City Blueprints:

- `/Game/RegionSystem/Blueprints/City/BP_CityAnchor`
- `/Game/RegionSystem/Blueprints/City/BP_CityDistrictSpline`
- `/Game/RegionSystem/Blueprints/City/BP_CityGenDistrictSpline`
- `/Game/RegionSystem/Blueprints/City/BP_CityRoadSpline`

Data:

- `/Game/RegionSystem/Data/ERegionBaseType`
- `/Game/RegionSystem/Data/City/Collections/DA_CityMainRoad`
- `/Game/RegionSystem/Data/City/Collections/DA_MainRoad`
- `/Game/RegionSystem/Data/Vegetation/PDA_RegionVegetation`
- `/Game/RegionSystem/Data/Vegetation/DA_NatureAliveVegetation`
- `/Game/RegionSystem/Data/Vegetation/DA_NatureCorruptedVegetation`
- `/Game/RegionSystem/Data/Vegetation/DA_NatureDeadVegetation`
- `/Game/RegionSystem/Data/Vegetation/S_VegetationEntry`

Materials and Textures:

- `/Game/RegionSystem/Materials/Masters/M_LandBlendState`
- `/Game/RegionSystem/Materials/Masters/M_StateMask`
- `/Game/RegionSystem/Materials/Instances/MI_RegionLandscape`
- `/Game/RegionSystem/Materials/Instances/MI_RegionLandscape_LargeTest`
- `/Game/RegionSystem/Materials/Instances/MI_RegionLandscape_MainMap`
- `/Game/RegionSystem/Materials/Functions/RegionFunction/MF_RS_GetRegionState`
- `/Game/RegionSystem/Materials/Functions/RegionFunction/MF_RS_StateEqualsMask`
- `/Game/RegionSystem/RenderTargets/RT_RegionState_20x1`
- `/Game/RegionSystem/RenderTargets/RT_RegionState_9x1`
- `/Game/RegionSystem/Textures/RegionID/T_MainMap_RegionID`
- `/Game/RegionSystem/Textures/RegionID/T_RegionMapLT_RegionID`
- `/Game/RegionSystem/Textures/RegionID/T_RegionMap_RegionID`
- `/Game/RegionSystem/Textures/Debug/T_RS_DebugGround_State_00`
- `/Game/RegionSystem/Textures/Debug/T_RS_DebugGround_State_01`
- `/Game/RegionSystem/Textures/Debug/T_RS_DebugGround_State_02`
- `/Game/RegionSystem/Textures/Debug/T_RS_DebugGround_State_03`

PCG:

- `/Game/RegionSystem/PCG/PCG_Vegetation_Global`
- `/Game/RegionSystem/PCG/PCG_City_FromAnchor`
- `/Game/RegionSystem/PCG/CityGen/PCG_CityGen`
- `/Game/RegionSystem/PCG/CityGen/PCG_CityGen1`
- `/Game/RegionSystem/PCG/CityGen/PCG_CityGen2`
- `/Game/RegionSystem/PCG/CityGen/PCG_CityGen_FromAnchor`
- `/Game/RegionSystem/PCG/CityGen/PCG_BuildingsWithConnections`
- `/Game/RegionSystem/PCG/CityGen/Loops/PCGLoop_GrammarByDistrict`
- `/Game/RegionSystem/PCG/CityGen/Loops/PCGLoop_RotationToTangent`
- `/Game/RegionSystem/PCG/CityGen/SubGraphs/SG_Building_PostProcess`
- `/Game/RegionSystem/PCG/CityGen/SubGraphs/SG_Driveway_Generation`
- `/Game/RegionSystem/PCG/CityGen/SubGraphs/SG_RoadTypeCheck`
- `/Game/RegionSystem/PCG/CityGen/SubGraphs/SG_SplitSpline`
- `/Game/RegionSystem/PCG/CityGen/SubGraphs/SG_TreeArea_Prepare`
- `/Game/RegionSystem/PCG/CityGen/SubGraphs/SG_TreePointGeneration`

## Current Understanding

- RegionSystem appears to own region seeds, region state management, landscape region visualization, vegetation PCG, and CityAnchor-based city generation assets.
- The default game map is `/Game/Maps/DemoMap/RegionMap`.
- Large test map assets exist at `/Game/Maps/DemoMap/RegionMap_LargeTest` and `/Game/Maps/DemoMap/RegionMap_LargeTest_PCGSaveTest`.
- MCP/Python inspection verified placed RegionSystem actors in `RegionMap` and `RegionMap_LargeTest`; see `.memoc/systems/final-inspection.md`.

## Current Review Risks

- `RegionMap` has 9 placed region seeds, but its placed `BP_RegionStateManager` reports `RegionCount=16` and uses `RT_RegionState_9x1`.
- `RegionMap_LargeTest` has 20 placed region seeds and `MI_RegionLandscape_LargeTest.RegionCount=20`, but its placed `BP_RegionStateManager` reports `RegionCount=16`.
- In `RegionMap_LargeTest`, seed labels and live `BaseType` values are not always semantically aligned; live property values must win over actor labels.
- In `RegionMap_LargeTest`, City_03 road/district tags were observed, but no City_03 anchor was detected in the inspected placed anchors.

## Verification Limits

- Blueprint variables/defaults/components, selected material instance parameters, DataAsset counts, and selected map actor placement were inspected through MCP/Python.
- PCG node internals, generated PCG output, material graph node internals, Blueprint compile, PIE, and build were not verified.
- Do not rename RegionSystem Blueprint, PCG, DataAsset, RenderTarget, Texture, or Material assets without checking references in Unreal Editor.

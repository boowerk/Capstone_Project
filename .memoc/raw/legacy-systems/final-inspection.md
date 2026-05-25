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
# Final Inspection

Last inspected: 2026-05-17T13:58:00

## Scope

This page records a deeper read-only inspection done against the running Unreal Editor through the MCP Automation Bridge and Unreal Python. It is based on live project assets, not the backed-up old docs.

## MCP / Editor Access

- Unreal Editor was running from `C:\Engine_server\Windows\Engine\Binaries\Win64\UnrealEditor.exe`.
- Native MCP responded at `http://127.0.0.1:3000/mcp`.
- MCP server reported `unreal-mcp` version `0.6.0`.
- `DefaultGame.ini` enables `bEnableNativeMCP=True` and lists bridge ports `8090,8091`.
- Useful verified actions: `manage_blueprint.get_graph_details`, `manage_blueprint.get_scs`, `inspect.inspect_cdo`, `system_control.execute_python`, `manage_gas.get_gas_info`.

## Map Placement Findings

### `/Game/Maps/DemoMap/RegionMap`

- Actor count: 25.
- Important actors: 9 `BP_RegionSeed_C`, 1 `BP_RegionStateManager_C`, 1 `BP_VegetationSpawner_C`, 1 `BP_CityAnchor_C`, 2 `BP_CityRoadSpline_C`, 2 `BP_CityDistrictSpline_C`, 1 `Landscape`, 1 `PCGWorldActor`.
- Seed indices observed: `0..8`.
- City candidate seed observed: `SeedIndex=2`, `BaseType=CITY_CANDIDATE`, `State=2`.
- Placed manager values: `RegionCount=16`, `RegionStateCount=4`, `StateRT=/Game/RegionSystem/RenderTargets/RT_RegionState_9x1`, `RegionIdTexture=/Game/RegionSystem/Textures/RegionID/T_RegionMapLT_RegionID`.
- Review risk: the map has 9 seeds, but the placed manager still reports `RegionCount=16`; the texture naming also points to the large-test ID texture.

### `/Game/Maps/DemoMap/RegionMap_LargeTest`

- Actor count: 48.
- Important actors: 20 `BP_RegionSeed_C`, 1 `BP_RegionStateManager_C`, 1 `BP_VegetationSpawner_C`, 3 `BP_CityAnchor_C`, 7 `BP_CityRoadSpline_C`, 7 `BP_CityGenDistrictSpline_C`, 1 `Landscape`, 1 `PCGWorldActor`.
- Seed indices observed: `0..19`.
- City candidate seeds observed: `SeedIndex=4,5,11,12,13`.
- Locked city candidate seeds observed: `SeedIndex=5,11,13`.
- Placed manager values: `RegionCount=16`, `RegionStateCount=4`, `StateRT=/Game/RegionSystem/RenderTargets/RT_RegionState_20x1`, `RegionIdTexture=/Game/RegionSystem/Textures/RegionID/T_RegionMapLT_RegionID`.
- Review risk: the map has 20 seeds and `MI_RegionLandscape_LargeTest.RegionCount=20`, but the placed manager reports `RegionCount=16`.
- Review risk: labels for some locked seeds include `Nature`, while their `BaseType` is `CITY_CANDIDATE`; treat live property values as authoritative.
- Review risk: `City_03` road/district tags exist, but no `City_03` anchor was detected in the sampled placed anchors; `City_02` appears to have two anchors.

### `/Game/Maps/DemoMap/BiomeBlendingTestMap`

- Actor count: 85.
- Important actors: 66 `PCGPartitionActor`, 3 `BP_SplineBiome_C`, 1 `BP_SplineRoad_C`, 1 `PCGVolume`, 1 `PCGWorldActor`, 1 `Landscape`.
- No RegionSystem seeds, managers, or CityAnchor actors were observed.
- This map is configured as `EditorStartupMap`.

## Material Parameter Findings

Verified material instance: `/Game/RegionSystem/Materials/Instances/MI_RegionLandscape_LargeTest`

- Class: `MaterialInstanceConstant`.
- Scalars: `RegionCount=20`, `RegionMinX=-25200`, `RegionMinY=-25200`, `RegionSizeX=50400`, `RegionSizeY=50400`, `RegionStateCount=4`, `RefractionDepthBias=0`.
- Textures: `IdTexture=/Game/RegionSystem/Textures/RegionID/T_RegionMapLT_RegionID`, `StateTexture=/Game/RegionSystem/RenderTargets/RT_RegionState_20x1`.
- Debug state textures point to `T_RS_DebugGround_State_00` through `T_RS_DebugGround_State_03`.
- `StatePaletteTex=None`.

## DataAsset Findings

- `PDA_RegionVegetation` and `PDA_Biome` Blueprint classes both expose `BiomeInfo:Array<S_Biome>`.
- `/Game/RegionSystem/Data/Vegetation/DA_NatureAliveVegetation`: `BiomeInfo_count=5`.
- `/Game/RegionSystem/Data/Vegetation/DA_NatureDeadVegetation`: `BiomeInfo_count=5`.
- `/Game/RegionSystem/Data/Vegetation/DA_NatureCorruptedVegetation`: `BiomeInfo_count=5`.
- `/Game/PCG/PCG_BiomeBlend/Data/DA_Forest`: `BiomeInfo_count=4`.
- `/Game/PCG/PCG_BiomeBlend/Data/DA_Desert`: `BiomeInfo_count=2`.
- `/Game/PCG/PCG_BiomeBlend/Data/DA_RockyConifer`: `BiomeInfo_count=5`.
- First alive vegetation entries exported meshes such as `/Game/Asset/Nature/StaticMeshes/CommonTree_1` through `CommonTree_5`, with `Weight=1.0` and `Scale=1.5`.

## GAS Spot Check

Verified `/Game/GAS_Pattern/AbilitySystem/Abilities/PlayerAbilities/GA_Primary.GA_Primary` through MCP:

- Blueprint class generated as `GA_Primary_C`.
- Parent class: `GP_Primary`.
- GAS type: `GameplayAbility`.
- `instancingPolicy=1`.
- `netExecutionPolicy=0`.

## Verification Limits

- C++ build was not run.
- Blueprint compile was not run.
- PIE was not run.
- PCG Generate was not run.
- Full PCG graph internals were not deeply inspected; a broad PCG graph analysis request timed out.
- Material graph node internals were not inspected; only material instance parameters were verified.
- `git status --short` failed because Git LFS clean filter hit an access denied error on a `.uasset`.


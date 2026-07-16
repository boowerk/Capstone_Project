# World Corruption System

## Runtime flow

`AGP_GameState` owns one replicated `UGP_WorldCorruptionComponent`. Regional corruption values are the source of truth and the world value is their average.

- `AGP_GameMode` initializes every region at `InitialCorruption`.
- The server adds `PassiveCorruptionIncreasePerMinute` over time.
- Each enemy receives its zone/event `CorruptionRegionId`.
- `UGP_EnemyCorruptionComponent` replaces one infinite GAS effect whenever that region changes.
- At 100% corruption the default effect grants `+0.5 DamageIncreaseRate` and `+50 Armor`.
- A boss death reduces its assigned region by `25` once, including scripted death paths.
- The replicated world average tints the minimap, SkyAtmosphere, ExponentialHeightFog, and the MainMap skybox `Tint`/`Brightness` parameters on each client.

## Editor settings

The native defaults work without placing another actor. In the active `GP_GameMode` Blueprint, use `Run > Corruption` to tune:

- enable/disable, initial value, maximum value;
- passive increase per minute and update interval;
- automatic presentation actor class.

Each `GP_EnemySpawnVolume` has `Zone > Corruption > CorruptionRegionId`.

- `-1`: use the first `RegionsToRevive` entry, then `ZoneOrder` as fallback;
- explicit id: use that region for enemy strength and boss cleansing.

Each enemy or boss Blueprint exposes `EnemyCorruptionComponent` values for maximum damage bonus, armor bonus, and boss cleanse amount.

For a custom skybox material, derive a Blueprint from `GP_CorruptionPresentationActor`, implement `BP_OnCorruptionPresentationChanged`, then assign that class to the GameMode. The native actor already supports the current Sci-Fi skybox parameters.

## PIE verification

1. Start PIE with `GP_GameMode` and `GP_GameState` active.
2. From any temporary Blueprint, call `Get Game State -> Get World Corruption Component`.
3. Call `Set World Corruption(100)` to verify maximum enemy buff and red/dark minimap, sky, and fog.
4. Call `Set Region Corruption(RegionId, 100)`, spawn an enemy assigned to that region, and inspect its GAS `DamageIncreaseRate`/`Armor`.
5. Kill a boss in that region and confirm the value drops by `BossDefeatCorruptionReduction`.
6. Call `Set World Corruption(0)` to verify the original environment and minimap colors return.

Automation coverage:

- `ProjectEden.Game.Corruption.State`
- `ProjectEden.Game.Corruption.EnemyEffectContract`
- `ProjectEden.UI.Minimap.CaptureStability`

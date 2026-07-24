# Dedicated Server Scripts

Use these scripts from `Project_Eden/Scripts/DedicatedServer`.

## Fast C++ Server Loop

Use when only C++ changed.

```bat
BuildDevServer.bat
StartDevServer_LocalExe.bat
```

`StartDevServer_LocalExe.bat` prefers:

```text
Binaries/Win64/Project_EdenServer.exe
```

It falls back to cooked or staged server builds only if the local server exe is missing.

## Cooked Server Loop

Use when assets changed, such as Blueprint, map, GameplayEffect, InputAction, IMC, or packaged server content.

```bat
BuildDevServer.bat
CookDevServer.bat
StartDevServer_Cooked.bat
```

`StartDevServer_Cooked.bat` prefers:

```text
Saved/DedicatedServer/WindowsServer/Project_EdenServer.exe
Saved/StagedBuilds/WindowsServer/Project_EdenServer.exe
```

It falls back to the local server exe only if cooked or staged builds are missing.

## Cooked Client Loop

Use the packaged game client for village streaming and PCG validation.
`UnrealEditor.exe -game` can hit a UE 5.7 editor-only animation metadata
transaction crash while dynamically streamed village assets are loading.

```bat
BuildDevClient.bat
CookDevClient.bat
StartDevClient_Cooked_Localhost.bat
```

`BuildDevClient.bat` also handles the installed PCGExtendedToolkit package
that ships Editor/Server binaries but no UnrealGame manifest. When required,
it prepares an ignored project-local source copy and compiles the Game modules.

For a remote host:

```bat
StartDevClient_Cooked_Radmin.bat <RadminVPN server IP>
```

## Editor Client Scripts

These remain useful for lightweight tests that do not stream the village
animation dependencies. They are not the release-equivalent client path.

```bat
StartEditorClient_Localhost.bat
StartEditorClient_Radmin.bat <RadminVPN server IP>
```

Use localhost for same-machine tests. Use Radmin when testing remote clients.

## Verified Test State

Last verified: 2026-05-19

Verified flow:

```text
Remote client connects through RadminVPN
Client input activates GAS ability
Server spawns BP_NetTestProjectile
Projectile replicates to clients
Server handles hit and applies GE_Damage_NetTestProjectile
GE_Cooldown_NetTestProjectile blocks repeated activation
HitReact event fires once
Hit VFX appears on clients through AGP_Projectile::MulticastPlayHitEffect
Damage number duplicate display is resolved
```

Current known deferred issue:

```text
Animation sync looks slightly off during remote dedicated-server play.
This is intentionally deferred from the projectile/GAS verification path.
```

Next feature direction:

```text
Design DataAsset-based skill management so PlayerController does not keep
growing one AbilityClass property per test skill.
```

## Rule of Thumb

```text
C++ only changed      -> BuildDevServer + StartDevServer_LocalExe
Content/assets changed -> BuildDevServer if needed + CookDevServer + StartDevServer_Cooked
```

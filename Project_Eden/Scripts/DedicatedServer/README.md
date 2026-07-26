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
Binaries/Win64/Project_EdenServer-Cmd.exe
Binaries/Win64/Project_EdenServer.exe
```

The server target builds both files. `Project_EdenServer-Cmd.exe` is the
console-subsystem variant used for live logs; the regular executable remains
available for deployment compatibility. The launcher falls back to cooked,
staged, or regular server builds when the local console executable is missing.
Run the BAT instead of double-clicking `Project_EdenServer.exe`. The BAT keeps
a visible CMD window, streams the complete server log there, and also writes a
dedicated file to:

```text
Saved/Cooked/WindowsServer/Project_Eden/Saved/Logs/Project_EdenServer.log
```

Unreal redirects local dedicated-server file output into its cooked sandbox;
the launcher prints this resolved path before startup. The window remains open
with the exit code if the server stops or crashes.

## Cooked Server Loop

Use when assets changed, such as Blueprint, map, GameplayEffect, InputAction, IMC, or packaged server content.

```bat
BuildDevServer.bat
CookDevServer.bat
StartDevServer_Cooked.bat
```

`StartDevServer_Cooked.bat` prefers:

```text
Saved/DedicatedServer/WindowsServer/Project_EdenServer-Cmd.exe
Saved/StagedBuilds/WindowsServer/Project_EdenServer-Cmd.exe
Saved/DedicatedServer/WindowsServer/Project_EdenServer.exe
Saved/StagedBuilds/WindowsServer/Project_EdenServer.exe
```

Like the local launcher, it shows live server output in its CMD window and
writes `Project_Eden/Saved/Logs/Project_EdenServer.log` inside the selected
DedicatedServer or StagedBuilds package. The launcher prints the exact path.

The launcher rejects a package when its successful-cook stamp is missing, a
project input is newer, or the package index does not contain `BP_RunPortal`.
It never falls back to a loose executable because that can silently combine
new server code with stale cooked assets.

The server cook explicitly includes all four village maps (`L_Village_00` through
`L_Village_03`) because the village catalog loads them dynamically.

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

The cooked localhost server/client scripts opt into `-AllowLobbyForceStart`.
This restores the lobby's solo-start button for Development dedicated-server
testing. The bypass remains unavailable in Shipping and in scripts without the
explicit flag.

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

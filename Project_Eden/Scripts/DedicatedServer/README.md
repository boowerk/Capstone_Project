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

## Client Scripts

```bat
StartEditorClient_Localhost.bat
StartEditorClient_Radmin.bat <RadminVPN server IP>
```

Use localhost for same-machine tests. Use Radmin when testing remote clients.

## Rule of Thumb

```text
C++ only changed      -> BuildDevServer + StartDevServer_LocalExe
Content/assets changed -> BuildDevServer if needed + CookDevServer + StartDevServer_Cooked
```

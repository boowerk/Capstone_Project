param(
    [Parameter(Mandatory = $true)]
    [string]$UProject,

    [ValidateSet("Editor", "Build")]
    [string]$Kind = "Editor"
)

$requiredRelativePath = if ($Kind -eq "Build") {
    "Engine\Build\BatchFiles\Build.bat"
} else {
    "Engine\Binaries\Win64\UnrealEditor.exe"
}

function Test-EngineRoot {
    param([string]$Root)

    if ([string]::IsNullOrWhiteSpace($Root)) {
        return $false
    }

    $expandedRoot = [Environment]::ExpandEnvironmentVariables($Root.Trim('"'))
    return Test-Path -LiteralPath (Join-Path $expandedRoot $requiredRelativePath)
}

function Add-Candidate {
    param(
        [System.Collections.Generic.List[string]]$Candidates,
        [string]$Path
    )

    if (-not [string]::IsNullOrWhiteSpace($Path) -and -not $Candidates.Contains($Path)) {
        [void]$Candidates.Add($Path)
    }
}

$candidates = [System.Collections.Generic.List[string]]::new()

Add-Candidate $candidates $env:UE_SERVER_ROOT

$engineAssociation = $null
if (Test-Path -LiteralPath $UProject) {
    try {
        $projectJson = Get-Content -LiteralPath $UProject -Raw | ConvertFrom-Json
        $engineAssociation = $projectJson.EngineAssociation
    } catch {
        $engineAssociation = $null
    }
}

if (-not [string]::IsNullOrWhiteSpace($engineAssociation)) {
    try {
        $builds = Get-ItemProperty -Path "HKCU:\Software\Epic Games\Unreal Engine\Builds" -ErrorAction Stop
        $buildProperty = $builds.PSObject.Properties[$engineAssociation]
        if ($buildProperty) {
            Add-Candidate $candidates $buildProperty.Value
        }
    } catch {
    }

    try {
        $versionKey = "HKLM:\SOFTWARE\EpicGames\Unreal Engine\$engineAssociation"
        $installedVersion = Get-ItemProperty -Path $versionKey -ErrorAction Stop
        Add-Candidate $candidates $installedVersion.InstalledDirectory
    } catch {
    }

    try {
        $versionKeyWow = "HKLM:\SOFTWARE\WOW6432Node\EpicGames\Unreal Engine\$engineAssociation"
        $installedVersionWow = Get-ItemProperty -Path $versionKeyWow -ErrorAction Stop
        Add-Candidate $candidates $installedVersionWow.InstalledDirectory
    } catch {
    }
}

try {
    Get-ChildItem -Path "HKLM:\SOFTWARE\EpicGames\Unreal Engine" -ErrorAction Stop | ForEach-Object {
        $installedVersion = Get-ItemProperty -Path $_.PSPath -ErrorAction SilentlyContinue
        Add-Candidate $candidates $installedVersion.InstalledDirectory
    }
} catch {
}

$launcherInstalledPath = Join-Path $env:ProgramData "Epic\UnrealEngineLauncher\LauncherInstalled.dat"
if (Test-Path -LiteralPath $launcherInstalledPath) {
    try {
        $launcherData = Get-Content -LiteralPath $launcherInstalledPath -Raw | ConvertFrom-Json
        foreach ($installation in $launcherData.InstallationList) {
            Add-Candidate $candidates $installation.InstallLocation
        }
    } catch {
    }
}

Add-Candidate $candidates "C:\Engine_server\Windows"
Add-Candidate $candidates "C:\Program Files\Epic Games\UE_5.7"
Add-Candidate $candidates "C:\Program Files\Epic Games\UE_5.6"
Add-Candidate $candidates "D:\Engine_server\Windows"
Add-Candidate $candidates "D:\Epic Games\UE_5.7"
Add-Candidate $candidates "D:\Epic Games\UE_5.6"

foreach ($candidate in $candidates) {
    if (Test-EngineRoot $candidate) {
        [Environment]::ExpandEnvironmentVariables($candidate.Trim('"'))
        exit 0
    }
}

exit 1

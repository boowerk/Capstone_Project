param(
    [Parameter(Mandatory = $true)]
    [string]$ProjectRoot
)

$ErrorActionPreference = "Stop"

# cmd.exe can preserve the closing quote as a literal character when a quoted
# argument ends in a backslash (the scripts historically use a trailing slash).
$projectRootArgument = $ProjectRoot.Trim().Trim('"')
$projectRootPath = [System.IO.Path]::GetFullPath($projectRootArgument)
$cookStampName = ".project_eden_server_cook_complete"
$requiredCookedAsset = "BP_RunPortal.uasset"

function Get-LatestProjectInput {
    param([string]$Root)

    $candidateFiles = [System.Collections.Generic.List[System.IO.FileInfo]]::new()
    $singleFiles = @(
        (Join-Path $Root "Project_Eden.uproject"),
        (Join-Path $Root "Binaries\Win64\Project_EdenServer.exe")
    )

    foreach ($singleFile in $singleFiles) {
        if (Test-Path -LiteralPath $singleFile -PathType Leaf) {
            $candidateFiles.Add((Get-Item -LiteralPath $singleFile))
        }
    }

    foreach ($relativeDirectory in @("Config", "Content", "Source", "Scripts\DedicatedServer")) {
        $directory = Join-Path $Root $relativeDirectory
        if (Test-Path -LiteralPath $directory -PathType Container) {
            foreach ($file in Get-ChildItem -LiteralPath $directory -Recurse -File) {
                $candidateFiles.Add($file)
            }
        }
    }

    $pluginsDirectory = Join-Path $Root "Plugins"
    if (Test-Path -LiteralPath $pluginsDirectory -PathType Container) {
        foreach ($file in Get-ChildItem -LiteralPath $pluginsDirectory -Recurse -File) {
            $normalizedPath = $file.FullName.Replace("/", "\")
            if ($file.Extension -eq ".uplugin" -or
                $normalizedPath -match "\\(Config|Content|Source)\\") {
                $candidateFiles.Add($file)
            }
        }
    }

    return $candidateFiles |
        Sort-Object LastWriteTimeUtc -Descending |
        Select-Object -First 1
}

function Find-ServerExecutable {
    param([string]$CookRoot)

    foreach ($relativePath in @(
        "Project_EdenServer-Cmd.exe",
        "Project_Eden\Binaries\Win64\Project_EdenServer-Cmd.exe",
        "Project_EdenServer.exe",
        "Project_Eden\Binaries\Win64\Project_EdenServer.exe"
    )) {
        $candidate = Join-Path $CookRoot $relativePath
        if (Test-Path -LiteralPath $candidate -PathType Leaf) {
            return Get-Item -LiteralPath $candidate
        }
    }

    return $null
}

function Test-PortalAssetInCook {
    param([string]$CookRoot)

    $paksDirectory = Join-Path $CookRoot "Project_Eden\Content\Paks"
    $containerIndex = Get-ChildItem `
        -LiteralPath $paksDirectory `
        -Filter "*WindowsServer.utoc" `
        -File `
        -ErrorAction SilentlyContinue |
        Select-Object -First 1

    if (-not $containerIndex) {
        return $false
    }

    $indexBytes = [System.IO.File]::ReadAllBytes($containerIndex.FullName)
    $indexText = [System.Text.Encoding]::UTF8.GetString($indexBytes)
    return $indexText.Contains($requiredCookedAsset)
}

$latestInput = Get-LatestProjectInput -Root $projectRootPath
$rejectionReasons = [System.Collections.Generic.List[string]]::new()
$cookRoots = @(
    (Join-Path $projectRootPath "Saved\DedicatedServer\WindowsServer"),
    (Join-Path $projectRootPath "Saved\StagedBuilds\WindowsServer")
)

foreach ($cookRoot in $cookRoots) {
    $serverExecutable = Find-ServerExecutable -CookRoot $cookRoot
    if (-not $serverExecutable) {
        $rejectionReasons.Add("Server executable is missing under '$cookRoot'.")
        continue
    }

    $cookStampPath = Join-Path $cookRoot $cookStampName
    if (-not (Test-Path -LiteralPath $cookStampPath -PathType Leaf)) {
        $rejectionReasons.Add(
            "Cook completion stamp is missing under '$cookRoot'."
        )
        continue
    }

    $cookStamp = Get-Item -LiteralPath $cookStampPath
    if ($latestInput -and
        $latestInput.LastWriteTimeUtc -gt $cookStamp.LastWriteTimeUtc) {
        $rejectionReasons.Add(
            "Cook '$cookRoot' is older than '$($latestInput.FullName)'."
        )
        continue
    }

    if (-not (Test-PortalAssetInCook -CookRoot $cookRoot)) {
        $rejectionReasons.Add(
            "Cook '$cookRoot' does not contain '$requiredCookedAsset'."
        )
        continue
    }

    [Console]::Out.WriteLine($serverExecutable.FullName)
    exit 0
}

foreach ($reason in $rejectionReasons) {
    [Console]::Error.WriteLine("Cooked server rejected: $reason")
}

[Console]::Error.WriteLine(
    "Run BuildDevServer.bat and CookDevServer.bat before starting the cooked server."
)
exit 1

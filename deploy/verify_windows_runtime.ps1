param(
    [Parameter(Mandatory = $true)]
    [string]$OutDir,
    [Parameter(Mandatory = $true)]
    [string]$ServiceExeName
)

$ErrorActionPreference = "Stop"

$resolvedOutDir = (Resolve-Path -LiteralPath $OutDir).Path
$serviceExePath = Join-Path $resolvedOutDir $ServiceExeName

if (-not (Test-Path -LiteralPath $serviceExePath)) {
    throw "Service executable not found: $serviceExePath"
}

function Get-CandidateRuntimeDirs {
    param(
        [string]$QtBinDir
    )

    $dirs = New-Object System.Collections.Generic.List[string]

    if ($QtBinDir -and (Test-Path -LiteralPath $QtBinDir)) {
        $dirs.Add($QtBinDir)

        $qtArchDir = Split-Path -Path $QtBinDir -Parent
        $qtVersionDir = Split-Path -Path $qtArchDir -Parent
        $qtRootDir = Split-Path -Path $qtVersionDir -Parent

        if (Test-Path -LiteralPath $qtRootDir) {
            foreach ($toolDir in Get-ChildItem -Path (Join-Path $qtRootDir "Tools") -Directory -ErrorAction SilentlyContinue) {
                if ($toolDir.Name -match "^mingw") {
                    $toolBin = Join-Path $toolDir.FullName "bin"
                    if (Test-Path -LiteralPath $toolBin) {
                        $dirs.Add($toolBin)
                    }
                }
            }
        }
    }

    $gpp = Get-Command g++.exe -ErrorAction SilentlyContinue
    if ($gpp -and $gpp.Source) {
        $gppDir = Split-Path -Path $gpp.Source -Parent
        if (Test-Path -LiteralPath $gppDir) {
            $dirs.Add($gppDir)
        }
    }

    foreach ($pathEntry in ($env:PATH -split ';')) {
        $entry = $pathEntry.Trim().Trim('"')
        if ($entry -ne "" -and (Test-Path -LiteralPath $entry)) {
            $dirs.Add($entry)
        }
    }

    return $dirs | Select-Object -Unique
}

function Copy-MissingRuntimeFiles {
    param(
        [string]$DestinationDir,
        [string[]]$RuntimeFiles,
        [string[]]$SearchDirs
    )

    foreach ($file in $RuntimeFiles) {
        $dest = Join-Path $DestinationDir $file
        if (Test-Path -LiteralPath $dest) {
            continue
        }

        foreach ($dir in $SearchDirs) {
            $candidate = Join-Path $dir $file
            if (Test-Path -LiteralPath $candidate) {
                Copy-Item -LiteralPath $candidate -Destination $dest -Force
                break
            }
        }
    }
}

$qtBinDir = ""
if ($env:QT_BIN_DIR) {
    $qtBinDir = $env:QT_BIN_DIR.Trim('"')
}

$commonRuntimeFiles = @(
    "Qt6Core.dll",
    "Qt6Core5Compat.dll",
    "Qt6Gui.dll",
    "Qt6Network.dll",
    "Qt6RemoteObjects.dll"
)

$mingwRuntimeFiles = @(
    "libgcc_s_seh-1.dll",
    "libstdc++-6.dll",
    "libwinpthread-1.dll"
)

$isMinGwRuntimeExpected = $false
if ($qtBinDir -ne "" -and ($qtBinDir -match "mingw" -or (Test-Path -LiteralPath (Join-Path $qtBinDir "libstdc++-6.dll")))) {
    $isMinGwRuntimeExpected = $true
}

if (-not $isMinGwRuntimeExpected) {
    $dumpbin = Get-Command dumpbin.exe -ErrorAction SilentlyContinue
    if ($dumpbin) {
        $depsText = (& $dumpbin.Source /dependents $serviceExePath 2>$null | Out-String)
        if ($depsText -match "libgcc_s_seh-1\.dll|libstdc\+\+-6\.dll|libwinpthread-1\.dll") {
            $isMinGwRuntimeExpected = $true
        }
    }
}

$candidateDirs = Get-CandidateRuntimeDirs -QtBinDir $qtBinDir
Copy-MissingRuntimeFiles -DestinationDir $resolvedOutDir -RuntimeFiles $commonRuntimeFiles -SearchDirs $candidateDirs

if ($isMinGwRuntimeExpected) {
    Copy-MissingRuntimeFiles -DestinationDir $resolvedOutDir -RuntimeFiles $mingwRuntimeFiles -SearchDirs $candidateDirs
}

$requiredRuntimeFiles = @($commonRuntimeFiles)
if ($isMinGwRuntimeExpected) {
    $requiredRuntimeFiles += $mingwRuntimeFiles
}

$missing = @()
foreach ($file in $requiredRuntimeFiles) {
    if (-not (Test-Path -LiteralPath (Join-Path $resolvedOutDir $file))) {
        $missing += $file
    }
}

if ($missing.Count -gt 0) {
    throw ("Missing runtime files in output directory: " + ($missing -join ", "))
}

if ($qtBinDir -ne "") {
    $expectedQtCore = Join-Path $qtBinDir "Qt6Core.dll"
    $actualQtCore = Join-Path $resolvedOutDir "Qt6Core.dll"

    if (Test-Path -LiteralPath $expectedQtCore) {
        $expectedVersion = (Get-Item -LiteralPath $expectedQtCore).VersionInfo.FileVersion
        $actualVersion = (Get-Item -LiteralPath $actualQtCore).VersionInfo.FileVersion
        if ($expectedVersion -ne $actualVersion) {
            throw "Qt6Core.dll version mismatch. Expected: $expectedVersion, packaged: $actualVersion"
        }
    }
}

$oldPath = $env:PATH
try {
    $env:PATH = "$resolvedOutDir;C:\Windows\System32;C:\Windows"
    $process = Start-Process -FilePath $serviceExePath -ArgumentList "-e" -PassThru

    Start-Sleep -Seconds 4

    if ($process.HasExited) {
        $exitCode = $process.ExitCode
        if ($exitCode -ne 0) {
            $hexCode = "{0:X8}" -f ([uint32]($exitCode -band 0xffffffff))
            throw "Service smoke test failed early (exit=$exitCode, hex=0x$hexCode). Runtime package is incompatible."
        }
    } else {
        Stop-Process -Id $process.Id -Force
    }
}
finally {
    $env:PATH = $oldPath
}

Write-Host "Service runtime verification passed for $serviceExePath"

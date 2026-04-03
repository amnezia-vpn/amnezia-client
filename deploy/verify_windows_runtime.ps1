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

$requiredRuntimeFiles = @(
    "Qt6Core.dll",
    "Qt6Core5Compat.dll",
    "Qt6Gui.dll",
    "Qt6Network.dll",
    "Qt6RemoteObjects.dll",
    "libgcc_s_seh-1.dll",
    "libstdc++-6.dll",
    "libwinpthread-1.dll"
)

$missing = @()
foreach ($file in $requiredRuntimeFiles) {
    if (-not (Test-Path -LiteralPath (Join-Path $resolvedOutDir $file))) {
        $missing += $file
    }
}

if ($missing.Count -gt 0) {
    throw ("Missing runtime files in output directory: " + ($missing -join ", "))
}

if ($env:QT_BIN_DIR) {
    $qtBinDir = $env:QT_BIN_DIR.Trim('"')
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

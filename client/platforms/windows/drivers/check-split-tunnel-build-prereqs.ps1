#Requires -Version 7.2
<#
  Checks prerequisites for building mullvad-split-tunnel (KMDF).
  Run: pwsh -NoProfile -File .\check-split-tunnel-build-prereqs.ps1
#>
$ErrorActionPreference = "Continue"
$need = "WindowsKernelModeDriver10.0"

function Test-Dir($p) { Test-Path -LiteralPath $p -PathType Container }
function Test-File($p) { Test-Path -LiteralPath $p -PathType Leaf }

Write-Host "=== MSBuild (same search as build-split-tunnel.ps1) ===" -ForegroundColor Cyan
$msbuild = $null
foreach ($p in @(
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe"
    )) {
    if (Test-File $p) { $msbuild = $p; break }
}
if (-not $msbuild) {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-File $vswhere) {
        $install = & $vswhere -latest -products * -prerelease -property installationPath 2>$null | Select-Object -First 1
        if ($install) {
            $cand = Join-Path $install "MSBuild\Current\Bin\MSBuild.exe"
            if (Test-File $cand) { $msbuild = $cand }
        }
    }
}
if ($msbuild) {
    Write-Host "  OK MSBuild: $msbuild" -ForegroundColor Green
    # ...\BuildTools\MSBuild\Current\Bin\MSBuild.exe -> go up 4 levels to BuildTools root
    $instRoot = $msbuild
    for ($i = 0; $i -lt 4; $i++) { $instRoot = Split-Path $instRoot -Parent }
    $plat = Join-Path $instRoot "MSBuild\Microsoft\VC\v170\Platforms\x64\PlatformToolsets"
    if (Test-Dir $plat) {
        $names = Get-ChildItem -LiteralPath $plat -Directory -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Name
        Write-Host "  PlatformToolsets (x64): $($names -join ', ')"
        if ($names -contains $need) {
            Write-Host "  OK $need present" -ForegroundColor Green
        }
        else {
            Write-Host "  FAIL: missing folder '$need' under PlatformToolsets (install WDK after VS/Build Tools)" -ForegroundColor Red
        }
    }
    else {
        Write-Host "  WARN: PlatformToolsets path not found: $plat" -ForegroundColor Yellow
    }
}
else {
    Write-Host "  FAIL: MSBuild.exe not found" -ForegroundColor Red
}

Write-Host "`n=== winget (optional; MSI WDK does not register here) ===" -ForegroundColor Cyan
Write-Host "  Note: WDK installed from .msi often does not appear in winget list — that is normal." -ForegroundColor DarkGray
foreach ($id in @(
        "Microsoft.WindowsSDK.10.0.28000",
        "Microsoft.WindowsWDK.10.0.28000",
        "Microsoft.VisualStudio.2022.BuildTools"
    )) {
    $out = winget list --id $id 2>&1 | Out-String
    if ($out -match "No installed package") {
        Write-Host "  winget: not listed — $id" -ForegroundColor DarkGray
    }
    else {
        Write-Host "  winget: $id" -ForegroundColor Green
        $out.Trim() -split "`n" | Where-Object { $_ -match "\S" } | Select-Object -Last 3 | ForEach-Object { Write-Host "    $_" }
    }
}

Write-Host "`n=== All VS installs: WindowsKernelModeDriver10.0 (WDK integration) ===" -ForegroundColor Cyan
$vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-File $vswhere) {
    $paths = @(& $vswhere -all -products * -prerelease -property installationPath 2>$null)
    foreach ($install in $paths) {
        if (-not $install) { continue }
        $msb = Join-Path $install "MSBuild\Current\Bin\MSBuild.exe"
        $plat = Join-Path $install "MSBuild\Microsoft\VC\v170\Platforms\x64\PlatformToolsets\WindowsKernelModeDriver10.0"
        $hasMsb = Test-File $msb
        $hasKmd = Test-Dir $plat
        $tag = if ($hasKmd) { "OK driver toolset" } else { "no driver toolset" }
        $color = if ($hasKmd) { "Green" } else { "Yellow" }
        Write-Host "  $install" -ForegroundColor $color
        Write-Host "    MSBuild: $(if ($hasMsb) { 'yes' } else { 'no' }) | $need : $(if ($hasKmd) { 'yes' } else { 'no' }) ($tag)"
    }
    if ($paths.Count -eq 0) { Write-Host "  (no VS instances reported by vswhere)" -ForegroundColor Yellow }
}
else {
    Write-Host "  vswhere.exe not found" -ForegroundColor Red
}

Write-Host "`n=== Windows Kits 10 ===" -ForegroundColor Cyan
$kits = Join-Path ${env:ProgramFiles(x86)} "Windows Kits\10"
if (Test-Dir $kits) {
    $inc = Join-Path $kits "Include"
    if (Test-Dir $inc) {
        $vers = Get-ChildItem -LiteralPath $inc -Directory -ErrorAction SilentlyContinue |
            Where-Object { $_.Name -match '^\d+\.\d+' } |
            ForEach-Object { try { [version]$_.Name } catch { $null } } |
            Where-Object { $_ } |
            Sort-Object -Descending |
            Select-Object -First 5
        Write-Host "  Include versions (newest first): $($vers -join ', ')"
    }
    else {
        Write-Host "  WARN: $inc missing" -ForegroundColor Yellow
    }
}
else {
    Write-Host "  FAIL: $kits missing" -ForegroundColor Red
}

Write-Host "`nIf $need is missing on every SKU above, build-split-tunnel.ps1 will stop with an error (no pointless MSB8020)." -ForegroundColor Yellow
Write-Host "Repair WDK and enable integration with VS 2022 Build Tools, or install WDK via winget:" -ForegroundColor Yellow
Write-Host "  winget install -e --id Microsoft.WindowsWDK.10.0.28000 --accept-package-agreements --accept-source-agreements" -ForegroundColor Gray

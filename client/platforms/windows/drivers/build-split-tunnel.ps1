#Requires -Version 7.2
<#
  Build Mullvad-derived split-tunnel KMDF driver and copy mullvad-split-tunnel.sys here
  (same folder as this script) for Amnezia client POST_BUILD/install.

  Prerequisites: Visual Studio 2022 (or Build Tools) + MSVC, then Windows SDK + WDK (KMDF toolset
  WindowsKernelModeDriver10.0). If MSBuild reports MSB8020, install WDK after VCTools — see
  cursor_settings/scripts/install-vs-buildtools-elevated.cmd (step 2). WDK from MSI is fine; if
  toolset appears only under VS Community/Professional, this script prefers that MSBuild over
  Build Tools when Build Tools lack the driver toolset.

  Optional signing (set env or pass parameters):
    $env:AMNEZIA_SPLIT_TUNNEL_PFX = path to .pfx
    $env:AMNEZIA_SPLIT_TUNNEL_PFX_PASSWORD = password
  Or: -PfxPath -PfxPassword
  Override MSBuild (must already have WindowsKernelModeDriver10.0 toolset):
    -MsBuildPath "C:\...\MSBuild.exe"
#>
[Diagnostics.CodeAnalysis.SuppressMessageAttribute(
    'PSAvoidUsingPlainTextForPassword',
    'PfxPassword',
    Justification = 'signtool.exe requires a plaintext password argument; this is a local dev signing script.'
)]
param(
    [string]$Configuration = "Release",
    [string]$Platform = "x64",
    [string]$WinSplitTunnelRoot = "",
    [string]$PfxPath = "",
    [string]$PfxPassword = "",
    [string]$MsBuildPath = "",
    [string]$WdkVersion = "",
    [switch]$DisableSpectreMitigation
)

$ErrorActionPreference = "Stop"
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
# .../amnezia-client/client/platforms/windows/drivers -> repo root is 5 levels up (cursor/)
$repoRoot = (Resolve-Path (Join-Path $here "..\..\..\..\..")).Path

if (-not $WinSplitTunnelRoot) {
    $candidate = Join-Path $repoRoot "win-split-tunnel"
    if (Test-Path (Join-Path $candidate "src\mullvad-split-tunnel.sln")) {
        $WinSplitTunnelRoot = $candidate
    }
}
if (-not (Test-Path $WinSplitTunnelRoot)) {
    throw "win-split-tunnel not found. Clone next to amnezia-client or pass -WinSplitTunnelRoot."
}

if (-not $WdkVersion) {
    # VS 2022 is Dev17. Some newer SDK/WDK layouts (for example 10.0.28000) ship Dev18 task DLLs
    # only, while WindowsKernelModeDriver10.0 for VS2022 expects Microsoft.DriverKit.Build.Tasks.17.0.dll.
    $wdkBuildRoot = Join-Path ${env:ProgramFiles(x86)} "Windows Kits\10\build"
    $wdkVersion = Get-ChildItem -LiteralPath $wdkBuildRoot -Directory -ErrorAction SilentlyContinue |
        Where-Object { Test-Path -LiteralPath (Join-Path $_.FullName "bin\Microsoft.DriverKit.Build.Tasks.17.0.dll") } |
        Sort-Object { [version]$_.Name } -Descending |
        Select-Object -First 1 -ExpandProperty Name
}
if (-not $WdkVersion) {
    throw "No VS2022-compatible WDK build found. Expected Microsoft.DriverKit.Build.Tasks.17.0.dll under Windows Kits\10\build\<version>\bin."
}

function Get-VsInstallRootFromMsBuild([string]$msbuildExe) {
    $r = $msbuildExe
    for ($i = 0; $i -lt 4; $i++) { $r = Split-Path $r -Parent }
    return $r
}

function Test-WindowsKernelModeDriverToolset([string]$msbuildExe) {
    if (-not (Test-Path -LiteralPath $msbuildExe)) { return $false }
    $root = Get-VsInstallRootFromMsBuild $msbuildExe
    $kmd = Join-Path $root "MSBuild\Microsoft\VC\v170\Platforms\x64\PlatformToolsets\WindowsKernelModeDriver10.0"
    return Test-Path -LiteralPath $kmd -PathType Container
}

$seenMsbuild = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
$msbuildCandidates = [System.Collections.Generic.List[string]]::new()
function Add-MsBuildCandidate([string]$p) {
    if (-not (Test-Path -LiteralPath $p)) { return }
    if ($seenMsbuild.Add($p)) { [void]$msbuildCandidates.Add($p) }
}

if ($MsBuildPath) {
    if (-not (Test-Path -LiteralPath $MsBuildPath)) { throw "MsBuildPath not found: $MsBuildPath" }
    if (-not (Test-WindowsKernelModeDriverToolset $MsBuildPath)) {
        throw "MsBuildPath does not have WindowsKernelModeDriver10.0 toolset (WDK not integrated with that VS instance): $MsBuildPath"
    }
    $msbuild = $MsBuildPath
    Write-Host "Using MSBuild (from -MsBuildPath): $msbuild"
}
else {
    # Prefer MSBuild from an installation where WDK registered WindowsKernelModeDriver10.0 (MSI often
    # integrates with full VS but not Build Tools if Build Tools were installed later).
    foreach ($p in @(
            "${env:ProgramFiles}\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe",
            "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe",
            "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe",
            "${env:ProgramFiles}\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe",
            "${env:ProgramFiles}\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe",
            "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe"
        )) {
        Add-MsBuildCandidate $p
    }
    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path -LiteralPath $vswhere) {
        $paths = @(& $vswhere -all -products * -prerelease -property installationPath 2>$null)
        foreach ($install in $paths) {
            if (-not $install) { continue }
            Add-MsBuildCandidate (Join-Path $install "MSBuild\Current\Bin\MSBuild.exe")
        }
    }
    foreach ($root in @("${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022", "${env:ProgramFiles}\Microsoft Visual Studio\2022")) {
        if (-not (Test-Path -LiteralPath $root)) { continue }
        Get-ChildItem -LiteralPath $root -Directory -ErrorAction SilentlyContinue | ForEach-Object {
            Add-MsBuildCandidate (Join-Path $_.FullName "MSBuild\Current\Bin\MSBuild.exe")
        }
    }

    $msbuild = $null
    foreach ($p in $msbuildCandidates) {
        if (Test-WindowsKernelModeDriverToolset $p) { $msbuild = $p; break }
    }
    if (-not $msbuild) {
        $listed = if ($msbuildCandidates.Count -gt 0) { ($msbuildCandidates | ForEach-Object { "  - $_" }) -join "`n" } else { "  (none)" }
        throw @"
No MSBuild with KMDF toolset WindowsKernelModeDriver10.0 was found.

Checked MSBuild candidates:
$listed

WDK files under Windows Kits do not replace this: the WDK installer must extend a VS 2022 instance so this folder exists:
  ...\Microsoft Visual Studio\2022\<SKU>\MSBuild\Microsoft\VC\v170\Platforms\x64\PlatformToolsets\WindowsKernelModeDriver10.0

Fix (pick one):
  1) Run the same WDK MSI again -> Change/Repair and enable integration with Visual Studio 2022 Build Tools (install Build Tools first if needed).
  2) winget install -e --id Microsoft.WindowsWDK.10.0.28000   (after matching Windows SDK 10.0.28000)
  3) Install Visual Studio 2022 Community + Desktop C++ workload, then repair WDK and pick Community.

Then re-run this script, or pass -MsBuildPath to an MSBuild.exe whose tree contains the folder above.
"@
    }
    Write-Host "Using MSBuild: $msbuild"
}

$sln = Join-Path $WinSplitTunnelRoot "src\mullvad-split-tunnel.sln"
Write-Host "Using WDK/SDK target version: $WdkVersion"
$msbuildArgs = @(
    $sln,
    "/m",
    "/p:Configuration=$Configuration",
    "/p:Platform=$Platform",
    "/p:WindowsTargetPlatformVersion=$WdkVersion",
    "/t:Rebuild"
)
if ($DisableSpectreMitigation) {
    Write-Warning "Building with SpectreMitigation=false because Spectre-mitigated MSVC libraries are unavailable."
    $msbuildArgs += "/p:SpectreMitigation=false"
}
& $msbuild @msbuildArgs
if ($LASTEXITCODE -ne 0) { throw "MSBuild failed with exit code $LASTEXITCODE" }

$built = Join-Path $WinSplitTunnelRoot "bin\$Platform-$Configuration\mullvad-split-tunnel.sys"
if (-not (Test-Path $built)) {
    throw "Expected output not found: $built (check win-split-tunnel build / OutDir)"
}

$out = Join-Path $here "mullvad-split-tunnel.sys"
Copy-Item -Force $built $out
Write-Host "Copied driver to $out"

$pfx = $PfxPath
if (-not $pfx) { $pfx = $env:AMNEZIA_SPLIT_TUNNEL_PFX }
$pw = $PfxPassword
if (-not $pw) { $pw = $env:AMNEZIA_SPLIT_TUNNEL_PFX_PASSWORD }

if ($pfx -and (Test-Path $pfx)) {
    $signtool = $null
    foreach ($w in @(
            "${env:ProgramFiles(x86)}\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe",
            "${env:ProgramFiles(x86)}\Windows Kits\10\bin\10.0.22621.0\x64\signtool.exe",
            "${env:ProgramFiles(x86)}\Windows Kits\10\bin\x64\signtool.exe"
        )) {
        if (Test-Path $w) { $signtool = $w; break }
    }
    if (-not $signtool) {
        $kitsBin = Join-Path ${env:ProgramFiles(x86)} "Windows Kits\10\bin"
        if (Test-Path $kitsBin) {
            $signtool = Get-ChildItem -Path $kitsBin -Filter signtool.exe -Recurse -ErrorAction SilentlyContinue |
                Where-Object { $_.DirectoryName -match '\\x64$' } |
                Sort-Object { [version]($_.Directory.Parent.Name) } -Descending |
                Select-Object -First 1 -ExpandProperty FullName
        }
    }
    if (-not $signtool) {
        Write-Warning "signtool.exe not found; skip signing."
    }
    else {
        $signArgs = @("sign", "/fd", "SHA256", "/f", $pfx, "/tr", "http://timestamp.digicert.com", "/td", "SHA256")
        if ($pw) { $signArgs += @("/p", $pw) }
        $signArgs += $out
        & $signtool @signArgs
        if ($LASTEXITCODE -ne 0) { throw "signtool failed with exit code $LASTEXITCODE" }
        Write-Host "Signed $out"
    }
}
else {
    Write-Host "No PFX provided — driver copied unsigned. For production, sign with your EV/OV cert."
}

#Requires -Version 7.2
$ErrorActionPreference = "Stop"
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = (Resolve-Path (Join-Path $here "..\..\..\..\..")).Path
$env:AMNEZIA_SPLIT_TUNNEL_PFX = Join-Path $repoRoot "cursor_settings\certs\amnezia-dev-codesign.pfx"
$pwdFile = Join-Path $repoRoot "cursor_settings\certs\amnezia-dev-codesign-pfx-password.txt"
$env:AMNEZIA_SPLIT_TUNNEL_PFX_PASSWORD = (Get-Content $pwdFile -Raw).Trim()
Set-Location $here
& (Join-Path $here "build-split-tunnel.ps1") -DisableSpectreMitigation

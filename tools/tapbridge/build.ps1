param([string]$Go = 'go')
$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    & $Go build -trimpath -o '../../service/server/sharing/tapbridge.exe' .
    if ($LASTEXITCODE -ne 0) { throw 'tapbridge build failed' }
} finally { Pop-Location }

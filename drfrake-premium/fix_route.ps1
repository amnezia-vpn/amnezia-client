$ErrorActionPreference = "SilentlyContinue"
$tunIf = Get-NetAdapter -Name "DrFrakeVPN"
if (!$tunIf) { Write-Host "TUN not found"; exit 1 }
$tunIdx = $tunIf.InterfaceIndex

Set-NetIPInterface -InterfaceIndex $tunIdx -InterfaceMetric 1
New-NetRoute -DestinationPrefix "0.0.0.0/0" -InterfaceIndex $tunIdx -NextHop "10.8.1.7" -RouteMetric 1

Set-NetConnectionProfile -InterfaceAlias "DrFrakeVPN" -NetworkCategory Private

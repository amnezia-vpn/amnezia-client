# network-diagnostics-windows.ps1
# Windows network state snapshot for establishing a baseline.
# Run in PowerShell AS ADMINISTRATOR.
#
# Usage (three runs):
#   .\network-diagnostics-windows.ps1 -Label clean
#   .\network-diagnostics-windows.ps1 -Label awg      -Endpoint <server IP>
#   .\network-diagnostics-windows.ps1 -Label xray     -Endpoint <server IP>
#
# Then compare the folders, e.g.:
#   Compare-Object (gc .\snapshot-clean\routes.txt) (gc .\snapshot-awg\routes.txt)

param(
    [string]$Label = "snapshot",
    [string]$Endpoint = ""
)

$out = Join-Path (Get-Location) "snapshot-$Label"
New-Item -ItemType Directory -Path $out -Force | Out-Null

function Save($name, $block) {
    $path = Join-Path $out "$name.txt"
    "=== $name === $(Get-Date -Format o)" | Out-File $path -Encoding utf8
    try   { & $block *>&1 | Out-File $path -Append -Encoding utf8 }
    catch { "ERROR: $($_.Exception.Message)" | Out-File $path -Append -Encoding utf8 }
    Write-Host "  [ok] $name"
}

Write-Host "Snapshot '$Label' -> $out"

# --- 1. System ---------------------------------------------------------
Save "system" {
    Get-ComputerInfo -Property OsName, OsVersion, OsBuildNumber, CsName |
        Format-List
    "IsAdmin: " + ([Security.Principal.WindowsPrincipal]`
        [Security.Principal.WindowsIdentity]::GetCurrent()`
        ).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

# --- 2. Adapters, including hidden and disabled -----------------------------
# Key for detecting an "overwritten driver" and leftover interfaces.
Save "adapters" {
    Get-NetAdapter -IncludeHidden |
        Select-Object Name, InterfaceDescription, ifIndex, Status,
                      AdminStatus, MacAddress, LinkSpeed,
                      DriverProvider, DriverVersion, DriverDate,
                      DriverFileName, DriverInformation, Virtual, Hidden |
        Format-List
}

# --- 3. Connection type (cellular / wifi / ethernet) ------------------------
Save "link-type" {
    "--- NdisPhysicalMedium (802.3=ethernet, Native802.11=wifi, WirelessWan=cellular) ---"
    Get-NetAdapter -IncludeHidden |
        Select-Object Name, InterfaceType, NdisPhysicalMedium, MediaConnectionState |
        Format-Table -AutoSize
    "--- Connection profiles ---"
    Get-NetConnectionProfile |
        Select-Object Name, InterfaceAlias, NetworkCategory,
                      IPv4Connectivity, IPv6Connectivity |
        Format-List
}

# --- 4. Drivers: network filters and tunnels --------------------------------
# Catches WinDivert-style filters, TAP/Wintun, and split-tunnel drivers.
Save "drivers-system" {
    Get-CimInstance Win32_SystemDriver |
        Where-Object { $_.PathName -match 'drivers' } |
        Select-Object Name, DisplayName, State, StartMode, PathName |
        Sort-Object Name | Format-Table -AutoSize
}

Save "drivers-store" {
    pnputil /enum-drivers
}

Save "drivers-net-class" {
    Get-CimInstance Win32_PnPSignedDriver |
        Where-Object { $_.DeviceClass -eq 'NET' } |
        Select-Object DeviceName, DriverProviderName, DriverVersion,
                      DriverDate, InfName, IsSigned, Signer |
        Format-List
}

# --- 5. Routes ---------------------------------------------------------
Save "routes" {
    "--- route print ---"
    route print
    "`n--- Get-NetRoute (IPv4+IPv6) ---"
    Get-NetRoute |
        Select-Object DestinationPrefix, NextHop, ifIndex, InterfaceAlias,
                      RouteMetric, InterfaceMetric, Protocol, AddressFamily |
        Sort-Object AddressFamily, DestinationPrefix | Format-Table -AutoSize
    "`n--- default routes only ---"
    Get-NetRoute -DestinationPrefix '0.0.0.0/0','::/0' -ErrorAction SilentlyContinue |
        Format-Table -AutoSize
    "`n--- interface metrics ---"
    Get-NetIPInterface |
        Select-Object ifIndex, InterfaceAlias, AddressFamily,
                      InterfaceMetric, Dhcp, ConnectionState |
        Sort-Object ifIndex | Format-Table -AutoSize
}

# --- 6. Actual routing stack resolution --------------------------------------
# What you can't see in route print: where the packet will ACTUALLY go.
Save "route-resolution" {
    $targets = @('8.8.8.8', '1.1.1.1')
    if ($Endpoint) { $targets = @($Endpoint) + $targets }
    foreach ($t in $targets) {
        "--- Find-NetRoute -RemoteIPAddress $t ---"
        Find-NetRoute -RemoteIPAddress $t -ErrorAction SilentlyContinue |
            Select-Object IPAddress, InterfaceAlias, ifIndex,
                          DestinationPrefix, NextHop, RouteMetric |
            Format-List
        ""
    }
}

# --- 7. DNS -----------------------------------------------------------------
Save "dns" {
    "--- configured servers ---"
    Get-DnsClientServerAddress |
        Select-Object InterfaceAlias, ifIndex, AddressFamily, ServerAddresses |
        Format-Table -AutoSize
    "`n--- global settings ---"
    Get-DnsClientGlobalSetting | Format-List
    "`n--- NRPT rules (VPN clients write here) ---"
    Get-DnsClientNrptPolicy -ErrorAction SilentlyContinue | Format-List
    Get-DnsClientNrptRule   -ErrorAction SilentlyContinue | Format-List
    "`n--- DoH configuration ---"
    Get-DnsClientDohServerAddress -ErrorAction SilentlyContinue | Format-Table -AutoSize
}

# --- 8. IPv6 and localhost ---------------------------------------------
Save "ipv6-localhost" {
    "--- IP addresses ---"
    Get-NetIPAddress |
        Select-Object InterfaceAlias, IPAddress, PrefixLength,
                      AddressFamily, AddressState, SkipAsSource |
        Sort-Object AddressFamily | Format-Table -AutoSize
    "`n--- IPv6 global config ---"
    netsh interface ipv6 show global
    "`n--- IPv6 disabled via registry (DisabledComponents) ---"
    Get-ItemProperty 'HKLM:\SYSTEM\CurrentControlSet\Services\Tcpip6\Parameters' `
        -Name DisabledComponents -ErrorAction SilentlyContinue |
        Select-Object DisabledComponents | Format-List
    "`n--- localhost resolution ---"
    Resolve-DnsName localhost -ErrorAction SilentlyContinue | Format-Table -AutoSize
    "`n--- hosts file ---"
    Get-Content "$env:SystemRoot\System32\drivers\etc\hosts" -ErrorAction SilentlyContinue
}

# --- 9. Proxy: system and browser (WinINET) ----------------------------
# Key block for Xray: it often works through the system proxy.
Save "proxy" {
    "--- env vars ---"
    Get-ChildItem Env: |
        Where-Object { $_.Name -match 'PROXY|proxy' } |
        Format-Table -AutoSize
    "`n--- WinINET (HKCU, what the browser sees) ---"
    Get-ItemProperty 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Internet Settings' `
        -ErrorAction SilentlyContinue |
        Select-Object ProxyEnable, ProxyServer, ProxyOverride, AutoConfigURL |
        Format-List
    "`n--- WinHTTP ---"
    netsh winhttp show proxy
}

# --- 10. VPN/antiDPI services and processes ----------------------------
Save "services-processes" {
    "--- services matching known patterns ---"
    Get-Service | Where-Object {
        $_.Name -match 'amnezia|wireguard|wg|openvpn|tap|mullvad|tailscale|zapret|nfqws|winws|xray|v2ray|clash|outline|hiddify|proton|nord|express'
    } | Select-Object Name, DisplayName, Status, StartType | Format-Table -AutoSize

    "`n--- processes ---"
    Get-Process | Where-Object {
        $_.ProcessName -match 'amnezia|wireguard|openvpn|mullvad|tailscale|nfqws|winws|goodbyedpi|byedpi|xray|v2ray|clash|tun2socks|sing-box'
    } | Select-Object ProcessName, Id, Path | Format-Table -AutoSize

    "`n--- listening sockets on loopback (Xray inbounds) ---"
    Get-NetTCPConnection -State Listen -ErrorAction SilentlyContinue |
        Where-Object { $_.LocalAddress -in '127.0.0.1','::1','0.0.0.0','::' } |
        Select-Object LocalAddress, LocalPort, OwningProcess,
                      @{n='Process';e={(Get-Process -Id $_.OwningProcess -EA SilentlyContinue).ProcessName}} |
        Sort-Object LocalPort | Format-Table -AutoSize

    "`n--- UDP endpoints (AmneziaWG) ---"
    Get-NetUDPEndpoint -ErrorAction SilentlyContinue |
        Select-Object LocalAddress, LocalPort, OwningProcess,
                      @{n='Process';e={(Get-Process -Id $_.OwningProcess -EA SilentlyContinue).ProcessName}} |
        Sort-Object LocalPort | Format-Table -AutoSize
}

# --- 11. Firewall / WFP -----------------------------------------------------
Save "firewall" {
    "--- profiles ---"
    Get-NetFirewallProfile | Select-Object Name, Enabled, DefaultInboundAction,
                                           DefaultOutboundAction | Format-Table -AutoSize
    "`n--- enabled block rules (short list) ---"
    Get-NetFirewallRule -Enabled True -ErrorAction SilentlyContinue |
        Where-Object { $_.Action -eq 'Block' } |
        Select-Object DisplayName, Direction, Profile, Owner |
        Format-Table -AutoSize
    "`n--- third-party security products ---"
    Get-CimInstance -Namespace root\SecurityCenter2 -ClassName AntiVirusProduct `
        -ErrorAction SilentlyContinue |
        Select-Object displayName, productState | Format-List
}

# --- 12. WFP filters (heavy output, separate file) --------------------------
Save "wfp-filters" {
    netsh wfp show filters file=- 2>$null | Select-Object -First 400
    "(output truncated; for a full dump: netsh wfp show filters file=wfp.xml)"
}

Write-Host "`nDone. Folder: $out"
Write-Host "Diff of two runs, e.g.:"
Write-Host "  Compare-Object (gc .\snapshot-clean\routes.txt) (gc .\snapshot-awg\routes.txt)"

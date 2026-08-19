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

$isAdmin = ([Security.Principal.WindowsPrincipal]`
    [Security.Principal.WindowsIdentity]::GetCurrent()`
    ).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isAdmin) {
    Write-Warning "Running WITHOUT administrator rights. WFP filters and some driver data will not be collected."
}

function Save($name, $block) {
    $path = Join-Path $out "$name.txt"
    "=== $name === $(Get-Date -Format o)  (admin=$isAdmin)" | Out-File $path -Encoding utf8
    try   { & $block 2>&1 | Out-String -Width 500 | Out-File $path -Append -Encoding utf8 }
    catch { "ERROR: $($_.Exception.Message)" | Out-File $path -Append -Encoding utf8 }
    Write-Host "  [ok] $name"
}

function Has($cmd) { [bool](Get-Command $cmd -ErrorAction SilentlyContinue) }

Write-Host "Snapshot '$Label' -> $out"

# --- 1. System ---------------------------------------------------------
Save "system" {
    Get-ComputerInfo -Property OsName, OsVersion, OsBuildNumber, CsName |
        Format-List
    "IsAdmin           : $isAdmin"
    "PSVersion         : $($PSVersionTable.PSVersion)"
}

# --- 2. Adapters, including hidden and disabled -----------------------------
# Key for detecting an "overwritten driver" and leftover interfaces.
Save "adapters" {
    Get-NetAdapter -IncludeHidden |
        Select-Object Name, InterfaceDescription, ifIndex, Status, AdminStatus,
                      MacAddress, DriverProvider, DriverVersion, DriverDate,
                      DriverFileName, Virtual, Hidden |
        Format-List

    "`n--- leftover virtual adapters ---"
    "(virtual, not hidden, AdminStatus=Up but Status != Up)"
    Get-NetAdapter -IncludeHidden |
        Where-Object { $_.Virtual -and -not $_.Hidden -and
                       $_.AdminStatus -eq 'Up' -and $_.Status -ne 'Up' } |
        Select-Object Name, InterfaceDescription, ifIndex, Status,
                      DriverProvider, DriverFileName |
        Format-Table -AutoSize

    "`n--- APIPA / deprecated addresses (sign of a stuck interface) ---"
    Get-NetIPAddress -ErrorAction SilentlyContinue |
        Where-Object { $_.IPAddress -like '169.254.*' -or
                       $_.AddressState -in 'Tentative','Deprecated','Invalid' } |
        Select-Object InterfaceAlias, ifIndex, IPAddress, AddressState |
        Format-Table -AutoSize
}

# --- 3. Connection type (cellular / wifi / ethernet) ------------------------
Save "link-type" {
    $medium = @{
        0='Unspecified'; 1='Wireless'; 2='CableModem'; 3='PhoneLine'; 4='PowerLine'
        5='DSL'; 6='FibreChannel'; 7='1394'; 8='WirelessWan (CELLULAR)'
        9='Native802.11 (WIFI)'; 10='Bluetooth'; 11='Infiniband'; 12='WiMax'
        13='UWB'; 14='802.3 (ETHERNET)'; 15='802.5'; 16='Irda'; 17='WiredWan'
        18='WiredCoWanDevice'; 19='Other'
    }
    Get-NetAdapter -IncludeHidden |
        Select-Object Name, ifIndex, InterfaceType, NdisPhysicalMedium,
                      @{n='MediumName';e={ $medium[[int]$_.NdisPhysicalMedium] }},
                      MediaConnectionState, Virtual |
        Format-Table -AutoSize

    "`n--- interface carrying the default route (determines the link type) ---"
    Get-NetRoute -DestinationPrefix '0.0.0.0/0' -ErrorAction SilentlyContinue |
        ForEach-Object {
            $a = Get-NetAdapter -InterfaceIndex $_.ifIndex -ErrorAction SilentlyContinue
            [pscustomobject]@{
                ifIndex     = $_.ifIndex
                Alias       = $_.InterfaceAlias
                NextHop     = $_.NextHop
                TotalMetric = $_.RouteMetric + $_.InterfaceMetric
                Medium      = if ($a) { $medium[[int]$a.NdisPhysicalMedium] } else { 'n/a' }
                Virtual     = if ($a) { $a.Virtual } else { 'n/a' }
            }
        } | Sort-Object TotalMetric | Format-Table -AutoSize

    "`n--- Connection profiles ---"
    Get-NetConnectionProfile | Select-Object Name, InterfaceAlias, NetworkCategory,
                                             IPv4Connectivity, IPv6Connectivity |
        Format-List
}

# --- 4. Drivers: network filters and tunnels --------------------------------
# Catches WinDivert-style filters, TAP/Wintun, and split-tunnel drivers.
# Matched by exact driver/service name rather than a substring, since a
# substring like "tap" also matches unrelated services (TapiSrv, EntAppSvc,
# XboxNetApiSvc on stock Windows).
Save "drivers-known" {
    $known = @{
        'wintun'                = 'Wintun (WireGuard/Amnezia/tun2socks)'
        'wireguard'             = 'WireGuard NT'
        'tap0901'               = 'TAP-Windows (OpenVPN)'
        'tapwindscribe0901'     = 'Windscribe (TAP)'
        'windtun420'            = 'Windscribe (Wintun)'
        'tapnordvpn'            = 'NordVPN (TAP)'
        'tapprotonvpn'          = 'ProtonVPN (TAP)'
        'mullvad-split-tunnel'  = 'Mullvad split tunnel'
        'mullvad-wireguard'     = 'Mullvad WireGuard'
        'Tailscale'             = 'Tailscale'
        'WinDivert'             = 'WinDivert (used by DPI circumvention tools)'
        'WinDivert1.4'          = 'WinDivert 1.4'
        'adgnetworkwfpdrv'      = 'AdGuard (WFP callout)'
        'adguardsvc'            = 'AdGuard service'
    }
    "--- exact-name matches ---"
    Get-CimInstance Win32_SystemDriver -ErrorAction SilentlyContinue |
        Where-Object { $known.ContainsKey($_.Name) } |
        Select-Object Name, @{n='Identified';e={ $known[$_.Name] }},
                      State, StartMode, PathName |
        Format-Table -AutoSize

    "`n--- all WFP callout / NDIS filter drivers (traffic interception) ---"
    Get-CimInstance Win32_SystemDriver -ErrorAction SilentlyContinue |
        Where-Object { $_.PathName -match 'drivers' -and
                       $_.Name -match 'wfp|ndis|filter|divert|tun|tap' } |
        Select-Object Name, DisplayName, State, StartMode |
        Sort-Object Name | Format-Table -AutoSize
}

Save "drivers-store" {
    if ($isAdmin) { pnputil /enum-drivers } else { "Requires administrator rights." }
}

Save "drivers-net-class" {
    Get-CimInstance Win32_PnPSignedDriver -ErrorAction SilentlyContinue |
        Where-Object { $_.DeviceClass -eq 'NET' } |
        Select-Object DeviceName, DriverProviderName, DriverVersion,
                      DriverDate, InfName, IsSigned, Signer |
        Sort-Object DriverProviderName | Format-List

    "`n--- network driver providers (grouping by Provider is more reliable than by inf name) ---"
    "e.g. Windscribe ships its TAP driver under oemvista.inf, so the inf name alone is not reliable."
    Get-CimInstance Win32_PnPSignedDriver -ErrorAction SilentlyContinue |
        Where-Object { $_.DeviceClass -eq 'NET' } |
        Group-Object DriverProviderName |
        Select-Object Count, Name | Format-Table -AutoSize
}

# --- 5. Routes + conflict analysis on TOTAL metric --------------------------
Save "routes" {
    "--- route print ---"
    route print

    "`n--- Get-NetRoute with total metric ---"
    Get-NetRoute -ErrorAction SilentlyContinue |
        Select-Object AddressFamily, DestinationPrefix, NextHop, ifIndex,
                      InterfaceAlias, RouteMetric, InterfaceMetric,
                      @{n='TotalMetric';e={ $_.RouteMetric + $_.InterfaceMetric }},
                      Protocol |
        Sort-Object AddressFamily, DestinationPrefix, TotalMetric |
        Format-Table -AutoSize

    "`n--- default route conflicts ---"
    "Amnezia's norm: two default routes, the tunnel wins on the LOWER interface metric."
    "Conflict: equal total metrics between the tunnel and another default route."
    foreach ($p in @('0.0.0.0/0','::/0')) {
        "--- $p ---"
        $rs = Get-NetRoute -DestinationPrefix $p -ErrorAction SilentlyContinue |
              Select-Object ifIndex, InterfaceAlias, NextHop,
                            @{n='TotalMetric';e={ $_.RouteMetric + $_.InterfaceMetric }} |
              Sort-Object TotalMetric
        $rs | Format-Table -AutoSize
        if ($rs.Count -gt 1) {
            $g = $rs | Group-Object TotalMetric | Where-Object { $_.Count -gt 1 }
            if ($g) { "!!! CONFLICT: multiple default routes with equal metric $($g.Name)" }
            else    { "OK: winner is $($rs[0].InterfaceAlias) (metric $($rs[0].TotalMetric))" }
        }
    }

    "`n--- private subnet reachability vs tunnel ---"
    "If a private subnet route prefers the tunnel, LAN (router, NAS, printer) becomes unreachable."
    Get-NetRoute -ErrorAction SilentlyContinue |
        Where-Object { $_.DestinationPrefix -match '^(10\.|172\.(1[6-9]|2\d|3[01])\.|192\.168\.)' } |
        Select-Object DestinationPrefix, ifIndex, InterfaceAlias,
                      @{n='TotalMetric';e={ $_.RouteMetric + $_.InterfaceMetric }}, Protocol |
        Sort-Object DestinationPrefix, TotalMetric | Format-Table -AutoSize

    "`n--- interface metrics ---"
    Get-NetIPInterface | Select-Object ifIndex, InterfaceAlias, AddressFamily,
                                       InterfaceMetric, AutomaticMetric, Dhcp,
                                       ConnectionState |
        Sort-Object ifIndex | Format-Table -AutoSize
}

# --- 6. Actual routing stack resolution --------------------------------------
# What you can't see in route print: where the packet will ACTUALLY go.
# Also checks the route to the endpoint's own next-hop, since that's where a
# routing loop through the tunnel would actually show up.
Save "route-resolution" {
    $targets = @('8.8.8.8','1.1.1.1')
    if ($Endpoint) { $targets = @($Endpoint) + $targets }

    if ($Endpoint) {
        $ep = Find-NetRoute -RemoteIPAddress $Endpoint -ErrorAction SilentlyContinue |
              Where-Object { $_.NextHop -and $_.NextHop -ne '0.0.0.0' } |
              Select-Object -First 1
        if ($ep) { $targets += $ep.NextHop }
    }

    foreach ($t in ($targets | Select-Object -Unique)) {
        "=== Find-NetRoute -RemoteIPAddress $t ==="
        Find-NetRoute -RemoteIPAddress $t -ErrorAction SilentlyContinue |
            Select-Object IPAddress, InterfaceAlias, ifIndex,
                          DestinationPrefix, NextHop, RouteMetric |
            Format-List
        ""
    }

    if ($Endpoint) {
        "=== endpoint reachability ==="
        $r = Find-NetRoute -RemoteIPAddress $Endpoint -ErrorAction SilentlyContinue |
             Select-Object -First 1
        $a = if ($r) { Get-NetAdapter -InterfaceIndex $r.ifIndex -ErrorAction SilentlyContinue }
        if ($a -and $a.Virtual) {
            "!!! LOOP: traffic to server $Endpoint goes through the VIRTUAL/TUNNEL interface '$($a.Name)'"
        } elseif ($a) {
            "OK: traffic to server goes through the physical interface '$($a.Name)'"
        } else {
            "Could not determine the interface."
        }

        "`n--- host route /32 to endpoint (anti-loop guard) ---"
        Get-NetRoute -DestinationPrefix "$Endpoint/32" -ErrorAction SilentlyContinue |
            Select-Object DestinationPrefix, NextHop, ifIndex, InterfaceAlias,
                          RouteMetric, InterfaceMetric | Format-Table -AutoSize
    }
}

# --- 7. DNS + leak risk assessment -------------------------------------------
Save "dns" {
    "--- configured servers (only interfaces that have any) ---"
    Get-DnsClientServerAddress -ErrorAction SilentlyContinue |
        Where-Object { $_.ServerAddresses.Count -gt 0 } |
        Select-Object InterfaceAlias, InterfaceIndex,
                      @{n='Family';e={ if ($_.AddressFamily -eq 2) {'IPv4'} else {'IPv6'} }},
                      @{n='Servers';e={ $_.ServerAddresses -join ', ' }} |
        Format-Table -AutoSize

    "`n--- global settings ---"
    Get-DnsClientGlobalSetting | Format-List

    "`n--- NRPT rules (VPN clients write here to bind DNS to the tunnel) ---"
    $nrpt = Get-DnsClientNrptRule -ErrorAction SilentlyContinue
    if ($nrpt) { $nrpt | Format-List } else { "No NRPT rules." }

    "`n--- DoH configuration (cmdlet not present on every build) ---"
    if (Has 'Get-DnsClientDohServerAddress') {
        Get-DnsClientDohServerAddress | Format-Table -AutoSize
    } else { "Get-DnsClientDohServerAddress is not available on this Windows build." }

    "`n--- DNS leak risk ---"
    $tunIdx = Get-NetRoute -DestinationPrefix '0.0.0.0/0' -ErrorAction SilentlyContinue |
              Sort-Object { $_.RouteMetric + $_.InterfaceMetric } |
              Select-Object -First 1 -ExpandProperty ifIndex
    $withDns = Get-DnsClientServerAddress -AddressFamily IPv4 -ErrorAction SilentlyContinue |
               Where-Object { $_.ServerAddresses.Count -gt 0 }
    "Default route goes through ifIndex=$tunIdx"
    "Interfaces with DNS configured: $($withDns.Count)"
    $other = $withDns | Where-Object { $_.InterfaceIndex -ne $tunIdx }
    if ($other -and -not $nrpt) {
        "!!! LEAK RISK: DNS is configured on more than just the active interface, and there are no NRPT rules."
        $other | Select-Object InterfaceAlias, InterfaceIndex,
                               @{n='Servers';e={ $_.ServerAddresses -join ', ' }} |
            Format-Table -AutoSize
    } else { "No obvious risk." }

    "`n--- Smart multi-homed name resolution (policy) ---"
    Get-ItemProperty 'HKLM:\SOFTWARE\Policies\Microsoft\Windows NT\DNSClient' `
        -ErrorAction SilentlyContinue |
        Select-Object DisableSmartNameResolution, DisableParallelAandAAAA | Format-List
}

# --- 8. IPv6 and localhost ---------------------------------------------
Save "ipv6-localhost" {
    "--- IP addresses ---"
    Get-NetIPAddress -ErrorAction SilentlyContinue |
        Select-Object InterfaceAlias, ifIndex, IPAddress, PrefixLength,
                      @{n='Family';e={ $_.AddressFamily }}, AddressState, SkipAsSource |
        Sort-Object Family, InterfaceAlias | Format-Table -AutoSize

    "`n--- DisabledComponents (0=enabled, 8=prefer IPv4, 255=disabled, 32=...) ---"
    Get-ItemProperty 'HKLM:\SYSTEM\CurrentControlSet\Services\Tcpip6\Parameters' `
        -Name DisabledComponents -ErrorAction SilentlyContinue |
        Select-Object DisabledComponents | Format-List

    "`n--- IPv6 leak risk ---"
    "Sign: ::/0 goes through the tunnel, but the tunnel only has ULA (fc00::/7) addresses - no real connectivity."
    $v6def = Get-NetRoute -DestinationPrefix '::/0' -ErrorAction SilentlyContinue |
             Sort-Object { $_.RouteMetric + $_.InterfaceMetric } | Select-Object -First 1
    if ($v6def) {
        "::/0 goes through '$($v6def.InterfaceAlias)' (ifIndex $($v6def.ifIndex))"
        $addrs = Get-NetIPAddress -InterfaceIndex $v6def.ifIndex -AddressFamily IPv6 `
                   -ErrorAction SilentlyContinue
        $addrs | Select-Object IPAddress, PrefixLength, AddressState | Format-Table -AutoSize
        $global = $addrs | Where-Object {
            $_.IPAddress -notmatch '^fe80|^fc|^fd' }
        if (-not $global) {
            "!!! Only ULA/link-local addresses - real IPv6 traffic will leak outside the tunnel."
        }
        Get-NetConnectionProfile -InterfaceIndex $v6def.ifIndex -ErrorAction SilentlyContinue |
            Select-Object InterfaceAlias, IPv6Connectivity | Format-List
    } else { "::/0 is absent - no IPv6 routing." }

    "`n--- localhost resolution ---"
    Resolve-DnsName localhost -ErrorAction SilentlyContinue | Format-Table -AutoSize
    "`n--- hosts file (only non-empty, non-commented lines) ---"
    Get-Content "$env:SystemRoot\System32\drivers\etc\hosts" -ErrorAction SilentlyContinue |
        Where-Object { $_ -match '\S' -and $_ -notmatch '^\s*#' }
}

# --- 9. Proxy: system and browser (WinINET) ----------------------------
# Key block for Xray: it often works through the system proxy.
Save "proxy" {
    "--- env vars ---"
    Get-ChildItem Env: | Where-Object { $_.Name -match 'PROXY' } | Format-Table -AutoSize
    "`n--- WinINET (HKCU, what the browser sees) ---"
    Get-ItemProperty 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Internet Settings' `
        -ErrorAction SilentlyContinue |
        Select-Object ProxyEnable, ProxyServer, ProxyOverride, AutoConfigURL | Format-List
    "`n--- WinHTTP ---"
    netsh winhttp show proxy
}

# --- 10. VPN/antiDPI services and processes ----------------------------
# Matched by exact name rather than a broad substring, since a substring
# match also pulls in unrelated software installed on the machine.
Save "services-processes" {
    $svc = @('AmneziaVPN-service','wintun','windtun420','tapwindscribe0901',
             'tap0901','Tailscale','WinDivert','WinDivert1.4','adgnetworkwfpdrv',
             'adguardsvc','mullvad-split-tunnel','mullvad-wireguard','MullvadVPN',
             'NordVPN','ProtonVPNService','OpenVPNService','OpenVPNServiceInteractive')
    "--- services matching known exact names (+ per-tunnel Amnezia services) ---"
    Get-Service -ErrorAction SilentlyContinue |
        Where-Object { $svc -contains $_.Name -or $_.Name -like 'AmneziaWGTunnel$*' } |
        Select-Object Name, DisplayName, Status, StartType | Format-Table -AutoSize

    $proc = @('AmneziaVPN','AmneziaVPN-service','wireguard','openvpn','mullvad-daemon',
              'tailscaled','nfqws','winws','goodbyedpi','byedpi','ciadpi',
              'xray','v2ray','sing-box','tun2socks','clash-verge','Windscribe')
    "`n--- processes matching known exact names ---"
    Get-Process -ErrorAction SilentlyContinue |
        Where-Object { $proc -contains $_.ProcessName } |
        Select-Object ProcessName, Id, Path | Format-Table -AutoSize

    "`n--- listening sockets belonging to those processes ---"
    Get-NetTCPConnection -State Listen -ErrorAction SilentlyContinue |
        ForEach-Object {
            $p = Get-Process -Id $_.OwningProcess -ErrorAction SilentlyContinue
            if ($p -and $proc -contains $p.ProcessName) {
                [pscustomobject]@{ Addr=$_.LocalAddress; Port=$_.LocalPort; Process=$p.ProcessName }
            }
        } | Sort-Object Port | Format-Table -AutoSize

    "`n--- UDP endpoints belonging to those processes ---"
    Get-NetUDPEndpoint -ErrorAction SilentlyContinue |
        ForEach-Object {
            $p = Get-Process -Id $_.OwningProcess -ErrorAction SilentlyContinue
            if ($p -and $proc -contains $p.ProcessName) {
                [pscustomobject]@{ Addr=$_.LocalAddress; Port=$_.LocalPort; Process=$p.ProcessName }
            }
        } | Sort-Object Port | Format-Table -AutoSize
}

# --- 11. Firewall / WFP -----------------------------------------------------
Save "firewall" {
    Get-NetFirewallProfile | Select-Object Name, Enabled, DefaultInboundAction,
                                           DefaultOutboundAction | Format-Table -AutoSize
    "`n--- enabled outbound block rules ---"
    Get-NetFirewallRule -Enabled True -Direction Outbound -Action Block `
        -ErrorAction SilentlyContinue |
        Select-Object DisplayName, Profile, Owner | Format-Table -AutoSize
    "`n--- third-party security products ---"
    Get-CimInstance -Namespace root\SecurityCenter2 -ClassName AntiVirusProduct `
        -ErrorAction SilentlyContinue |
        Select-Object displayName, productState, pathToSignedProductExe | Format-List
    Get-CimInstance -Namespace root\SecurityCenter2 -ClassName FirewallProduct `
        -ErrorAction SilentlyContinue | Select-Object displayName | Format-List
}

# --- 12. WFP filters (heavy output, separate file) --------------------------
Save "wfp-filters" {
    if (-not $isAdmin) { "Requires administrator rights (would be ACCESS_DENIED otherwise)." }
    else {
        $tmp = Join-Path $env:TEMP "wfp-$Label.xml"
        netsh wfp show filters file="$tmp" 2>&1
        if (Test-Path $tmp) {
            $x = [xml](Get-Content $tmp)
            "Total filters: $($x.wfpdiag.filters.item.Count)"
            "`n--- filter providers (who intercepts traffic) ---"
            $x.wfpdiag.filters.item |
                Group-Object { $_.displayData.name } |
                Sort-Object Count -Descending |
                Select-Object -First 40 Count, Name | Format-Table -AutoSize
            Remove-Item $tmp -Force -ErrorAction SilentlyContinue
        }
    }
}

Write-Host "`nDone. Folder: $out"
Write-Host "Run three times: clean (VPN off) / awg / xray, then compare."

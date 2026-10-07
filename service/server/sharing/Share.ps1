# Runs as a child of the Amnezia service. No credentials on disk or argv.
$ErrorActionPreference = 'Stop'
[Console]::InputEncoding = [Text.UTF8Encoding]::new($false)
[Console]::OutputEncoding = [Text.UTF8Encoding]::new($false)
Set-StrictMode -Version Latest
function Report($state, $message, $data = @{}) {
    $report = @{state=$state;message=$message}
    foreach ($key in $data.Keys) { $report[$key] = $data[$key] }
    [Console]::Out.WriteLine(($report | ConvertTo-Json -Compress))
    [Console]::Out.Flush()
}
function Write-ShareDiagnostic([string]$message) {
    try {
        $path = Join-Path $env:ProgramData 'AmneziaVPN Share\log\share-worker.log'
        Add-Content -LiteralPath $path -Encoding UTF8 -Value ('{0} {1}' -f [DateTime]::Now.ToString('o'), $message)
    } catch { }
}
function Invoke-ShareCommand($fileName, $arguments, $timeoutMs = 5000) {
    $info = New-Object Diagnostics.ProcessStartInfo
    $info.FileName = $fileName
    $info.Arguments = $arguments
    $info.UseShellExecute = $false
    $info.CreateNoWindow = $true
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    $process = [Diagnostics.Process]::Start($info)
    $stdoutTask = $process.StandardOutput.ReadToEndAsync()
    $stderrTask = $process.StandardError.ReadToEndAsync()
    if (!$process.WaitForExit($timeoutMs)) {
        $process.Kill()
        $process.Dispose()
        throw "Network command timed out: $fileName $arguments"
    }
    $result = [pscustomobject]@{ ExitCode=$process.ExitCode; Output=$stdoutTask.Result.Trim(); Error=$stderrTask.Result.Trim() }
    $process.Dispose()
    return $result
}
function Set-ShareIPv4Forwarding([int]$InterfaceIndex, [string]$State) {
    if ($State -notin @('Enabled','Disabled')) { throw "Invalid IPv4 forwarding state: $State" }
    $value = $State.ToLowerInvariant()
    $netsh = Join-Path $env:SystemRoot 'System32\netsh.exe'
    $result = Invoke-ShareCommand $netsh "interface ipv4 set interface $InterfaceIndex forwarding=$value store=active" 5000
    if ($result.ExitCode -ne 0) {
        throw "netsh could not set IPv4 forwarding on interface $InterfaceIndex`: $($result.Error) $($result.Output)"
    }
    $verified = Get-NetIPInterface -InterfaceIndex $InterfaceIndex -AddressFamily IPv4 -ErrorAction Stop
    if ($verified.Forwarding.ToString() -ne $State) {
        throw "IPv4 forwarding did not become $State on interface $InterfaceIndex."
    }
}
Report 'starting' 'Загружаю модуль сетевой службы…'
Import-Module (Join-Path $PSScriptRoot 'Hotspot.psm1') -Force
Report 'starting' 'Модуль загружен; принимаю параметры запуска…'
$inputReader = [System.IO.StreamReader]::new(
    [Console]::OpenStandardInput(),
    [Text.UTF8Encoding]::new($false),
    $false)
$bridge = $null
$hotspotStarted = $false
$routeAdded = $false
$tapOverrideRoutesAdded = @()
$tapMirroredRoutesAdded = @()
$snapshot = @()
$failed = $false
$configChanged = $false
$icsConfigured = $false
$natName = 'AmneziaVPNShare'
$hotspotForwardingOriginal = $null
$hotspotClampMssOriginal = $null
$vpnForwardingOriginal = $null
$tapForwardingOriginal = $null
$tapMtuOriginal = $null
$tapClampMssOriginal = $null
$tapAutomaticMetricOriginal = $null
$tapInterfaceMetricOriginal = $null
$uplinkAdapter = $null
$uplinkDnsChanged = $false
$uplinkDnsWasAutomatic = $false
$uplinkDnsOriginal = @()
$dohSnapshots = @()
$privateId = $null
$privateAdapter = $null
$vpnAdapter = $null
$readyFile = $null
$lastForwardingRefresh = [DateTime]::MinValue
try {
    # Keep the inherited control pipe synchronous until the TAP permission
    # handshake is complete. Mixing ReadLineAsync and ReadLine on PowerShell
    # 5.1 can leave the second read waiting after the parent writes "allow".
    $configLine = $inputReader.ReadLine()
    if ($null -eq $configLine) { throw 'The service closed the sharing configuration pipe.' }
    $cfg = $configLine | ConvertFrom-Json
    if ($cfg.address -ne '127.0.0.1' -or $cfg.port -lt 1 -or $cfg.port -gt 65535) { throw 'Invalid local XRay endpoint.' }
    if (!$cfg.firewallReplyFile) { throw 'The service did not provide a TAP firewall response channel.' }
    if (!$cfg.hotspotFirewallReplyFile) { throw 'The service did not provide a Wi-Fi DHCP firewall response channel.' }
    if (!$cfg.stopFile) { throw 'The service did not provide a stop-signal channel.' }
    Set-ShareControlTask $null $cfg.stopFile
    Report 'starting' 'Проверяю виртуальный TAP-адаптер…'
    $tap = Get-NetAdapter -Name 'Amnezia Share' -ErrorAction SilentlyContinue
    if (!$tap) {
        & (Join-Path $PSScriptRoot 'tapctl.exe') create --name 'Amnezia Share' --hwid tap0901 | Out-Null
        if ($LASTEXITCODE -ne 0) { throw 'Cannot create TAP adapter. Install the TAP-Windows driver first.' }
        $tap = Get-NetAdapter -Name 'Amnezia Share'
    }
    if ($tap.InterfaceDescription -notmatch 'TAP-Windows') { throw 'Amnezia Share name belongs to a non-TAP adapter.' }
    Assert-ShareNotStopping
    Report 'starting' 'Проверяю конфликты сети…'
    $conflicts = @(Get-NetIPAddress -AddressFamily IPv4 | Where-Object { $_.IPAddress -like '10.254.254.*' -and $_.InterfaceIndex -ne $tap.ifIndex })
    if ($conflicts.Count) { throw 'Sharing subnet 10.254.254.0/24 is already in use.' }
    Report 'starting' 'Проверяю настройки общего доступа Windows…'
    $snapshot = @(Get-SharingEntries | Where-Object Enabled | Select-Object Id,Role)
    if ($snapshot.Count) { throw 'Internet sharing is already in use. Stop Connectify or the existing hotspot first.' }
    $vpnAdapter = Get-NetAdapter -IncludeHidden | Where-Object {
        $_.Status -eq 'Up' -and $_.InterfaceDescription -match 'tun2socks Tunnel'
    } | Select-Object -First 1
    if (!$vpnAdapter) { throw 'Connect XRay in TUN mode before enabling sharing.' }
    $vpnRoutes = @('1.1.1.1','8.8.8.8','9.9.9.9','100.64.0.1','130.49.163.95','203.0.113.1') | ForEach-Object {
        $target = $_
        $route = Find-NetRoute -RemoteIPAddress $target -ErrorAction Stop |
            Where-Object { $_.CimClass.CimClassName -eq 'MSFT_NetRoute' } | Select-Object -First 1
        [pscustomobject]@{ Address=$target; InterfaceIndex=[int]$route.InterfaceIndex; InterfaceAlias=$route.InterfaceAlias }
    }
    $wrongVpnRoutes = @($vpnRoutes | Where-Object InterfaceIndex -ne $vpnAdapter.ifIndex)
    if ($wrongVpnRoutes.Count) {
        throw ('XRay TUN does not carry every checked IPv4 route: ' + (($wrongVpnRoutes | ForEach-Object { '{0} via {1}' -f $_.Address,$_.InterfaceAlias }) -join ', '))
    }
    Assert-ShareNotStopping
    Report 'starting' 'Получаю профиль Wi-Fi…'
    $seed = Get-SeedManager
    $uplinkAdapter = Get-NetAdapter -Physical | Where-Object {
        ([guid]$_.InterfaceGuid).ToString() -eq ([guid]$seed).ToString()
    } | Select-Object -First 1
    if (!$uplinkAdapter) { throw 'Cannot identify the physical network adapter used to start the hotspot.' }
    $uplinkDnsOriginal = @((Get-DnsClientServerAddress -InterfaceIndex $uplinkAdapter.ifIndex -AddressFamily IPv4 -ErrorAction Stop).ServerAddresses)
    $uplinkDnsKey = "HKLM:\SYSTEM\CurrentControlSet\Services\Tcpip\Parameters\Interfaces\$(([guid]$uplinkAdapter.InterfaceGuid).ToString('B'))"
    $uplinkNameServerOverride = (Get-ItemProperty -LiteralPath $uplinkDnsKey -Name NameServer -ErrorAction SilentlyContinue).NameServer
    $uplinkDnsWasAutomatic = [string]::IsNullOrWhiteSpace([string]$uplinkNameServerOverride)
    $vpnClientDns = @('1.1.1.1','9.9.9.9')
    if (($uplinkDnsOriginal -join ',') -ne ($vpnClientDns -join ',')) {
        Report 'starting' 'Настраиваю DNS через VPN для подключённых устройств…'
        Set-DnsClientServerAddress -InterfaceIndex $uplinkAdapter.ifIndex -ServerAddresses $vpnClientDns -ErrorAction Stop
        $uplinkDnsChanged = $true
    }
    # Windows Mobile Hotspot runs a DNS proxy on 192.168.137.1. Plain UDP DNS
    # through SOCKS can time out intermittently, which makes Android mark the
    # network as having no Internet even while HTTPS traffic works. Make the
    # host resolver use DoH so the hotspot proxy's upstream queries travel over
    # reliable TCP/TLS through XRay.
    if (Get-Command Get-DnsClientDohServerAddress -ErrorAction SilentlyContinue) {
        $dohTemplates = @{
            '1.1.1.1' = 'https://cloudflare-dns.com/dns-query'
            '9.9.9.9' = 'https://dns.quad9.net/dns-query'
        }
        foreach ($server in $vpnClientDns) {
            $doh = Get-DnsClientDohServerAddress -ServerAddress $server -ErrorAction Stop
            $dohSnapshots += [pscustomobject]@{
                ServerAddress = $server
                DohTemplate = [string]$doh.DohTemplate
                AllowFallbackToUdp = [bool]$doh.AllowFallbackToUdp
                AutoUpgrade = [bool]$doh.AutoUpgrade
            }
            Set-DnsClientDohServerAddress -ServerAddress $server -DohTemplate $dohTemplates[$server] -AllowFallbackToUdp $false -AutoUpgrade $true -ErrorAction Stop
        }
        Clear-DnsClientCache -ErrorAction SilentlyContinue
    }
    Report 'starting' 'Проверяю состояние мобильной точки…'
    $hotspotState = Get-HotspotState
    if ($hotspotState -ne 'Off') {
        # Recover an orphaned Amnezia hotspot after a forced service update or
        # worker crash. The app-owned NAT plus a disconnected dedicated TAP is
        # evidence that our previous sharing worker is gone; do not leave the
        # Windows hotspot stranded and make the user turn it off manually.
        $shareNat = @(Get-NetNat -Name $natName -ErrorAction SilentlyContinue)
        $tapState = Get-NetAdapter -Name 'Amnezia Share' -ErrorAction SilentlyContinue
        if ($shareNat.Count -gt 0 -and (!$tapState -or $tapState.Status -ne 'Up')) {
            Report 'starting' 'Останавливаю оставшуюся точку доступа после сбоя…'
            Disable-Hotspot
            if ((Get-HotspotState) -ne 'Off') { throw 'Windows did not stop the orphaned Amnezia hotspot.' }
            Write-ShareDiagnostic 'Stopped orphaned Amnezia hotspot before restarting Tunnel Sharing.'
        } else {
            throw 'Turn off the existing Windows mobile hotspot before starting Tunnel Sharing.'
        }
    }
    # Older builds used WinNAT here. The TAP bridge is a userspace IP router
    # that returns packets addressed to the original Wi-Fi client; source NAT
    # rewrites those packets to the TAP host address and can prevent Windows
    # from routing replies back to the client. Remove only our obsolete NAT.
    $staleShareNat = @(Get-NetNat -Name $natName -ErrorAction SilentlyContinue)
    foreach ($staleNat in $staleShareNat) {
        Write-ShareDiagnostic ('Removing obsolete Amnezia WinNAT ' + $staleNat.Name + ' for prefix ' + $staleNat.InternalIPInterfaceAddressPrefix)
        $staleNat | Remove-NetNat -Confirm:$false -ErrorAction Stop
    }
    Report 'starting' 'Читаю текущие настройки Wi-Fi…'
    $previousConfig = Get-HotspotConfig
    Report 'starting' 'Применяю имя Wi-Fi и пароль…'
    Set-HotspotConfig $cfg.ssid $cfg.wifiPassword
    $configChanged = $true
    Assert-ShareNotStopping
    Report 'starting' 'Запускаю сетевой мост…'
    $readyFile = Join-Path $env:ProgramData ("AmneziaVPN Share\log\tapbridge-" + [guid]::NewGuid().ToString('N') + '.ready')
    $traceFile = [System.IO.Path]::ChangeExtension($readyFile, '.trace')
    $startInfo = New-Object Diagnostics.ProcessStartInfo
    $startInfo.FileName = Join-Path $PSScriptRoot 'tapbridge.exe'
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardInput = $true
    $startInfo.RedirectStandardOutput = $false
    $startInfo.RedirectStandardError = $true
    $bridge = [Diagnostics.Process]::Start($startInfo)
    $bridgeErrorTask = $bridge.StandardError.ReadToEndAsync()
    $bridge.StandardInput.WriteLine((@{adapter='Amnezia Share';address=$cfg.address;port=$cfg.port;username=$cfg.username;password=$cfg.password;readyFile=$readyFile;traceFile=$traceFile} | ConvertTo-Json -Compress))
    $bridge.StandardInput.Flush()
    $readyDeadline = (Get-Date).AddSeconds(40)
    while (!(Test-Path -LiteralPath $readyFile) -and !$bridge.HasExited -and (Get-Date) -lt $readyDeadline) {
        Start-Sleep -Milliseconds 100
    }
    if (!(Test-Path -LiteralPath $readyFile)) {
        $bridgeTrace = if (Test-Path -LiteralPath $traceFile) { (Get-Content -LiteralPath $traceFile -Tail 6) -join '; ' } else { 'no bridge trace was written' }
        if ($bridge.HasExited) {
            $bridgeError = if ($bridgeErrorTask.IsCompleted) { $bridgeErrorTask.Result.Trim() } else { '' }
            throw "TAP bridge exited during startup (code $($bridge.ExitCode)): $bridgeError [$bridgeTrace]"
        }
        throw "TAP bridge did not become ready within 40 seconds. [$bridgeTrace]"
    }
    Remove-Item -LiteralPath $readyFile -Force -ErrorAction SilentlyContinue
    $readyFile = $null
    Report 'tap-ready' 'Waiting for the TAP-only firewall rule'
    $replyDeadline = (Get-Date).AddSeconds(60)
    while (!(Test-Path -LiteralPath $cfg.firewallReplyFile) -and (Get-Date) -lt $replyDeadline) {
        Assert-ShareNotStopping
        Start-Sleep -Milliseconds 100
    }
    if (!(Test-Path -LiteralPath $cfg.firewallReplyFile)) { throw 'The service did not answer the TAP firewall request within 60 seconds.' }
    $permit = (Get-Content -LiteralPath $cfg.firewallReplyFile -Raw).Trim()
    Remove-Item -LiteralPath $cfg.firewallReplyFile -Force -ErrorAction SilentlyContinue
    if ($permit -eq 'stop') { throw 'Sharing startup was canceled.' }
    if ($permit -ne 'allow') {
        $denial = $permit | ConvertFrom-Json -ErrorAction SilentlyContinue
        if ($denial.message) { throw $denial.message }
        throw 'The service rejected the TAP firewall request.'
    }
    Report 'starting' 'Настраиваю TAP и точку доступа…'
    $controlTask = $inputReader.ReadLineAsync()
    Set-ShareControlTask $controlTask $cfg.stopFile
    Report 'starting' 'Настраиваю IPv4 TAP-адаптера…'
    $existing = @(Get-NetIPAddress -InterfaceIndex $tap.ifIndex -AddressFamily IPv4 -ErrorAction SilentlyContinue | Where-Object IPAddress -eq '10.254.254.1')
    if (!$existing.Count) { New-NetIPAddress -InterfaceIndex $tap.ifIndex -IPAddress '10.254.254.1' -PrefixLength 24 | Out-Null }
    Report 'starting' 'Настраиваю DNS TAP-адаптера…'
    $dns = Get-DnsClientServerAddress -InterfaceIndex $tap.ifIndex -AddressFamily IPv4 -ErrorAction SilentlyContinue
    $desiredDns = @('1.1.1.1','9.9.9.9')
    if (!$dns -or (@($dns.ServerAddresses) -join ',') -ne ($desiredDns -join ',')) {
        Set-DnsClientServerAddress -InterfaceIndex $tap.ifIndex -ServerAddresses $desiredDns
    }
    Report 'starting' 'Добавляю маршрут сетевого моста…'
    $routeExe = Join-Path $env:SystemRoot 'System32\route.exe'
    $tapDefaultRoutes = @(Get-NetRoute -InterfaceIndex $tap.ifIndex -DestinationPrefix '0.0.0.0/0' -ErrorAction SilentlyContinue)
    $ownedTapRoute = @($tapDefaultRoutes | Where-Object NextHop -eq '10.254.254.2')
    if ($ownedTapRoute.Count) {
        # A prior worker may have stopped before removing its route.
        $routeAdded = $true
    } elseif ($tapDefaultRoutes.Count) {
        throw 'The TAP adapter already has a different default route.'
    } else {
        $routeArgs = "ADD 0.0.0.0 MASK 0.0.0.0 10.254.254.2 METRIC 9999 IF $($tap.ifIndex)"
        $routeResult = Invoke-ShareCommand $routeExe $routeArgs 5000
        if ($routeResult.ExitCode -ne 0) {
            throw "Cannot add TAP bridge route: $($routeResult.Output) $($routeResult.Error)"
        }
        $routeAdded = $true
    }
    # XRay TUN owns two /1 routes. Prefer the TAP bridge for both /1 halves
    # while sharing is active; tapbridge forwards them to XRay's local SOCKS
    # inbound and returns packets with the original Wi-Fi client address.
    $tapMetric = Get-NetIPInterface -InterfaceIndex $tap.ifIndex -AddressFamily IPv4 -ErrorAction Stop
    $tapAutomaticMetricOriginal = $tapMetric.AutomaticMetric.ToString()
    $tapInterfaceMetricOriginal = [int]$tapMetric.InterfaceMetric
    $tapMtuOriginal = [int]$tapMetric.NlMtu
    $tapClampMssOriginal = $tapMetric.ClampMss.ToString()
    if ($tapAutomaticMetricOriginal -ne 'Disabled' -or $tapInterfaceMetricOriginal -ne 1) {
        Set-NetIPInterface -InterfaceIndex $tap.ifIndex -AddressFamily IPv4 -AutomaticMetric Disabled -InterfaceMetric 1 -PolicyStore ActiveStore -ErrorAction Stop
    }
    # The hotspot path adds TAP and XRay/SOCKS encapsulation. Clamp TCP MSS and
    # use a conservative MTU so larger HTTPS/video responses do not
    # disappear when an upstream path drops IPv4 fragments or ICMP PMTU replies.
    if ($tapMtuOriginal -ne 1280 -or $tapClampMssOriginal -ne 'Enabled') {
        Set-NetIPInterface -InterfaceIndex $tap.ifIndex -AddressFamily IPv4 -NlMtuBytes 1280 -ClampMss Enabled -PolicyStore ActiveStore -ErrorAction Stop
    }
    $tapOverrideRoutes = @(
        @{ Prefix='0.0.0.0'; Mask='128.0.0.0' },
        @{ Prefix='128.0.0.0'; Mask='128.0.0.0' }
    )
    foreach ($override in $tapOverrideRoutes) {
        $prefix = if ($override.Prefix -eq '0.0.0.0') { '0.0.0.0/1' } else { '128.0.0.0/1' }
        $existingOverride = @(Get-NetRoute -InterfaceIndex $tap.ifIndex -DestinationPrefix $prefix -ErrorAction SilentlyContinue)
        if ($existingOverride.Count -and @($existingOverride | Where-Object NextHop -ne '10.254.254.2').Count) {
            throw "The TAP adapter already has a different route for $prefix."
        }
        if (!$existingOverride.Count) {
            $overrideArgs = "ADD $($override.Prefix) MASK $($override.Mask) 10.254.254.2 METRIC 1 IF $($tap.ifIndex)"
            $overrideResult = Invoke-ShareCommand $routeExe $overrideArgs 5000
            if ($overrideResult.ExitCode -ne 0) {
                throw "Cannot route $prefix through the TAP bridge: $($overrideResult.Output) $($overrideResult.Error)"
            }
        }
        $tapOverrideRoutesAdded += $prefix
    }
    # XRay installs more-specific routes (for example 1.1.1.1/32 and several
    # split ranges) on its TUN adapter. Longest-prefix matching beats the TAP
    # /1 routes above, so Windows would send many hotspot flows directly into
    # the XRay TUN instead of the dedicated SOCKS bridge. Mirror every active
    # non-local IPv4 route from that TUN onto TAP with a lower effective metric.
    $vpnForwardingRoutes = @(Get-NetRoute -InterfaceIndex $vpnAdapter.ifIndex -AddressFamily IPv4 -ErrorAction Stop |
        Where-Object { $_.Protocol.ToString() -ne 'Local' -and $_.DestinationPrefix -ne '0.0.0.0/0' })
    foreach ($vpnRoute in $vpnForwardingRoutes) {
        $prefix = [string]$vpnRoute.DestinationPrefix
        $tapRoute = @(Get-NetRoute -InterfaceIndex $tap.ifIndex -DestinationPrefix $prefix -ErrorAction SilentlyContinue)
        if ($tapRoute.Count -gt 0) {
            if (@($tapRoute | Where-Object NextHop -ne '10.254.254.2').Count -gt 0) {
                throw "The TAP adapter already has a conflicting route for $prefix."
            }
            $tapMirroredRoutesAdded += $prefix
            continue
        }
        New-NetRoute -DestinationPrefix $prefix -InterfaceIndex $tap.ifIndex -NextHop '10.254.254.2' -RouteMetric 1 -PolicyStore ActiveStore -ErrorAction Stop | Out-Null
        $tapMirroredRoutesAdded += $prefix
    }
    foreach ($vpnRoute in $vpnForwardingRoutes) {
        $selected = Find-NetRoute -RemoteIPAddress ([string]$vpnRoute.DestinationPrefix.Split('/')[0]) -ErrorAction Stop |
            Where-Object { $_.CimClass.CimClassName -eq 'MSFT_NetRoute' } | Select-Object -First 1
        if (!$selected -or [int]$selected.InterfaceIndex -ne [int]$tap.ifIndex) {
            throw "Windows did not select the TAP bridge for VPN route $($vpnRoute.DestinationPrefix)."
        }
    }
    Report 'starting' 'Selecting the TAP bridge to XRay'
    $seed = Get-SeedManager ([guid]$tap.InterfaceGuid).ToString()
    Report 'starting' 'Starting Windows mobile hotspot'
    $before = @(Get-HotspotAdapters)
    $hotspotStarted = $true
    Enable-Hotspot
    Report 'starting' 'Hotspot started; locating Wi-Fi Direct adapter'
    $privateId = Get-HotspotPrivateId $before
    if (!$privateId) { throw 'Cannot identify the Wi-Fi Direct hotspot adapter.' }
    $privateAdapter = Get-NetAdapter -IncludeHidden | Where-Object {
        ([guid]$_.InterfaceGuid).ToString() -eq ([guid]$privateId).ToString()
    } | Select-Object -First 1
    if (!$privateAdapter) { throw 'Windows cannot resolve the hotspot adapter interface index.' }
    $hotspotIp = Get-NetIPAddress -InterfaceIndex $privateAdapter.ifIndex -AddressFamily IPv4 -ErrorAction SilentlyContinue |
        Where-Object { $_.IPAddress -notmatch '^(169\.254\.|0\.|127\.)' } | Select-Object -First 1
    if (!$hotspotIp) { throw 'Windows did not assign an IPv4 gateway to the Wi-Fi Direct adapter.' }
    $natPrefix = Get-IPv4NetworkPrefix $hotspotIp.IPAddress ([int]$hotspotIp.PrefixLength)
    Report 'hotspot-interface-ready' 'Разрешаю интернет для подключённых устройств через VPN…' @{interfaceIndex=[int]$privateAdapter.ifIndex;tapInterfaceIndex=[int]$tap.ifIndex;clientSubnet=$natPrefix}
    $dhcpReplyDeadline = (Get-Date).AddSeconds(20)
    while (!(Test-Path -LiteralPath $cfg.hotspotFirewallReplyFile) -and (Get-Date) -lt $dhcpReplyDeadline) {
        Assert-ShareNotStopping
        Start-Sleep -Milliseconds 100
    }
    if (!(Test-Path -LiteralPath $cfg.hotspotFirewallReplyFile)) { throw 'Windows firewall did not answer the hotspot DHCP request.' }
    $dhcpPermit = (Get-Content -LiteralPath $cfg.hotspotFirewallReplyFile -Raw).Trim()
    Remove-Item -LiteralPath $cfg.hotspotFirewallReplyFile -Force -ErrorAction SilentlyContinue
    if ($dhcpPermit -eq 'stop') { throw 'Sharing startup was canceled.' }
    if ($dhcpPermit -ne 'allow') {
        $denial = $dhcpPermit | ConvertFrom-Json -ErrorAction SilentlyContinue
        if ($denial.message) { throw $denial.message }
        throw 'The service rejected the hotspot DHCP firewall request.'
    }
    Report 'starting' 'Настраиваю маршрутизацию IPv4 через XRay TUN…'
    $vpnIp = Get-NetIPAddress -InterfaceIndex $vpnAdapter.ifIndex -AddressFamily IPv4 -ErrorAction SilentlyContinue |
        Where-Object { $_.IPAddress -notmatch '^(169\.254\.|0\.|127\.)' } | Select-Object -First 1
    if (!$vpnIp) { throw 'XRay TUN has no usable IPv4 address for the hotspot NAT.' }
    $vpnIpInterface = Get-NetIPInterface -InterfaceIndex $vpnAdapter.ifIndex -AddressFamily IPv4 -ErrorAction Stop
    $hotspotIpInterface = Get-NetIPInterface -InterfaceIndex $privateAdapter.ifIndex -AddressFamily IPv4 -ErrorAction Stop
    $tapIpInterface = Get-NetIPInterface -InterfaceIndex $tap.ifIndex -AddressFamily IPv4 -ErrorAction Stop
    $vpnForwardingOriginal = $vpnIpInterface.Forwarding.ToString()
    $hotspotForwardingOriginal = $hotspotIpInterface.Forwarding.ToString()
    $hotspotClampMssOriginal = $hotspotIpInterface.ClampMss.ToString()
    $tapForwardingOriginal = $tapIpInterface.Forwarding.ToString()
    if ($vpnForwardingOriginal -ne 'Enabled') {
        Set-ShareIPv4Forwarding ([int]$vpnAdapter.ifIndex) 'Enabled'
    }
    if ($hotspotForwardingOriginal -ne 'Enabled') {
        Set-ShareIPv4Forwarding ([int]$privateAdapter.ifIndex) 'Enabled'
    }
    if ([int]$hotspotIpInterface.NlMtu -ne 1500 -or $hotspotClampMssOriginal -ne 'Enabled') {
        # Keep the Wi-Fi Direct link at its native MTU. Reducing this interface
        # prevents some Windows hotspot drivers from delivering client frames.
        # MSS clamping still protects the smaller TAP/XRay path.
        Set-NetIPInterface -InterfaceIndex $privateAdapter.ifIndex -AddressFamily IPv4 -NlMtuBytes 1500 -ClampMss Enabled -PolicyStore ActiveStore -ErrorAction Stop
    }
    if ($tapForwardingOriginal -ne 'Enabled') {
        Set-ShareIPv4Forwarding ([int]$tap.ifIndex) 'Enabled'
    }
    $forwardingCheck = @(Get-NetIPInterface -InterfaceIndex @($vpnAdapter.ifIndex,$privateAdapter.ifIndex,$tap.ifIndex) -AddressFamily IPv4 -ErrorAction Stop)
    $forwardingFailures = @($forwardingCheck | Where-Object { $_.Forwarding.ToString() -ne 'Enabled' })
    if ($forwardingFailures.Count) {
        throw ('Windows did not enable IPv4 forwarding on every VPN-sharing interface: ' +
            (($forwardingFailures | ForEach-Object { '{0} ({1})' -f $_.InterfaceAlias,$_.Forwarding }) -join ', '))
    }
    Write-ShareDiagnostic ('Routing hotspot prefix ' + $natPrefix + ' without source NAT through TAP ' + $tap.ifIndex + ' to XRay TUN ' + $vpnAdapter.ifIndex)
    $lastForwardingRefresh = Get-Date
    Report 'running' ('VPN sharing enabled: ' + $cfg.ssid)
    while ($true) {
        if (Test-Path -LiteralPath $cfg.stopFile) { break }
        if ($controlTask.Wait(1000)) { break }
        if (((Get-Date) - $lastForwardingRefresh).TotalSeconds -ge 5) {
            foreach ($interfaceIndex in @([int]$privateAdapter.ifIndex,[int]$tap.ifIndex,[int]$vpnAdapter.ifIndex)) {
                $interface = Get-NetIPInterface -InterfaceIndex $interfaceIndex -AddressFamily IPv4 -ErrorAction Stop
                if ($interface.Forwarding.ToString() -ne 'Enabled') {
                    Set-ShareIPv4Forwarding ([int]$interfaceIndex) 'Enabled'
                    Write-ShareDiagnostic ('Restored IPv4 forwarding on ' + $interface.InterfaceAlias)
                }
            }
            $lastForwardingRefresh = Get-Date
        }
        if ($bridge.HasExited) { throw 'Packet bridge stopped; shutting down sharing.' }
        if ((Get-HotspotState) -ne 'On') { throw 'Windows stopped the hotspot.' }
    }
} catch {
    $failed = $true
    Write-ShareDiagnostic ('ERROR ' + $_.Exception.ToString())
    Report 'error' $_.Exception.Message
} finally {
    if ($readyFile -and (Test-Path -LiteralPath $readyFile)) { Remove-Item -LiteralPath $readyFile -Force -ErrorAction SilentlyContinue }
    Report 'stopping' 'Останавливаю точку доступа…'
    Reset-ShareControlTask
    if ($hotspotStarted) { try { Disable-Hotspot } catch { Report 'error' ('Hotspot cleanup: ' + $_.Exception.Message); $failed=$true } }
    if ($null -ne $hotspotForwardingOriginal -and $null -ne $privateAdapter) {
        try { Set-ShareIPv4Forwarding ([int]$privateAdapter.ifIndex) $hotspotForwardingOriginal } catch { }
    }
    if ($null -ne $hotspotClampMssOriginal -and $null -ne $privateAdapter) {
        try { Set-NetIPInterface -InterfaceIndex $privateAdapter.ifIndex -AddressFamily IPv4 -ClampMss $hotspotClampMssOriginal -PolicyStore ActiveStore -ErrorAction Stop } catch { }
    }
    if ($null -ne $tapForwardingOriginal -and $null -ne $tap) {
        try { Set-ShareIPv4Forwarding ([int]$tap.ifIndex) $tapForwardingOriginal } catch { }
    }
    if ($null -ne $vpnForwardingOriginal -and $null -ne $vpnAdapter) {
        try { Set-ShareIPv4Forwarding ([int]$vpnAdapter.ifIndex) $vpnForwardingOriginal } catch { }
    }
    foreach ($prefix in $tapOverrideRoutesAdded) {
        $network = if ($prefix -eq '0.0.0.0/1') { '0.0.0.0' } else { '128.0.0.0' }
        try { Invoke-ShareCommand (Join-Path $env:SystemRoot 'System32\route.exe') "DELETE $network MASK 128.0.0.0 10.254.254.2 IF $($tap.ifIndex)" 5000 | Out-Null } catch { }
    }
    foreach ($prefix in $tapMirroredRoutesAdded | Select-Object -Unique) {
        try {
            Get-NetRoute -InterfaceIndex $tap.ifIndex -DestinationPrefix $prefix -ErrorAction SilentlyContinue |
                Where-Object NextHop -eq '10.254.254.2' |
                Remove-NetRoute -Confirm:$false -ErrorAction Stop
        } catch { }
    }
    if ($null -ne $tapAutomaticMetricOriginal -and $null -ne $tapInterfaceMetricOriginal -and $null -ne $tap) {
        try { Set-NetIPInterface -InterfaceIndex $tap.ifIndex -AddressFamily IPv4 -AutomaticMetric $tapAutomaticMetricOriginal -InterfaceMetric $tapInterfaceMetricOriginal -PolicyStore ActiveStore -ErrorAction Stop } catch { }
    }
    if ($null -ne $tapMtuOriginal -and $null -ne $tapClampMssOriginal -and $null -ne $tap) {
        try { Set-NetIPInterface -InterfaceIndex $tap.ifIndex -AddressFamily IPv4 -NlMtuBytes $tapMtuOriginal -ClampMss $tapClampMssOriginal -PolicyStore ActiveStore -ErrorAction Stop } catch { }
    }
    if ($routeAdded) {
        Report 'stopping' 'Удаляю маршрут…'
        try { Invoke-ShareCommand (Join-Path $env:SystemRoot 'System32\route.exe') "DELETE 0.0.0.0 MASK 0.0.0.0 10.254.254.2 IF $($tap.ifIndex)" 5000 | Out-Null } catch { }
    }
    if ($bridge) {
        Report 'stopping' 'Закрываю сетевой мост…'
        if (!$bridge.HasExited) { $bridge.StandardInput.Close(); if (!$bridge.WaitForExit(3000)) { $bridge.Kill() } }
        $bridge.Dispose()
    }
    if ($configChanged) {
        Report 'stopping' 'Восстанавливаю настройки Wi-Fi…'
        try { Set-HotspotConfig $previousConfig.Ssid $previousConfig.Password $previousConfig.Band 15 $true } catch { }
    }
    if ($uplinkDnsChanged -and $uplinkAdapter) {
        try {
            if ($uplinkDnsWasAutomatic) {
                Set-DnsClientServerAddress -InterfaceIndex $uplinkAdapter.ifIndex -ResetServerAddresses -ErrorAction Stop
            } elseif ($uplinkDnsOriginal.Count) {
                Set-DnsClientServerAddress -InterfaceIndex $uplinkAdapter.ifIndex -ServerAddresses $uplinkDnsOriginal -ErrorAction Stop
            }
        } catch { Write-ShareDiagnostic ('Could not restore uplink DNS settings: ' + $_.Exception.Message) }
    }
    foreach ($doh in $dohSnapshots) {
        try {
            Set-DnsClientDohServerAddress -ServerAddress $doh.ServerAddress -DohTemplate $doh.DohTemplate -AllowFallbackToUdp $doh.AllowFallbackToUdp -AutoUpgrade $doh.AutoUpgrade -ErrorAction Stop
        } catch { }
    }
}
if ($failed) { exit 1 }
Report 'stopped' ''

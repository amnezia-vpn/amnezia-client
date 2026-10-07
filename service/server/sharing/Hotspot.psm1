# Windows PowerShell 5.1. WinRT owns hotspot lifecycle and NAT.
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$script:Manager = $null
$script:ControlTask = $null
 $script:StopFile = ''
$script:NoConnectionsTimeoutOriginal = $null
function Write-HotspotDiagnostic([string]$Message) {
    try {
        $path = Join-Path $env:ProgramData 'AmneziaVPN Share\log\share-worker.log'
        Add-Content -LiteralPath $path -Encoding UTF8 -Value ('{0} {1}' -f [DateTime]::Now.ToString('o'), $Message)
    } catch { }
}
function Set-ShareControlTask($Task, [string]$StopFile = '') { $script:ControlTask = $Task; $script:StopFile = $StopFile }
function Reset-ShareControlTask { $script:ControlTask = $null; $script:StopFile = '' }
function Assert-ShareNotStopping {
    if ($script:StopFile -and (Test-Path -LiteralPath $script:StopFile)) {
        throw [OperationCanceledException]::new('Раздача остановлена пользователем.')
    }
    if ($script:ControlTask -and $script:ControlTask.IsCompleted -and
        $script:ControlTask.GetAwaiter().GetResult() -eq 'stop') {
        throw [OperationCanceledException]::new('Раздача отменена пользователем.')
    }
}
function Initialize-WinRT {
    Add-Type -AssemblyName System.Runtime.WindowsRuntime
    $null = [Windows.Networking.Connectivity.NetworkInformation,Windows.Networking.Connectivity,ContentType=WindowsRuntime]
    $null = [Windows.Networking.NetworkOperators.NetworkOperatorTetheringManager,Windows.Networking.NetworkOperators,ContentType=WindowsRuntime]
    $null = [Windows.Networking.NetworkOperators.NetworkOperatorTetheringAccessPointConfiguration,Windows.Networking.NetworkOperators,ContentType=WindowsRuntime]
    $null = [Windows.Networking.NetworkOperators.NetworkOperatorTetheringOperationResult,Windows.Networking.NetworkOperators,ContentType=WindowsRuntime]
}
function Wait-WinRT($Operation, [Type]$ResultType, [int]$TimeoutSeconds = 45, [bool]$IgnoreStop = $false) {
    if ($ResultType) {
        $method = [System.WindowsRuntimeSystemExtensions].GetMethods() | Where-Object {
            $_.Name -eq 'AsTask' -and $_.IsGenericMethod -and $_.GetGenericArguments().Count -eq 1 -and
            $_.GetParameters().Count -eq 1 -and $_.GetParameters()[0].ParameterType.Name -eq 'IAsyncOperation`1'
        } | Select-Object -First 1
        $task = $method.MakeGenericMethod($ResultType).Invoke($null, @($Operation))
    } else {
        $method = [System.WindowsRuntimeSystemExtensions].GetMethods() | Where-Object {
            $_.Name -eq 'AsTask' -and -not $_.IsGenericMethod -and $_.GetParameters().Count -eq 1 -and
            $_.GetParameters()[0].ParameterType.Name -eq 'IAsyncAction'
        } | Select-Object -First 1
        $task = $method.Invoke($null, @($Operation))
    }
    $watch = [Diagnostics.Stopwatch]::StartNew()
    while (-not $task.IsCompleted) {
        if (-not $IgnoreStop) { Assert-ShareNotStopping }
        if ($watch.Elapsed.TotalSeconds -gt $TimeoutSeconds) {
            try { $Operation.Cancel() } catch { }
            throw "Windows did not finish the hotspot operation within $TimeoutSeconds seconds."
        }
        if ('System.Windows.Forms.Application' -as [type]) { [Windows.Forms.Application]::DoEvents() }
        Start-Sleep -Milliseconds 50
    }
    if ($ResultType) { $task.GetAwaiter().GetResult() } else { $null = $task.GetAwaiter().GetResult() }
}
function Get-SharingEntries {
    $com = New-Object -ComObject HNetCfg.HNetShare
    @($com.EnumEveryConnection() | ForEach-Object {
        $props = $com.NetConnectionProps($_)
        $config = $com.INetSharingConfigurationForINetConnection($_)
        $enabled = [bool]$config.SharingEnabled
        $role = -1
        if ($enabled) { $role = [int]$config.SharingConnectionType }
        [pscustomobject]@{ Id = ([guid]$props.Guid).ToString(); Name = $props.Name; Enabled = $enabled; Role = $role; Config = $config }
    })
}
function Get-SeedManager([string]$SeedId = '') {
    Initialize-WinRT
    $profiles = @([Windows.Networking.Connectivity.NetworkInformation]::GetConnectionProfiles())
    if ($SeedId) {
        $profile = $profiles | Where-Object { $_.NetworkAdapter -and $_.NetworkAdapter.NetworkAdapterId.ToString() -eq $SeedId } | Select-Object -First 1
    } else {
        # The global Internet profile can be XRay's Wintun. WinRT attempts ICS
        # on its seed profile, so it must start from a physical connection.
        $physical = @(Get-NetAdapter -Physical | Where-Object Status -eq 'Up')
        # Pick the physical adapter that actually owns the IPv4 default route.
        # A disconnected Wi-Fi adapter can still have a WinRT profile and sort
        # before Ethernet; using that profile makes Windows hand hotspot clients
        # DNS servers from the wrong network.
        $physicalIndexes = @($physical | ForEach-Object { [int]$_.ifIndex })
        $defaultRoute = @(Get-NetRoute -AddressFamily IPv4 -DestinationPrefix '0.0.0.0/0' -ErrorAction SilentlyContinue |
            Where-Object { $_.InterfaceIndex -in $physicalIndexes -and $_.NextHop -ne '0.0.0.0' } |
            Sort-Object RouteMetric | Select-Object -First 1)
        if ($defaultRoute.Count) {
            $preferred = $physical | Where-Object { $_.ifIndex -eq $defaultRoute[0].InterfaceIndex } | Select-Object -First 1
            if ($preferred) {
                $physical = @($preferred) + @($physical | Where-Object { $_.ifIndex -ne $preferred.ifIndex })
            }
        }
        $profile = $null
        foreach ($adapter in $physical) {
            $match = @($profiles | Where-Object { $_.NetworkAdapter -and
                $_.NetworkAdapter.NetworkAdapterId.ToString() -eq ([guid]$adapter.InterfaceGuid).ToString() })
            if ($match.Count) { $profile = $match[0]; break }
        }
    }
    if (-not $profile) { throw 'Windows не нашла профиль подключения. Подключите компьютер к интернету.' }
    $script:Manager = [Windows.Networking.NetworkOperators.NetworkOperatorTetheringManager]::CreateFromConnectionProfile($profile)
    return $profile.NetworkAdapter.NetworkAdapterId.ToString()
}
function Get-HotspotState { $script:Manager.TetheringOperationalState.ToString() }
function Get-HotspotConfig {
    $config = $script:Manager.GetCurrentAccessPointConfiguration()
    [pscustomobject]@{ Ssid = $config.Ssid; Password = $config.Passphrase; Band = [int]$config.Band }
}
function Set-HotspotConfig([string]$Ssid, [string]$Password, [int]$Band = 0, [int]$TimeoutSeconds = 45, [bool]$IgnoreStop = $false) {
    $config = New-Object Windows.Networking.NetworkOperators.NetworkOperatorTetheringAccessPointConfiguration
    $config.Ssid = $Ssid; $config.Passphrase = $Password; $config.Band = $Band
    Wait-WinRT ($script:Manager.ConfigureAccessPointAsync($config)) $null $TimeoutSeconds $IgnoreStop
}
function Enable-Hotspot {
    if ($null -eq $script:NoConnectionsTimeoutOriginal) {
        try {
            $script:NoConnectionsTimeoutOriginal = [Windows.Networking.NetworkOperators.NetworkOperatorTetheringManager]::IsNoConnectionsTimeoutEnabled()
            if ($script:NoConnectionsTimeoutOriginal) {
                [Windows.Networking.NetworkOperators.NetworkOperatorTetheringManager]::DisableNoConnectionsTimeout()
                Write-HotspotDiagnostic 'Disabled Windows hotspot no-client timeout for the lifetime of Tunnel Sharing.'
            }
        } catch {
            Write-HotspotDiagnostic ('Could not disable Windows hotspot no-client timeout: ' + $_.Exception.Message)
            $script:NoConnectionsTimeoutOriginal = $null
        }
    }
    $result = Wait-WinRT ($script:Manager.StartTetheringAsync()) ([Windows.Networking.NetworkOperators.NetworkOperatorTetheringOperationResult])
    if ($result.Status.ToString() -ne 'Success') { throw ('Запуск хот-спота: {0}. {1}' -f $result.Status, $result.AdditionalErrorMessage) }
}
function Disable-Hotspot {
    try {
        if ((Get-HotspotState) -ne 'Off') {
            $result = Wait-WinRT ($script:Manager.StopTetheringAsync()) ([Windows.Networking.NetworkOperators.NetworkOperatorTetheringOperationResult]) 15 $true
            if ($result.Status.ToString() -ne 'Success') { throw ('Остановка хот-спота: {0}. {1}' -f $result.Status, $result.AdditionalErrorMessage) }
        }
    } finally {
        if ($script:NoConnectionsTimeoutOriginal -eq $true) {
            try {
                [Windows.Networking.NetworkOperators.NetworkOperatorTetheringManager]::EnableNoConnectionsTimeout()
                Write-HotspotDiagnostic 'Restored Windows hotspot no-client timeout.'
            } catch {
                Write-HotspotDiagnostic ('Could not restore Windows hotspot no-client timeout: ' + $_.Exception.Message)
            }
        }
        $script:NoConnectionsTimeoutOriginal = $null
    }
}
function Set-ShareIcs([string]$PublicId, [string]$PrivateId) {
    $publicId = ([guid]$PublicId).ToString()
    $privateId = ([guid]$PrivateId).ToString()
    $entries = @(Get-SharingEntries)
    $public = $entries | Where-Object Id -eq $publicId | Select-Object -First 1
    $private = $entries | Where-Object Id -eq $privateId | Select-Object -First 1
    if (!$public -or !$private) { throw 'Windows ICS cannot find the TAP and Wi-Fi Direct connections.' }

    $enabled = @($entries | Where-Object Enabled)
    if ($enabled.Count -gt 2) {
        throw 'More than one Internet sharing pair became active; refusing to change unrelated sharing.'
    }
    $foreignPrivate = @($enabled | Where-Object { $_.Role -eq 1 -and $_.Id -ne $privateId })
    if ($foreignPrivate.Count -or @($enabled | Where-Object { $_.Role -notin @(0,1) }).Count) {
        throw 'Windows has an unrelated private Internet sharing connection active.'
    }
    if (@($enabled | Where-Object Role -eq 0).Count -gt 1) {
        throw 'Windows has multiple public Internet sharing connections active.'
    }
    $alreadyCorrect = $enabled.Count -eq 2 -and
        @($enabled | Where-Object { $_.Id -eq $publicId -and $_.Role -eq 0 }).Count -eq 1 -and
        @($enabled | Where-Object { $_.Id -eq $privateId -and $_.Role -eq 1 }).Count -eq 1
    if ($alreadyCorrect) { return $false }

    # WinRT can choose the machine's physical default route for its upstream
    # even when the manager was created from the TAP profile. The initial
    # sharing snapshot was empty, and the only private role must be this
    # hotspot, so replace WinRT's auto-selected public leg with TAP.
    foreach ($entry in $enabled) { $entry.Config.DisableSharing() }
    # ICS must establish its public leg before the private leg. Enabling the
    # private side first can make SharedAccess raise 0x80040201 while it tries
    # to publish a half-configured sharing pair.
    try {
        $public.Config.EnableSharing(0)
    } catch {
        throw "Windows ICS could not enable TAP as the public connection: $($_.Exception.Message)"
    }
    try {
        $private.Config.EnableSharing(1)
    } catch {
        foreach ($entry in @(Get-SharingEntries | Where-Object { $_.Enabled -and $_.Id -in @($publicId, $privateId) })) {
            try { $entry.Config.DisableSharing() } catch { }
        }
        throw "Windows ICS could not enable Wi-Fi Direct as the private connection: $($_.Exception.Message)"
    }

    $verified = @(Get-SharingEntries | Where-Object Enabled)
    $publicOk = @($verified | Where-Object { $_.Id -eq $publicId -and $_.Role -eq 0 }).Count -eq 1
    $privateOk = @($verified | Where-Object { $_.Id -eq $privateId -and $_.Role -eq 1 }).Count -eq 1
    if ($verified.Count -ne 2 -or !$publicOk -or !$privateOk) {
        foreach ($entry in @($verified | Where-Object { $_.Id -in @($publicId, $privateId) })) {
            try { $entry.Config.DisableSharing() } catch { }
        }
        throw 'Windows did not verify TAP as the hotspot public connection. Sharing stopped to prevent direct Internet access.'
    }
    return $true
}
function Disable-ShareIcs([string]$PublicId, [string]$PrivateId) {
    $ids = @(([guid]$PublicId).ToString(), ([guid]$PrivateId).ToString())
    foreach ($entry in @(Get-SharingEntries | Where-Object { $_.Enabled -and $_.Id -in $ids })) {
        try { $entry.Config.DisableSharing() } catch { }
    }
}
function Get-HotspotAdapters {
    @(Get-NetAdapter -IncludeHidden | Where-Object {
        $_.InterfaceDescription -match 'Wi-Fi Direct' -or $_.PnPDeviceID -match 'VWIFIMP_WFD'
    } | ForEach-Object {
        $ips = @(Get-NetIPAddress -InterfaceIndex $_.ifIndex -AddressFamily IPv4 -ErrorAction SilentlyContinue |
            Where-Object { $_.IPAddress -notmatch '^(169\.254\.|0\.)' } | Select-Object -ExpandProperty IPAddress)
        [pscustomobject]@{
            Id=([guid]$_.InterfaceGuid).ToString(); Name=$_.Name; Description=$_.InterfaceDescription
            Status=$_.Status.ToString(); Addresses=$ips
        }
    })
}
function Get-IPv4NetworkPrefix([string]$Address, [int]$PrefixLength) {
    if ($PrefixLength -lt 1 -or $PrefixLength -gt 32) { throw "Invalid hotspot IPv4 prefix length: $PrefixLength" }
    $bytes = ([Net.IPAddress]::Parse($Address)).GetAddressBytes()
    if ($bytes.Length -ne 4) { throw 'The hotspot adapter must use IPv4.' }
    for ($i = 0; $i -lt 4; $i++) {
        $bits = [Math]::Min(8, [Math]::Max(0, $PrefixLength - ($i * 8)))
        $mask = if ($bits -eq 0) { 0 } else { (0xff -shl (8 - $bits)) -band 0xff }
        $bytes[$i] = [byte](([int]$bytes[$i]) -band $mask)
    }
    return '{0}/{1}' -f ([Net.IPAddress]::new($bytes).ToString()), $PrefixLength
}
function Resolve-HotspotPrivateId($Wifi, $Entries, $Before) {
    # An explicit ICS private role is authoritative, even while NDIS reports
    # Disconnected before a station joins. Never guess between two adapters.
    $private = @($Entries | Where-Object { $_.Enabled -and $_.Role -eq 1 })
    $wifiIds = @($Wifi | ForEach-Object { $_.Id })
    if ($private.Count -eq 1 -and $private[0].Id -in $wifiIds) { return $private[0].Id }
    if ($private.Count -gt 0) { return $null }
    # Some Windows versions expose the miniport before assigning an ICS role.
    # Without full ICS, standalone Mobile Hotspot still creates a private
    # Wi-Fi Direct adapter. Select it only when it is the unique adapter that
    # changed state or gained an address after this worker started the AP.
    $candidates = @($Wifi | Where-Object {
        $current = $_
        $prior = @($Before | Where-Object Id -eq $current.Id)
        $becameUp = $current.Status -eq 'Up' -and ($prior.Count -eq 0 -or $prior[0].Status -ne 'Up')
        $newAddress = @($current.Addresses | Where-Object {
            $prior.Count -eq 0 -or $_ -notin @($prior[0].Addresses)
        }).Count -gt 0
        ($becameUp -or $newAddress)
    })
    if ($candidates.Count -eq 1) { return $candidates[0].Id }
    return $null
}
function Get-HotspotPrivateId($Before) {
    $deadline = [DateTime]::UtcNow.AddSeconds(30)
    $previousId = ''; $stable = 0
    $last = $null
    do {
        Assert-ShareNotStopping
        $entries = @(Get-SharingEntries)
        $wifi = @(Get-HotspotAdapters)
        $hotspotState = Get-HotspotState
        $last = [pscustomobject]@{
            Timestamp=[DateTime]::UtcNow.ToString('o'); Hotspot=$hotspotState
            Before=@($Before); Wifi=$wifi; Sharing=@($entries | Select-Object Id,Name,Enabled,Role)
        }
        $id = $null
        if ($hotspotState -eq 'On') { $id = Resolve-HotspotPrivateId $wifi $entries $Before }
        if ($id -and $id -eq $previousId) { $stable++ } else { $stable = 1 }
        if ($id -and $stable -ge 2) {

            return $id
        }
        $previousId = $id
        Start-Sleep -Milliseconds 300
        if ('System.Windows.Forms.Application' -as [type]) { [Windows.Forms.Application]::DoEvents() }
    } while ([DateTime]::UtcNow -lt $deadline)

    $details = (@($last.Wifi | ForEach-Object { '{0}: {1}' -f $_.Name,$_.Status }) -join '; ')
    throw ('Не удалось выбрать единственный адаптер хот-спота. Состояние хот-спота: {0}. Wi-Fi: {1}.' -f $last.Hotspot,$details)
}
Export-ModuleMember -Function Initialize-WinRT,Wait-WinRT,Set-ShareControlTask,Reset-ShareControlTask,Assert-ShareNotStopping,Get-SharingEntries,Get-SeedManager,Get-HotspotState,Get-HotspotConfig,Set-HotspotConfig,Enable-Hotspot,Disable-Hotspot,Set-ShareIcs,Disable-ShareIcs,Get-HotspotAdapters,Get-IPv4NetworkPrefix,Resolve-HotspotPrivateId,Get-HotspotPrivateId

#Requires -RunAsAdministrator
<#
Prepare Windows to load the development-signed Amnezia split tunnel driver.

This script is intended for test/dev builds only. A production Windows kernel
driver should be signed through Microsoft's driver signing flow.
#>

[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

$certificateBase64 = @"
MIIDGjCCAgKgAwIBAgIQRe/lpSIlLLRLRg6B7+gYEjANBgkqhkiG9w0BAQsFADAlMSMwIQYDVQQDDBpBbW5lemlhIExvY2FsIENvZGUgU2lnbmluZzAeFw0yNjA1MTMxMzI2NTZaFw0zNjA1MTMxMzM2NTRaMCUxIzAhBgNVBAMMGkFtbmV6aWEgTG9jYWwgQ29kZSBTaWduaW5nMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAwbdIawApX6La3UcpD7I3vLqQKyzSZ5+1FpIAT3JDMf7wYkBjPAueux894P1p6QlSM8KwT8V2yHtda2XMAIOh1u2fcUsdWbNh4fIQ1Pkzgx2A5asfPm4x2q9L1quRRO/YcP9NTrrF7pqEE8QoeD0OvfNE5irzAc+rm5UOqwibuNhoNIIKvaP9074yWArGBMahcpvQVAsreH8oWDtiM3T1u1OfB3zuwUhzQhsil8EeooQImGzkNTDUUYZvTJUqkRdJrw0CR6Vb2DxtCZ48Q6GGq+cA4QkU90PXxwv8ZfBvaGUNAHHVVp7xEadVscR5hS4ZoRLlBVjWdV24FapA1ZG/EQIDAQABo0YwRDAOBgNVHQ8BAf8EBAMCB4AwEwYDVR0lBAwwCgYIKwYBBQUHAwMwHQYDVR0OBBYEFHfCLeYinfErgJMnDotOuYXkZsQoMA0GCSqGSIb3DQEBCwUAA4IBAQAGpfz/caRQ0zqEXdnEKPGaES4MJJZBXRefUDF3Ubtu+0FMtHkmgpTNIJHDfjxL35tVDRFkb7bMifhmdML3fjSu1Faz2g7UflRjBu9iuZA2gGwnBW1tOc01ai3VX2F7eAV+CH+KLc9RrQFOIoZL2ixsMzMfQOu7GiI/VjxVk8AQ/pjLSgUTasYi4HsAaogouRvw+z4Ym43K0TlTbCAL5dU6L6s2M7sWQUo9EIh+1P46s0kAASbe5hdrDVkfjhAn3EwgPwJxeQr3HKuxm9izdvhnObgj46zMtdGyVH40pvQfqL4ijubDTcxQKPgSemieoZE7iGl3zm0qoNNRF/JwKOMc
"@

function Write-Step {
    param([Parameter(Mandatory = $true)][string]$Message)
    Write-Host ""
    Write-Host "==> $Message"
}

function Test-TestSigningEnabled {
    $output = & bcdedit /enum "{current}" 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw "bcdedit query failed: $output"
    }

    return ($output -match "(?im)^\s*testsigning\s+Yes\s*$")
}

function Import-CertificateIfMissing {
    param(
        [Parameter(Mandatory = $true)][Security.Cryptography.X509Certificates.X509Certificate2]$Certificate,
        [Parameter(Mandatory = $true)][string]$StoreName
    )

    $store = [Security.Cryptography.X509Certificates.X509Store]::new(
        $StoreName,
        [Security.Cryptography.X509Certificates.StoreLocation]::LocalMachine)
    $store.Open([Security.Cryptography.X509Certificates.OpenFlags]::ReadWrite)
    try {
        $existing = $store.Certificates.Find(
            [Security.Cryptography.X509Certificates.X509FindType]::FindByThumbprint,
            $Certificate.Thumbprint,
            $false)

        if ($existing.Count -eq 0) {
            $store.Add($Certificate)
            Write-Host "Imported certificate to LocalMachine\$StoreName"
        }
        else {
            Write-Host "Certificate already exists in LocalMachine\$StoreName"
        }
    }
    finally {
        $store.Close()
    }
}

$installDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$driverSource = Join-Path $installDir "mullvad-split-tunnel.sys"
$driverTarget = Join-Path $env:windir "System32\drivers\mullvad-split-tunnel.sys"
$serviceName = "AmneziaVPNSplitTunnel"

Write-Step "Importing development code-signing certificate"
$certificateBytes = [Convert]::FromBase64String(($certificateBase64 -replace "\s", ""))
$certificate = [Security.Cryptography.X509Certificates.X509Certificate2]::new($certificateBytes)
Write-Host "Certificate subject: $($certificate.Subject)"
Write-Host "Certificate thumbprint: $($certificate.Thumbprint)"
Import-CertificateIfMissing -Certificate $certificate -StoreName "Root"
Import-CertificateIfMissing -Certificate $certificate -StoreName "TrustedPublisher"

Write-Step "Enabling Windows test-signing mode"
$testSigningWasEnabled = Test-TestSigningEnabled
if ($testSigningWasEnabled) {
    Write-Host "Test-signing mode is already enabled."
}
else {
    $output = & bcdedit /set testsigning on 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Error @"
Failed to enable test-signing mode.

bcdedit output:
$output

If Secure Boot is enabled, Windows may reject test-signing mode. Disable Secure Boot in UEFI/BIOS
or use a production driver signed through Microsoft's driver signing flow.
"@
        exit 1
    }

    Write-Host "Test-signing mode was enabled. A reboot is required before the driver can load."
}

Write-Step "Installing split tunnel driver service"
if (-not (Test-Path -LiteralPath $driverSource -PathType Leaf)) {
    throw "Driver file not found next to this script: $driverSource"
}

& sc.exe stop $serviceName | Out-Null
Copy-Item -LiteralPath $driverSource -Destination $driverTarget -Force
Write-Host "Copied driver to $driverTarget"

& sc.exe query $serviceName 2>&1 | Out-Null
if ($LASTEXITCODE -eq 0) {
    & sc.exe config $serviceName binPath= "system32\drivers\mullvad-split-tunnel.sys" start= demand | Out-Host
}
else {
    & sc.exe create $serviceName type= kernel start= demand binPath= "system32\drivers\mullvad-split-tunnel.sys" DisplayName= "Amnezia Split Tunnel Service" | Out-Host
}

if ($testSigningWasEnabled) {
    Write-Step "Starting split tunnel driver service"
    & sc.exe start $serviceName | Out-Host
}
else {
    Write-Host ""
    Write-Host "Reboot Windows, then start AmneziaVPN. The driver should load after reboot."
}

Write-Host ""
Write-Host "Done."

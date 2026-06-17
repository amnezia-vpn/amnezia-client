[CmdletBinding()]
param(
    [string] $QtVersion = $(if ($env:QT_VERSION) { $env:QT_VERSION } else { "6.10.1" }),
    [string] $QtInstallDir = $(if ($env:QT_INSTALL_DIR) { $env:QT_INSTALL_DIR } else { "C:\Qt" }),
    [string] $QtMirrorBase = $(if ($env:QT_MIRROR_BASE) { $env:QT_MIRROR_BASE } else { "https://mirrors.20i.com/pub/qt.io" }),
    [string] $AndroidHome = $(if ($env:ANDROID_HOME) { $env:ANDROID_HOME } else { "$env:USERPROFILE\Android\Sdk" }),
    [string] $WslAndroidHome = $(if ($env:WSL_ANDROID_HOME) { $env:WSL_ANDROID_HOME } else { "" }),
    [string] $WslJdkUrl = $(if ($env:WSL_JDK_URL) { $env:WSL_JDK_URL } else { "https://aka.ms/download-jdk/microsoft-jdk-17-linux-x64.tar.gz" }),
    [string] $AndroidCmdlineToolsUrl = $(if ($env:ANDROID_CMDLINE_TOOLS_URL) { $env:ANDROID_CMDLINE_TOOLS_URL } else { "https://dl.google.com/android/repository/commandlinetools-linux-14742923_latest.zip" }),
    [string] $AndroidPlatform = $(if ($env:ANDROID_PLATFORM_VERSION) { $env:ANDROID_PLATFORM_VERSION } else { "android-36" }),
    [string] $AndroidBuildToolsVersion = $(if ($env:ANDROID_BUILD_TOOLS_VERSION) { $env:ANDROID_BUILD_TOOLS_VERSION } else { "36.0.0" }),
    [string] $AndroidNdkVersion = $(if ($env:ANDROID_NDK_VERSION) { $env:ANDROID_NDK_VERSION } else { "26.1.10909125" }),
    [string] $QtAndroidModules = $(if ($env:QT_ANDROID_MODULES) { $env:QT_ANDROID_MODULES } else { "qtremoteobjects qt5compat" }),
    [string] $KeyDir = $(if ($env:SELFHOSTED_UPDATE_KEY_DIR) { $env:SELFHOSTED_UPDATE_KEY_DIR } else { "C:\keys" }),
    [string] $AndroidReleaseKeystorePath = $(if ($env:QT_ANDROID_KEYSTORE_PATH) { $env:QT_ANDROID_KEYSTORE_PATH } else { "" }),
    [string] $AndroidReleaseKeystoreAlias = $(if ($env:QT_ANDROID_KEYSTORE_ALIAS) { $env:QT_ANDROID_KEYSTORE_ALIAS } else { "release" }),
    [string] $UpdateSyncHost = $(if ($env:SELFHOSTED_UPDATE_SYNC_HOST) { $env:SELFHOSTED_UPDATE_SYNC_HOST } else { "10.8.1.0" }),
    [string] $BaseUrl = $(if ($env:SELFHOSTED_UPDATE_BASE_URL) { $env:SELFHOSTED_UPDATE_BASE_URL } else { "" }),
    [string] $AndroidReleaseKeystoreEnvFile = "",
    [string] $EnvFile = "",
    [switch] $InstallMissing,
    [switch] $GenerateUpdateKeys,
    [switch] $GenerateAndroidKeystore,
    [switch] $RunPreflight
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$ScriptRoot = Split-Path -Parent $PSCommandPath
$RepoRoot = (Resolve-Path (Join-Path $ScriptRoot "..\..")).Path
$PrivateKeyPath = Join-Path $KeyDir "selfhosted-update-private.pem"
$PublicKeyPath = Join-Path $KeyDir "selfhosted-update-public.pem"

if ([string]::IsNullOrWhiteSpace($AndroidReleaseKeystorePath)) {
    $AndroidReleaseKeystorePath = Join-Path $KeyDir "android-release.keystore"
}
if ([string]::IsNullOrWhiteSpace($BaseUrl)) {
    $BaseUrl = "http://${UpdateSyncHost}:17865"
}

if ([string]::IsNullOrWhiteSpace($AndroidReleaseKeystoreEnvFile)) {
    $AndroidReleaseKeystoreEnvFile = Join-Path $KeyDir "android-release-keystore.env.ps1"
}

if ([string]::IsNullOrWhiteSpace($EnvFile)) {
    $EnvFile = Join-Path $RepoRoot "dist\selfhosted-release-env.ps1"
}

function Write-Step([string] $Message) {
    Write-Host ""
    Write-Host "==> $Message"
}

function Assert-Command([string] $CommandName) {
    if (-not (Get-Command $CommandName -ErrorAction SilentlyContinue)) {
        throw "Required command is not available in PATH: $CommandName"
    }
}

function Get-OpenSslCommand {
    $candidates = @(
        $env:OPENSSL,
        (Get-Command "openssl" -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source -First 1),
        "C:\Program Files\Git\usr\bin\openssl.exe",
        "C:\Program Files\Git\mingw64\bin\openssl.exe"
    )
    foreach ($candidate in $candidates) {
        if (-not [string]::IsNullOrWhiteSpace($candidate) -and (Test-Path -LiteralPath $candidate -PathType Leaf)) {
            return $candidate
        }
    }
    throw "OpenSSL is required to generate self-hosted update keys"
}

function Convert-ToWslPath([string] $Path) {
    Assert-Command "wsl.exe"
    $resolved = (Resolve-Path -LiteralPath $Path).Path
    $converted = & wsl.exe wslpath -a $resolved.Replace("\", "/")
    if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($converted)) {
        throw "Failed to convert path to WSL path: $Path"
    }
    return $converted.Trim()
}

function Quote-Sh([string] $Value) {
    return "'" + $Value.Replace("'", "'\''") + "'"
}

function Quote-PsSingle([string] $Value) {
    return "'" + $Value.Replace("'", "''") + "'"
}

function New-Base64UrlSecret([int] $ByteCount = 32) {
    $bytes = [byte[]]::new($ByteCount)
    $rng = [Security.Cryptography.RandomNumberGenerator]::Create()
    try {
        $rng.GetBytes($bytes)
    } finally {
        $rng.Dispose()
    }
    return [Convert]::ToBase64String($bytes).TrimEnd("=").Replace("+", "-").Replace("/", "_")
}

function Protect-SecretPath([string] $Path) {
    if ([string]::IsNullOrWhiteSpace($Path) -or -not (Test-Path -LiteralPath $Path)) {
        return
    }
    $sid = [Security.Principal.WindowsIdentity]::GetCurrent().User.Value
    $aclArgs = @(
        $Path,
        "/inheritance:r",
        "/grant:r",
        "*${sid}:F",
        "*S-1-5-32-544:F",
        "*S-1-5-18:F"
    )
    & icacls @aclArgs | Out-Null
}

function Test-WslCommand([string] $CommandName) {
    $bashScript = @(
        'export PATH="$HOME/.local/jdk-17/bin:$HOME/.local/bin:$PATH"',
        ('command -v ' + (Quote-Sh $CommandName) + ' >/dev/null')
    ) -join "`n"
    return (Invoke-WslScriptStatus $bashScript) -eq 0
}

function Test-WindowsJavaHome {
    if ([string]::IsNullOrWhiteSpace($env:JAVA_HOME)) {
        return $false
    }
    return (Test-Path -LiteralPath (Join-Path $env:JAVA_HOME "bin\java.exe") -PathType Leaf)
}

function Resolve-WslAndroidHome {
    if (-not [string]::IsNullOrWhiteSpace($WslAndroidHome)) {
        $script = 'cd ' + (Quote-Sh $WslAndroidHome) + ' 2>/dev/null && pwd || printf %s ' + (Quote-Sh $WslAndroidHome)
        $resolved = & wsl.exe bash -lc $script
        if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($resolved)) {
            throw "Failed to resolve WSL_ANDROID_HOME: $WslAndroidHome"
        }
        return $resolved.Trim()
    }
    $wslHome = & wsl.exe bash -lc 'printf %s "$HOME"'
    if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($wslHome)) {
        throw "Failed to resolve WSL home directory for Android SDK"
    }
    return ($wslHome.TrimEnd("/") + "/Android/sdk")
}

function Invoke-Wsl([string] $Script) {
    Write-Host "+ wsl.exe bash -lc $Script"
    & wsl.exe bash -lc "set -euo pipefail; $Script"
    if ($LASTEXITCODE -ne 0) {
        throw "WSL command failed with exit code ${LASTEXITCODE}"
    }
}

function Invoke-WslScriptStatus([string] $Script) {
    $tempScript = [System.IO.Path]::ChangeExtension([System.IO.Path]::GetTempFileName(), ".sh")
    [System.IO.File]::WriteAllText($tempScript, ("set -euo pipefail`n" + $Script), [System.Text.UTF8Encoding]::new($false))
    try {
        $tempScriptWsl = Convert-ToWslPath $tempScript
        & wsl.exe bash $tempScriptWsl
        return $LASTEXITCODE
    } finally {
        Remove-Item -LiteralPath $tempScript -Force -ErrorAction SilentlyContinue
    }
}

function Invoke-WslScriptOutput([string] $Script) {
    $tempScript = [System.IO.Path]::ChangeExtension([System.IO.Path]::GetTempFileName(), ".sh")
    [System.IO.File]::WriteAllText($tempScript, ("set -euo pipefail`n" + $Script), [System.Text.UTF8Encoding]::new($false))
    try {
        $tempScriptWsl = Convert-ToWslPath $tempScript
        $output = & wsl.exe bash $tempScriptWsl
        if ($LASTEXITCODE -ne 0) {
            return ""
        }
        return (($output | Out-String).Trim())
    } finally {
        Remove-Item -LiteralPath $tempScript -Force -ErrorAction SilentlyContinue
    }
}

function Test-QtKit([string] $KitName) {
    return Test-Path -LiteralPath (Join-Path $QtInstallDir "$QtVersion\$KitName\lib\cmake\Qt6\qt.toolchain.cmake") -PathType Leaf
}

function Test-QtKitModule([string] $KitName, [string] $ModuleName) {
    return Test-Path -LiteralPath (Join-Path $QtInstallDir "$QtVersion\$KitName\lib\cmake\$ModuleName\${ModuleName}Config.cmake") -PathType Leaf
}

function Test-AndroidQtRequiredModules([string] $KitName) {
    if ($QtAndroidModules -match "(^|[\s,])qtremoteobjects($|[\s,])" -and -not (Test-QtKitModule $KitName "Qt6RemoteObjects")) {
        return $false
    }
    if ($QtAndroidModules -match "(^|[\s,])qt5compat($|[\s,])" -and -not (Test-QtKitModule $KitName "Qt6Core5Compat")) {
        return $false
    }
    return $true
}

function Test-LinuxQtRequiredModules([string] $KitName) {
    if ($KitName -ne "gcc_64") {
        return $true
    }
    if ($QtAndroidModules -match "(^|[\s,])qtremoteobjects($|[\s,])" -and
        (-not (Test-QtKitModule $KitName "Qt6RemoteObjects") -or -not (Test-QtKitModule $KitName "Qt6RemoteObjectsTools"))) {
        return $false
    }
    if ($QtAndroidModules -match "(^|[\s,])qt5compat($|[\s,])" -and -not (Test-QtKitModule $KitName "Qt6Core5Compat")) {
        return $false
    }
    return $true
}

function Test-AndroidQtKit {
    if (Test-QtKit "android") {
        return Test-AndroidQtRequiredModules "android"
    }
    if (-not (Test-QtKit "android_arm64_v8a") -or -not (Test-AndroidQtRequiredModules "android_arm64_v8a")) {
        return $false
    }
    return $true
}

function Resolve-AndroidShaderToolsLib {
    $candidates = @(
        (Join-Path $QtInstallDir "$QtVersion\android_arm64_v8a\lib\libQt6ShaderTools_arm64-v8a.so"),
        (Join-Path $env:USERPROFILE "Qt\$QtVersion\android_arm64_v8a\lib\libQt6ShaderTools_arm64-v8a.so")
    )
    foreach ($candidate in $candidates) {
        if (-not [string]::IsNullOrWhiteSpace($candidate) -and (Test-Path -LiteralPath $candidate -PathType Leaf)) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }
    return ""
}

function Ensure-WslJava {
    Write-Step "Check Java inside WSL"
    if (Test-WslCommand "java") {
        Write-Host "WSL Java: present"
        return
    }
    if (-not $InstallMissing) {
        Write-Host "WSL Java: missing. Re-run with -InstallMissing to install a user-local JDK under ~/.local/jdk-17."
        return
    }
    $installScript = @(
        'command -v curl >/dev/null',
        'command -v tar >/dev/null',
        ('url=' + (Quote-Sh $WslJdkUrl)),
        'tmp=$(mktemp -d)',
        'trap ''rm -rf "$tmp"'' EXIT',
        'mkdir -p "$HOME/.local"',
        'curl -fsSL "$url" -o "$tmp/jdk.tar.gz"',
        'rm -rf "$HOME/.local/jdk-17"',
        'mkdir -p "$HOME/.local/jdk-17"',
        'tar -xzf "$tmp/jdk.tar.gz" -C "$HOME/.local/jdk-17" --strip-components=1',
        'test -x "$HOME/.local/jdk-17/bin/java"',
        '"$HOME/.local/jdk-17/bin/java" -version'
    ) -join "`n"
    if ((Invoke-WslScriptStatus $installScript) -ne 0) {
        throw "Failed to install user-local JDK inside WSL from $WslJdkUrl"
    }
    Write-Host "WSL Java: installed at ~/.local/jdk-17"
}

function Ensure-AqtInstall {
    Write-Step "Check aqtinstall inside WSL"
    if (Test-WslCommand "python3") {
        & wsl.exe bash -lc "python3 -m pip show aqtinstall >/dev/null 2>&1"
        if ($LASTEXITCODE -eq 0) {
            Write-Host "aqtinstall: present"
            return
        }
    }
    if (-not $InstallMissing) {
        Write-Host "aqtinstall: missing. Re-run with -InstallMissing to install it in WSL."
        return
    }
    Invoke-Wsl "python3 -m pip install --user aqtinstall"
}

function Ensure-Conan {
    Write-Step "Check Conan inside WSL"
    if (Test-WslCommand "conan") {
        Write-Host "Conan: present"
        return
    }
    if (-not $InstallMissing) {
        Write-Host "Conan: missing. Re-run with -InstallMissing to install it in WSL."
        return
    }
    Invoke-Wsl "python3 -m pip install --user conan"
}

function Get-AqtArch([string] $KitName) {
    if ($KitName -eq "gcc_64") {
        return "linux_gcc_64"
    }
    return $KitName
}

function Test-AqtQtVersionAvailable([string] $QtHost, [string] $Target) {
    & wsl.exe bash -lc "python3 -m aqt list-qt $(Quote-Sh $QtHost) $(Quote-Sh $Target) | tr ' ' '\n' | grep -Fx -- $(Quote-Sh $QtVersion) >/dev/null"
    return $LASTEXITCODE -eq 0
}

function Test-AqtQtArchAvailable([string] $QtHost, [string] $Target, [string] $Arch) {
    & wsl.exe bash -lc "python3 -m aqt list-qt $(Quote-Sh $QtHost) $(Quote-Sh $Target) --arch $(Quote-Sh $QtVersion) 2>/dev/null | tr ' ' '\n' | grep -Fx -- $(Quote-Sh $Arch) >/dev/null"
    return $LASTEXITCODE -eq 0
}

function Ensure-QtKit([string] $QtHost, [string] $Target, [string] $KitName) {
    Write-Step "Check Qt kit $KitName"
    if ((Test-QtKit $KitName) -and (Test-LinuxQtRequiredModules $KitName)) {
        Write-Host "Qt kit ${KitName}: present"
        return
    }
    if (-not $InstallMissing) {
        Write-Host "Qt kit ${KitName}: missing under $QtInstallDir\$QtVersion. Re-run with -InstallMissing to install it with aqtinstall."
        return
    }
    $aqtArch = Get-AqtArch $KitName
    if (-not (Test-AqtQtVersionAvailable $QtHost $Target) -or -not (Test-AqtQtArchAvailable $QtHost $Target $aqtArch)) {
        throw "aqtinstall cannot install Qt $QtVersion target '$Target' arch '$aqtArch' for host '$QtHost'. Install the kit manually with Qt MaintenanceTool or choose a Qt version available from aqt."
    }
    New-Item -ItemType Directory -Force -Path $QtInstallDir | Out-Null
    $qtInstallWsl = Convert-ToWslPath $QtInstallDir
    $mirrorArgs = ""
    if (-not [string]::IsNullOrWhiteSpace($QtMirrorBase)) {
        $mirrorArgs = " -b $(Quote-Sh $QtMirrorBase)"
    }
    $moduleArgs = ""
    if (($Target -eq "android" -or $KitName -eq "gcc_64") -and -not [string]::IsNullOrWhiteSpace($QtAndroidModules)) {
        $modules = $QtAndroidModules -split "[,;\s]+" | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
        foreach ($module in $modules) {
            if ($module -notmatch "^[A-Za-z0-9_.+-]+$") {
                throw "Invalid Qt module name: $module"
            }
        }
        if ($modules.Count -gt 0) {
            $moduleArgs = " -m " + (($modules | ForEach-Object { Quote-Sh $_ }) -join " ")
        }
    }
    Invoke-Wsl "python3 -m aqt install-qt $(Quote-Sh $QtHost) $(Quote-Sh $Target) $(Quote-Sh $QtVersion) $(Quote-Sh $aqtArch) -O $(Quote-Sh $qtInstallWsl) --timeout 30$moduleArgs$mirrorArgs"
}

function Resolve-WslQtInstallRoot {
    $script = @'
for base in "$HOME/Qt" "$HOME/.local/Qt" "/opt/Qt"; do
    if [ -d "$base" ]; then
        printf %s "$base"
        exit 0
    fi
done
printf %s "$HOME/Qt"
'@
    $result = Invoke-WslScriptOutput $script
    if ([string]::IsNullOrWhiteSpace($result)) {
        throw "Failed to resolve WSL Qt install root"
    }
    return $result.Trim()
}

function Resolve-WslQifRoot {
    $script = @'
for base in "$HOME/Qt" "$HOME/.local/Qt" "/opt/Qt"; do
    [ -d "$base" ] || continue
    match=$(find "$base" -maxdepth 6 -type f -path "*/bin/binarycreator" -print -quit 2>/dev/null || true)
    if [ -n "$match" ]; then
        dirname "$(dirname "$match")"
        exit 0
    fi
done
exit 1
'@
    $result = Invoke-WslScriptOutput $script
    if ([string]::IsNullOrWhiteSpace($result)) {
        return ""
    }
    return $result.Trim()
}

function Ensure-WslInstallerFramework {
    Write-Step "Check Linux Qt Installer Framework inside WSL"
    $qifRoot = Resolve-WslQifRoot
    if (-not [string]::IsNullOrWhiteSpace($qifRoot)) {
        Write-Host "WSL Qt Installer Framework: present at $qifRoot"
        return
    }
    if (-not $InstallMissing) {
        Write-Host "WSL Qt Installer Framework: missing. Re-run with -InstallMissing to install qt.tools.ifw.47 for Linux .run builds."
        return
    }
    $wslQtRoot = Resolve-WslQtInstallRoot
    $mirrorArgs = ""
    if (-not [string]::IsNullOrWhiteSpace($QtMirrorBase)) {
        $mirrorArgs = " -b $(Quote-Sh $QtMirrorBase)"
    }
    Invoke-Wsl "python3 -m aqt install-tool linux desktop tools_ifw qt.tools.ifw.47 -O $(Quote-Sh $wslQtRoot) --timeout 30$mirrorArgs"
    $qifRoot = Resolve-WslQifRoot
    if ([string]::IsNullOrWhiteSpace($qifRoot)) {
        throw "Failed to install Linux Qt Installer Framework inside WSL"
    }
    Write-Host "WSL Qt Installer Framework: installed at $qifRoot"
}

function Ensure-AndroidQtKits {
    Write-Step "Check Qt Android kits"
    if (Test-AndroidQtKit) {
        Write-Host "Qt Android kit: present"
        return
    }
    if (-not $InstallMissing) {
        Write-Host "Qt Android arm64-v8a kit: missing under $QtInstallDir\$QtVersion. Re-run with -InstallMissing to install it with aqtinstall."
        return
    }
    Ensure-QtKit "all_os" "android" "android_arm64_v8a"
}

function Ensure-AndroidSdkReadable {
    Write-Step "Check Android SDK"
    if (-not (Test-Path -LiteralPath $AndroidHome -PathType Container)) {
        Write-Host "Android SDK: missing at $AndroidHome"
        return
    }
    $ndk = @(Get-ChildItem -LiteralPath (Join-Path $AndroidHome "ndk") -Directory -ErrorAction SilentlyContinue | Select-Object -First 1)
    $apksigner = @(Get-ChildItem -LiteralPath (Join-Path $AndroidHome "build-tools") -Recurse -File -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -in @("apksigner", "apksigner.bat") } |
        Select-Object -First 1)
    Write-Host ("Android SDK: {0}" -f $AndroidHome)
    Write-Host ("Android NDK: {0}" -f ($(if ($ndk.Count -gt 0) { "present" } else { "missing" })))
    Write-Host ("Android apksigner: {0}" -f ($(if ($apksigner.Count -gt 0) { "present" } else { "missing" })))
}

function Test-WslAndroidSdk {
    $sdk = Resolve-WslAndroidHome
    $script = @(
        ('test -d ' + (Quote-Sh $sdk)),
        ('test -n "$(find ' + (Quote-Sh ($sdk + "/build-tools")) + ' -type f -name apksigner -print -quit)"'),
        ('test -n "$(find ' + (Quote-Sh ($sdk + "/ndk")) + ' -path "*/toolchains/llvm/prebuilt/linux-x86_64/bin/clang" -executable -print -quit)"'),
        ('test -n "$(find ' + (Quote-Sh ($sdk + "/ndk")) + ' -path "*/toolchains/llvm/prebuilt/linux-x86_64/bin/clang++" -executable -print -quit)"')
    ) -join "`n"
    return (Invoke-WslScriptStatus $script) -eq 0
}

function Ensure-WslAndroidSdk {
    Write-Step "Check Linux Android SDK inside WSL"
    $sdk = Resolve-WslAndroidHome
    if (Test-WslAndroidSdk) {
        Write-Host "WSL Android SDK: present at $sdk"
        return
    }
    if (-not $InstallMissing) {
        Write-Host "WSL Android SDK: missing or Windows-only at $sdk. Re-run with -InstallMissing to install Linux command-line tools, build-tools, platform, and NDK."
        return
    }
    $installScript = @(
        ('sdk=' + (Quote-Sh $sdk)),
        ('url=' + (Quote-Sh $AndroidCmdlineToolsUrl)),
        ('platform=' + (Quote-Sh $AndroidPlatform)),
        ('build_tools=' + (Quote-Sh $AndroidBuildToolsVersion)),
        ('ndk_version=' + (Quote-Sh $AndroidNdkVersion)),
        'command -v curl >/dev/null',
        'command -v unzip >/dev/null',
        'mkdir -p "$sdk/cmdline-tools"',
        'tmp=$(mktemp -d)',
        'trap ''rm -rf "$tmp"'' EXIT',
        'curl -fsSL "$url" -o "$tmp/cmdline-tools.zip"',
        'unzip -q "$tmp/cmdline-tools.zip" -d "$tmp"',
        'rm -rf "$sdk/cmdline-tools/latest"',
        'mkdir -p "$sdk/cmdline-tools/latest"',
        'cp -R "$tmp/cmdline-tools/"* "$sdk/cmdline-tools/latest/"',
        'yes | "$sdk/cmdline-tools/latest/bin/sdkmanager" --sdk_root="$sdk" --licenses >/dev/null || true',
        'yes | "$sdk/cmdline-tools/latest/bin/sdkmanager" --sdk_root="$sdk" "platform-tools" "platforms;$platform" "build-tools;$build_tools" "ndk;$ndk_version"',
        'yes | "$sdk/cmdline-tools/latest/bin/sdkmanager" --sdk_root="$sdk" --licenses >/dev/null || true'
    ) -join "`n"
    if ((Invoke-WslScriptStatus $installScript) -ne 0) {
        throw "Failed to install Linux Android SDK inside WSL at $sdk"
    }
    if (-not (Test-WslAndroidSdk)) {
        throw "Failed to prepare Linux Android SDK inside WSL at $sdk"
    }
    Write-Host "WSL Android SDK: installed at $sdk"
}

function Ensure-UpdateKeys {
    Write-Step "Check self-hosted update signing keys"
    New-Item -ItemType Directory -Force -Path $KeyDir | Out-Null
    $hasPrivate = Test-Path -LiteralPath $PrivateKeyPath -PathType Leaf
    $hasPublic = Test-Path -LiteralPath $PublicKeyPath -PathType Leaf
    if ($hasPrivate -and $hasPublic) {
        Write-Host "Update signing keys: present"
        return
    }
    if (-not $GenerateUpdateKeys) {
        Write-Host "Update signing keys: missing. Re-run with -GenerateUpdateKeys to create Ed25519 keys under $KeyDir."
        return
    }
    if ($hasPrivate -xor $hasPublic) {
        throw "Only one update key file exists. Refusing to overwrite partial keypair under $KeyDir"
    }
    $openssl = Get-OpenSslCommand
    & $openssl genpkey -algorithm Ed25519 -out $PrivateKeyPath | Out-Null
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to generate update private key"
    }
    & $openssl pkey -in $PrivateKeyPath -pubout -out $PublicKeyPath | Out-Null
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to generate update public key"
    }
    Write-Host "Generated update signing keys under $KeyDir"
}

function Ensure-AndroidReleaseKeystore {
    Write-Step "Check Android release keystore"
    New-Item -ItemType Directory -Force -Path $KeyDir | Out-Null
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $AndroidReleaseKeystorePath) | Out-Null
    $hasKeystore = Test-Path -LiteralPath $AndroidReleaseKeystorePath -PathType Leaf
    $hasEnvFile = Test-Path -LiteralPath $AndroidReleaseKeystoreEnvFile -PathType Leaf

    if ($hasKeystore -and $hasEnvFile) {
        Write-Host "Android release keystore: present"
        return
    }

    if (-not $GenerateAndroidKeystore) {
        Write-Host "Android release keystore: missing. Re-run with -GenerateAndroidKeystore to create it under $KeyDir."
        return
    }

    if ($hasKeystore -or $hasEnvFile) {
        throw "Partial Android keystore state exists. Refusing to overwrite $AndroidReleaseKeystorePath or $AndroidReleaseKeystoreEnvFile"
    }

    $storePass = New-Base64UrlSecret
    $keyPass = $storePass
    Protect-SecretPath $KeyDir
    $keystoreDirWsl = Convert-ToWslPath (Split-Path -Parent $AndroidReleaseKeystorePath)
    $keystoreWsl = $keystoreDirWsl.TrimEnd("/") + "/" + [IO.Path]::GetFileName($AndroidReleaseKeystorePath)
    $distinguishedName = "CN=AmneziaVPN Self-Hosted Release, OU=Release, O=AmneziaVPN, L=Local, ST=Local, C=US"
    $generateCommand = "keytool -genkeypair -v" +
        " -keystore $(Quote-Sh $keystoreWsl)" +
        " -storepass:env AMNEZIA_ANDROID_STORE_PASS" +
        " -keypass:env AMNEZIA_ANDROID_KEY_PASS" +
        " -alias $(Quote-Sh $AndroidReleaseKeystoreAlias)" +
        " -keyalg RSA -keysize 4096 -validity 10000" +
        " -dname $(Quote-Sh $distinguishedName)"
    $verifyCommand = "keytool -list" +
        " -keystore $(Quote-Sh $keystoreWsl)" +
        " -storepass:env AMNEZIA_ANDROID_STORE_PASS" +
        " -alias $(Quote-Sh $AndroidReleaseKeystoreAlias) >/dev/null"

    $generateScript = @(
        'export PATH="$HOME/.local/jdk-17/bin:$HOME/.local/bin:$PATH"',
        "export AMNEZIA_ANDROID_STORE_PASS=$(Quote-Sh $storePass)",
        "export AMNEZIA_ANDROID_KEY_PASS=$(Quote-Sh $keyPass)",
        'command -v keytool >/dev/null',
        $generateCommand,
        $verifyCommand
    ) -join "`n"

    if ((Invoke-WslScriptStatus $generateScript) -ne 0) {
        throw "Failed to generate Android release keystore"
    }

    $envContent = @(
        "`$env:QT_ANDROID_KEYSTORE_PATH = $(Quote-PsSingle $AndroidReleaseKeystorePath)",
        "`$env:QT_ANDROID_KEYSTORE_ALIAS = $(Quote-PsSingle $AndroidReleaseKeystoreAlias)",
        "`$env:QT_ANDROID_KEYSTORE_STORE_PASS = $(Quote-PsSingle $storePass)",
        "`$env:QT_ANDROID_KEYSTORE_KEY_PASS = $(Quote-PsSingle $keyPass)"
    ) -join [Environment]::NewLine
    Set-Content -LiteralPath $AndroidReleaseKeystoreEnvFile -Value $envContent -Encoding UTF8
    Protect-SecretPath $AndroidReleaseKeystorePath
    Protect-SecretPath $AndroidReleaseKeystoreEnvFile
    Write-Host "Generated Android release keystore and secret env file under $KeyDir"
}

function Write-EnvironmentFile {
    Write-Step "Write release environment file"
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $EnvFile) | Out-Null
    $resolvedWslAndroidHome = Resolve-WslAndroidHome
    $publicKeyBase64 = ""
    if (Test-Path -LiteralPath $PublicKeyPath -PathType Leaf) {
        $publicKeyBase64 = [Convert]::ToBase64String([IO.File]::ReadAllBytes($PublicKeyPath))
    }
    $content = @(
        "`$env:QT_INSTALL_DIR = $(Quote-PsSingle $QtInstallDir)",
        "`$env:QT_ROOT_PATH = $(Quote-PsSingle (Join-Path $QtInstallDir $QtVersion))",
        "`$env:QT_ANDROID_SHADERTOOLS_LIB = $(Quote-PsSingle (Resolve-AndroidShaderToolsLib))",
        "`$env:QIF_ROOT_PATH = $(Quote-PsSingle (Join-Path $QtInstallDir 'Tools\QtInstallerFramework\4.7'))",
        "`$env:WSL_QIF_ROOT_PATH = $(Quote-PsSingle (Resolve-WslQifRoot))",
        "`$env:ANDROID_HOME = $(Quote-PsSingle $AndroidHome)",
        "`$env:WSL_ANDROID_HOME = $(Quote-PsSingle $resolvedWslAndroidHome)",
        "`$env:SELFHOSTED_UPDATE_BASE_URL = $(Quote-PsSingle $BaseUrl)",
        "`$env:SELFHOSTED_UPDATE_SYNC_HOST = $(Quote-PsSingle $UpdateSyncHost)",
        "`$env:SELFHOSTED_UPDATE_PRIVATE_KEY_PATH = $(Quote-PsSingle $PrivateKeyPath)",
        "`$env:SELFHOSTED_UPDATE_PUBLIC_KEY_PEM_BASE64 = $(Quote-PsSingle $publicKeyBase64)"
    )
    if (Test-Path -LiteralPath $AndroidReleaseKeystoreEnvFile -PathType Leaf) {
        $content += ". $(Quote-PsSingle $AndroidReleaseKeystoreEnvFile)"
    } else {
        $content += @(
            "# Android APKs must keep using this workstation release key.",
            "# Re-run setup with -GenerateAndroidKeystore to create it under $KeyDir.",
            "# `$env:QT_ANDROID_KEYSTORE_PATH = 'C:\keys\android-release.keystore'",
            "# `$env:QT_ANDROID_KEYSTORE_ALIAS = 'release'",
            "# `$env:QT_ANDROID_KEYSTORE_STORE_PASS = '<password>'"
        )
    }
    $content = $content -join [Environment]::NewLine
    Set-Content -LiteralPath $EnvFile -Value $content -Encoding UTF8
    Write-Host "Wrote $EnvFile"
}

Assert-Command "wsl.exe"
Ensure-WslJava
Ensure-AqtInstall
Ensure-Conan
Ensure-WslInstallerFramework
Ensure-AndroidSdkReadable
Ensure-WslAndroidSdk
Ensure-QtKit "linux" "desktop" "gcc_64"
Ensure-AndroidQtKits
Ensure-UpdateKeys
Ensure-AndroidReleaseKeystore
Write-EnvironmentFile

if ($RunPreflight) {
    Write-Step "Run local release preflight"
    . $EnvFile
    powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $ScriptRoot "local_release.ps1") -Preflight
}

Write-Step "Done"

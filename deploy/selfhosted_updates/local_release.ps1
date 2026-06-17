[CmdletBinding()]
param(
    [string] $Version = "",
    [ValidateSet("windows", "linux", "android")]
    [string[]] $BuildPlatform = @("windows", "linux", "android"),
    [string[]] $RequirePlatform = @(
        "windows-x64",
        "linux-x64",
        "android-arm64-v8a"
    ),
    [string] $ArtifactDir = "",
    [string] $OutDir = "",
    [string] $BaseUrl = $env:SELFHOSTED_UPDATE_BASE_URL,
    [string] $SyncHost = $(if ($env:SELFHOSTED_UPDATE_SYNC_HOST) { $env:SELFHOSTED_UPDATE_SYNC_HOST } else { "10.8.1.0" }),
    [string] $PrivateKey = $env:SELFHOSTED_UPDATE_PRIVATE_KEY_PATH,
    [string] $PublicKeyBase64 = $env:SELFHOSTED_UPDATE_PUBLIC_KEY_PEM_BASE64,
    [string] $WslAndroidHome = $(if ($env:WSL_ANDROID_HOME) { $env:WSL_ANDROID_HOME } else { "" }),
    [ValidateRange(0, 256)]
    [int] $BuildJobs = 0,
    [switch] $SkipBuild,
    [switch] $NoBundleUpdatesInWindowsClient,
    [switch] $Preflight
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$ScriptRoot = Split-Path -Parent $PSCommandPath
$RepoRoot = (Resolve-Path (Join-Path $ScriptRoot "..\..")).Path

function Write-Step([string] $Message) {
    Write-Host ""
    Write-Host "==> $Message"
}

function Assert-Command([string] $CommandName) {
    if (-not (Get-Command $CommandName -ErrorAction SilentlyContinue)) {
        throw "Required command is not available in PATH: $CommandName"
    }
}

function Assert-ExistingFile([string] $Path, [string] $Label) {
    if ([string]::IsNullOrWhiteSpace($Path)) {
        throw "$Label is required"
    }
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "$Label does not exist or is not a file: $Path"
    }
}

function Resolve-BuildJobs {
    if ($BuildJobs -gt 0) {
        return $BuildJobs
    }
    if (-not [string]::IsNullOrWhiteSpace($env:AMNEZIA_BUILD_JOBS)) {
        $parsedJobs = 0
        if ([int]::TryParse($env:AMNEZIA_BUILD_JOBS, [ref] $parsedJobs) -and $parsedJobs -gt 0) {
            return $parsedJobs
        }
    }
    return [Math]::Max(1, [Environment]::ProcessorCount)
}

function Get-ProjectVersion {
    $cmakeLists = Get-Content -LiteralPath (Join-Path $RepoRoot "CMakeLists.txt") -Raw
    if ($cmakeLists -notmatch "set\(AMNEZIAVPN_VERSION\s+([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+)\)") {
        throw "Could not read AMNEZIAVPN_VERSION from CMakeLists.txt"
    }
    return $Matches[1]
}

function Get-RequiredAndroidBuildToolsRevision {
    $androidCmake = Get-Content -LiteralPath (Join-Path $RepoRoot "client\cmake\android.cmake") -Raw
    if ($androidCmake -match "QT_ANDROID_SDK_BUILD_TOOLS_REVISION\s+([0-9]+(?:\.[0-9]+)+)") {
        return $Matches[1]
    }
    return "36.0.0"
}

function Assert-ReleaseInputs {
    Assert-ExistingFile $PrivateKey "SELFHOSTED_UPDATE_PRIVATE_KEY_PATH or -PrivateKey"
    if ([string]::IsNullOrWhiteSpace($PublicKeyBase64)) {
        throw "SELFHOSTED_UPDATE_PUBLIC_KEY_PEM_BASE64 or -PublicKeyBase64 is required"
    }
    if ([string]::IsNullOrWhiteSpace($BaseUrl)) {
        throw "SELFHOSTED_UPDATE_BASE_URL or -BaseUrl is required"
    }
    if ([string]::IsNullOrWhiteSpace($SyncHost)) {
        throw "SELFHOSTED_UPDATE_SYNC_HOST or -SyncHost is required"
    }
    if ($SyncHost -match "://|/") {
        throw "SELFHOSTED_UPDATE_SYNC_HOST must be a host or IP without scheme/path/CIDR: $SyncHost"
    }
}

function Convert-ToWslPath([string] $Path) {
    Assert-Command "wsl.exe"
    $resolved = (Resolve-Path -LiteralPath $Path).Path
    $wslInputPath = $resolved.Replace("\", "/")
    $converted = & wsl.exe wslpath -a $wslInputPath
    if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($converted)) {
        throw "Failed to convert path to WSL path: $Path"
    }
    return $converted.Trim()
}

function Get-VersionSortKey([string] $Name) {
    $parts = @()
    foreach ($part in ($Name -split "[^0-9]+")) {
        if ($part -ne "") {
            $parts += "{0:D8}" -f [int] $part
        }
    }
    return $parts -join "."
}

function Resolve-LatestDirectory([string[]] $Roots, [string] $RelativePattern) {
    $matches = @()
    foreach ($root in $Roots) {
        if ([string]::IsNullOrWhiteSpace($root) -or -not (Test-Path -LiteralPath $root -PathType Container)) {
            continue
        }
        $matches += @(Get-ChildItem -LiteralPath $root -Directory -ErrorAction SilentlyContinue |
            Where-Object { Test-Path -LiteralPath (Join-Path $_.FullName $RelativePattern) })
    }
    if ($matches.Count -eq 0) {
        return ""
    }
    return ($matches | Sort-Object @{ Expression = { Get-VersionSortKey $_.Name } }, Name -Descending | Select-Object -First 1).FullName
}

function Resolve-LatestQtRootDirectory([string[]] $Roots) {
    $matches = @()
    foreach ($root in $Roots) {
        if ([string]::IsNullOrWhiteSpace($root) -or -not (Test-Path -LiteralPath $root -PathType Container)) {
            continue
        }
        foreach ($versionDir in @(Get-ChildItem -LiteralPath $root -Directory -ErrorAction SilentlyContinue)) {
            $hasQtKit = @(Get-ChildItem -LiteralPath $versionDir.FullName -Directory -ErrorAction SilentlyContinue |
                Where-Object { Test-Path -LiteralPath (Join-Path $_.FullName "lib\cmake\Qt6\qt.toolchain.cmake") } |
                Select-Object -First 1).Count -gt 0
            if ($hasQtKit) {
                $matches += $versionDir
            }
        }
    }
    if ($matches.Count -eq 0) {
        return ""
    }
    return ($matches | Sort-Object @{ Expression = { Get-VersionSortKey $_.Name } }, Name -Descending | Select-Object -First 1).FullName
}

function Resolve-QtInstallBase {
    if (-not [string]::IsNullOrWhiteSpace($env:QT_INSTALL_DIR)) {
        return (Resolve-Path -LiteralPath $env:QT_INSTALL_DIR).Path
    }
    if (Test-Path -LiteralPath "C:\Qt" -PathType Container) {
        return "C:\Qt"
    }
    return ""
}

function Resolve-QtRootPath {
    if (-not [string]::IsNullOrWhiteSpace($env:QT_ROOT_PATH)) {
        $explicitQtRoot = (Resolve-Path -LiteralPath $env:QT_ROOT_PATH).Path
        if (Test-Path -LiteralPath (Join-Path $explicitQtRoot "lib\cmake\Qt6\qt.toolchain.cmake") -PathType Leaf) {
            return (Split-Path -Parent $explicitQtRoot)
        }
        return $explicitQtRoot
    }
    $qtBase = Resolve-QtInstallBase
    if ([string]::IsNullOrWhiteSpace($qtBase)) {
        return ""
    }
    $roots = @($qtBase, (Join-Path $qtBase "Qt"))
    return Resolve-LatestQtRootDirectory $roots
}

function Resolve-AndroidShaderToolsLib([string] $QtRootPath) {
    $candidates = @()
    if (-not [string]::IsNullOrWhiteSpace($env:QT_ANDROID_SHADERTOOLS_LIB)) {
        $candidates += $env:QT_ANDROID_SHADERTOOLS_LIB
    }
    if (-not [string]::IsNullOrWhiteSpace($QtRootPath)) {
        $candidates += (Join-Path $QtRootPath "android_arm64_v8a\lib\libQt6ShaderTools_arm64-v8a.so")
        $candidates += (Join-Path $QtRootPath "android\lib\libQt6ShaderTools_arm64-v8a.so")
    }
    $qtBase = Resolve-QtInstallBase
    if (-not [string]::IsNullOrWhiteSpace($qtBase)) {
        $candidates += (Join-Path $qtBase "6.10.1\android_arm64_v8a\lib\libQt6ShaderTools_arm64-v8a.so")
        $candidates += (Join-Path $qtBase "6.10.1\android\lib\libQt6ShaderTools_arm64-v8a.so")
    }
    if (-not [string]::IsNullOrWhiteSpace($env:USERPROFILE)) {
        $candidates += (Join-Path $env:USERPROFILE "Qt\6.10.1\android_arm64_v8a\lib\libQt6ShaderTools_arm64-v8a.so")
        $candidates += (Join-Path $env:USERPROFILE "Qt\6.10.1\android\lib\libQt6ShaderTools_arm64-v8a.so")
    }
    foreach ($candidate in $candidates) {
        if (-not [string]::IsNullOrWhiteSpace($candidate) -and (Test-Path -LiteralPath $candidate -PathType Leaf)) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }
    return ""
}

function Resolve-QifRootPath {
    if (-not [string]::IsNullOrWhiteSpace($env:QIF_ROOT_PATH)) {
        return (Resolve-Path -LiteralPath $env:QIF_ROOT_PATH).Path
    }
    $qtBase = Resolve-QtInstallBase
    if ([string]::IsNullOrWhiteSpace($qtBase)) {
        return ""
    }
    return Resolve-LatestDirectory @((Join-Path $qtBase "Tools\QtInstallerFramework")) "bin"
}

function Resolve-WslQifRootPath {
    if (-not [string]::IsNullOrWhiteSpace($env:WSL_QIF_ROOT_PATH)) {
        return $env:WSL_QIF_ROOT_PATH
    }
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
    $result = Invoke-WslBashOutput $script
    if ([string]::IsNullOrWhiteSpace($result)) {
        return ""
    }
    return $result.Trim()
}

function Assert-QtTargetKit([string] $QtRootPath, [string] $KitName) {
    $kitPath = Join-Path $QtRootPath $KitName
    $toolchainPath = Join-Path $kitPath "lib\cmake\Qt6\qt.toolchain.cmake"
    if (-not (Test-Path -LiteralPath $toolchainPath -PathType Leaf)) {
        throw "Qt kit '$KitName' is required under QT_ROOT_PATH for this local release: $toolchainPath"
    }
}

function Test-QtTargetKit([string] $QtRootPath, [string] $KitName) {
    $toolchainPath = Join-Path (Join-Path $QtRootPath $KitName) "lib\cmake\Qt6\qt.toolchain.cmake"
    return (Test-Path -LiteralPath $toolchainPath -PathType Leaf)
}

function Test-QtTargetModule([string] $QtRootPath, [string] $KitName, [string] $ModuleName) {
    $moduleConfigPath = Join-Path (Join-Path $QtRootPath $KitName) "lib\cmake\$ModuleName\${ModuleName}Config.cmake"
    return (Test-Path -LiteralPath $moduleConfigPath -PathType Leaf)
}

function Assert-QtTargetModule([string] $QtRootPath, [string] $KitName, [string] $ModuleName, [string] $InstallHint) {
    if (-not (Test-QtTargetModule $QtRootPath $KitName $ModuleName)) {
        throw "Qt kit '$KitName' is missing required module $ModuleName under QT_ROOT_PATH. $InstallHint"
    }
}

function Assert-AndroidQtKit([string] $QtRootPath) {
    $requiredModules = @("Qt6RemoteObjects", "Qt6Core5Compat")
    if (Test-QtTargetKit $QtRootPath "android") {
        foreach ($module in $requiredModules) {
            if (-not (Test-QtTargetModule $QtRootPath "android" $module)) {
                throw "Qt Android kit is missing required module $module under '$QtRootPath\android'. Install Android Qt module qtremoteobjects."
            }
        }
        return
    }
    $missing = @()
    $kit = "android_arm64_v8a"
    if (-not (Test-QtTargetKit $QtRootPath $kit)) {
        $missing += $kit
    } else {
        foreach ($module in $requiredModules) {
            if (-not (Test-QtTargetModule $QtRootPath $kit $module)) {
                $missing += "$kit/$module"
            }
        }
    }
    if ($missing.Count -gt 0) {
        throw "Qt Android arm64-v8a kit is required under QT_ROOT_PATH. Install either '$QtRootPath\android' or '$QtRootPath\android_arm64_v8a', including qtremoteobjects. Missing: $($missing -join ', ')"
    }
}

function Quote-Sh([string] $Value) {
    return "'" + $Value.Replace("'", "'\''") + "'"
}

function Invoke-External([string] $FilePath, [string[]] $Arguments, [string] $WorkingDirectory = $RepoRoot) {
    Write-Host ("+ {0} {1}" -f $FilePath, ($Arguments -join " "))
    Push-Location $WorkingDirectory
    try {
        & $FilePath @Arguments
        if ($LASTEXITCODE -ne 0) {
            throw "Command failed with exit code ${LASTEXITCODE}: $FilePath"
        }
    } finally {
        Pop-Location
    }
}

function Invoke-WslBash([string] $Script) {
    Assert-Command "wsl.exe"
    $tempScript = [System.IO.Path]::ChangeExtension([System.IO.Path]::GetTempFileName(), ".sh")
$prelude = @'
set -euo pipefail
export PATH="$HOME/.local/jdk-17/bin:$HOME/.local/bin:$PATH"
run_repo_build_sh() {
    local source_script="deploy/build.sh"
    if [ ! -f "$source_script" ]; then
        echo "Missing $source_script" >&2
        return 127
    fi
    local temp_script="${TMPDIR:-/tmp}/amnezia-build-sh-$$.sh"
    tr -d '\r' < "$source_script" > "$temp_script"
    bash "$temp_script" "$@"
    local status=$?
    rm -f "$temp_script"
    return "$status"
}
'@
    $scriptBody = ($prelude + "`n" + $Script) -replace "`r`n", "`n"
    [System.IO.File]::WriteAllText($tempScript, $scriptBody, [System.Text.UTF8Encoding]::new($false))
    try {
        $tempScriptWsl = Convert-ToWslPath $tempScript
        Invoke-External "wsl.exe" @("bash", $tempScriptWsl)
    } finally {
        Remove-Item -LiteralPath $tempScript -Force -ErrorAction SilentlyContinue
    }
}

function Invoke-WslBashOutput([string] $Script) {
    Assert-Command "wsl.exe"
    $tempScript = [System.IO.Path]::ChangeExtension([System.IO.Path]::GetTempFileName(), ".sh")
    $scriptBody = ("set -euo pipefail`n" + $Script) -replace "`r`n", "`n"
    [System.IO.File]::WriteAllText($tempScript, $scriptBody, [System.Text.UTF8Encoding]::new($false))
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

function Copy-Artifact([string] $SourceRoot, [string] $Pattern, [string] $DestinationRoot, [switch] $Optional) {
    $matches = @(Get-ChildItem -LiteralPath $SourceRoot -Recurse -File -Filter $Pattern -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTimeUtc -Descending)
    if ($matches.Count -eq 0) {
        if ($Optional) {
            return
        }
        throw "Expected artifact not found under ${SourceRoot}: ${Pattern}"
    }
    New-Item -ItemType Directory -Force -Path $DestinationRoot | Out-Null
    Copy-Item -LiteralPath $matches[0].FullName -Destination (Join-Path $DestinationRoot $matches[0].Name) -Force
}

function Build-WindowsInstaller([string] $BundleDir) {
    $buildJobs = Resolve-BuildJobs
    $previousConanNoRemote = $env:CONAN_NO_REMOTE
    $previousPublicKeyBase64 = $env:SELFHOSTED_UPDATE_PUBLIC_KEY_PEM_BASE64
    $previousSyncHost = $env:SELFHOSTED_UPDATE_SYNC_HOST
    $previousBundleDir = $env:SELFHOSTED_UPDATE_BUNDLE_DIR
    $previousBuildJobs = $env:AMNEZIA_BUILD_JOBS
    $previousCmakeBuildParallelLevel = $env:CMAKE_BUILD_PARALLEL_LEVEL
    $env:CONAN_NO_REMOTE = "1"
    $env:AMNEZIA_BUILD_JOBS = [string] $buildJobs
    $env:CMAKE_BUILD_PARALLEL_LEVEL = [string] $buildJobs
    $env:SELFHOSTED_UPDATE_PUBLIC_KEY_PEM_BASE64 = $PublicKeyBase64
    $env:SELFHOSTED_UPDATE_SYNC_HOST = $SyncHost
    if ([string]::IsNullOrWhiteSpace($BundleDir)) {
        Remove-Item Env:\SELFHOSTED_UPDATE_BUNDLE_DIR -ErrorAction SilentlyContinue
    } else {
        $env:SELFHOSTED_UPDATE_BUNDLE_DIR = $BundleDir
    }
    try {
        Invoke-External "cmd.exe" @("/d", "/s", "/c", "`"$RepoRoot\deploy\build.bat`" --installer ifw -arch x64 --jobs $buildJobs")
    } finally {
        $env:CONAN_NO_REMOTE = $previousConanNoRemote
        $env:SELFHOSTED_UPDATE_PUBLIC_KEY_PEM_BASE64 = $previousPublicKeyBase64
        if ($null -eq $previousSyncHost) {
            Remove-Item Env:\SELFHOSTED_UPDATE_SYNC_HOST -ErrorAction SilentlyContinue
        } else {
            $env:SELFHOSTED_UPDATE_SYNC_HOST = $previousSyncHost
        }
        if ($null -eq $previousBuildJobs) {
            Remove-Item Env:\AMNEZIA_BUILD_JOBS -ErrorAction SilentlyContinue
        } else {
            $env:AMNEZIA_BUILD_JOBS = $previousBuildJobs
        }
        if ($null -eq $previousCmakeBuildParallelLevel) {
            Remove-Item Env:\CMAKE_BUILD_PARALLEL_LEVEL -ErrorAction SilentlyContinue
        } else {
            $env:CMAKE_BUILD_PARALLEL_LEVEL = $previousCmakeBuildParallelLevel
        }
        if ($null -eq $previousBundleDir) {
            Remove-Item Env:\SELFHOSTED_UPDATE_BUNDLE_DIR -ErrorAction SilentlyContinue
        } else {
            $env:SELFHOSTED_UPDATE_BUNDLE_DIR = $previousBundleDir
        }
    }
}

function Remove-UnsupportedAndroidArtifacts([string] $DestinationRoot, [string] $ReleaseVersion) {
    $unsupportedPatterns = @(
        "AmneziaVPN_${ReleaseVersion}.aab",
        "AmneziaVPN_${ReleaseVersion}_android9+_universal.apk",
        "AmneziaVPN_${ReleaseVersion}_android9+_armeabi-v7a.apk",
        "AmneziaVPN_${ReleaseVersion}_android9+_x86.apk",
        "AmneziaVPN_${ReleaseVersion}_android9+_x86_64.apk"
    )
    foreach ($pattern in $unsupportedPatterns) {
        Get-ChildItem -LiteralPath $DestinationRoot -Recurse -File -Filter $pattern -ErrorAction SilentlyContinue |
            Remove-Item -Force
    }
}

function Assert-AndroidSigningEnvironment {
    $required = @(
        "QT_ANDROID_KEYSTORE_PATH",
        "QT_ANDROID_KEYSTORE_ALIAS",
        "QT_ANDROID_KEYSTORE_STORE_PASS"
    )
    foreach ($name in $required) {
        if ([string]::IsNullOrWhiteSpace([Environment]::GetEnvironmentVariable($name))) {
            throw "Android local auto-update builds require $name. The APK must be signed with the same key as installed clients."
        }
    }
    Assert-ExistingFile $env:QT_ANDROID_KEYSTORE_PATH "QT_ANDROID_KEYSTORE_PATH"
}

function Assert-WslReady {
    Assert-Command "wsl.exe"
    $repoWsl = Convert-ToWslPath $RepoRoot
    Invoke-External "wsl.exe" @("bash", "-lc", "test -d $(Quote-Sh $repoWsl) && command -v bash >/dev/null")
}

function Assert-WslCommand([string] $CommandName) {
    $bashScript = 'export PATH="$HOME/.local/jdk-17/bin:$HOME/.local/bin:$PATH"; command -v ' + (Quote-Sh $CommandName) + ' >/dev/null'
    & wsl.exe bash -lc $bashScript
    if ($LASTEXITCODE -ne 0) {
        throw "Required command is not available inside WSL: $CommandName"
    }
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

function Test-WindowsJavaHome {
    if ([string]::IsNullOrWhiteSpace($env:JAVA_HOME)) {
        return $false
    }
    return (Test-Path -LiteralPath (Join-Path $env:JAVA_HOME "bin\java.exe") -PathType Leaf)
}

function Assert-JavaForWsl {
    & wsl.exe bash -lc 'export PATH="$HOME/.local/jdk-17/bin:$HOME/.local/bin:$PATH"; command -v java >/dev/null'
    if ($LASTEXITCODE -eq 0) {
        return
    }
    throw "Java must be available inside WSL. Run setup_release_workstation.ps1 -InstallMissing to install the user-local WSL JDK."
}

function Assert-WslAndroidSdkReady {
    $androidHomeWsl = Resolve-WslAndroidHome
    $requiredBuildTools = Get-RequiredAndroidBuildToolsRevision
    $script = @(
        ('test -d ' + (Quote-Sh $androidHomeWsl)),
        ('test -x ' + (Quote-Sh ($androidHomeWsl + "/build-tools/$requiredBuildTools/apksigner"))),
        ('test -n "$(find ' + (Quote-Sh ($androidHomeWsl + "/ndk")) + ' -path "*/toolchains/llvm/prebuilt/linux-x86_64/bin/clang" -executable -print -quit)"'),
        ('test -n "$(find ' + (Quote-Sh ($androidHomeWsl + "/ndk")) + ' -path "*/toolchains/llvm/prebuilt/linux-x86_64/bin/clang++" -executable -print -quit)"')
    ) -join "`n"
    try {
        Invoke-WslBash $script
    } catch {
        throw "Linux Android SDK/NDK is required inside WSL at $androidHomeWsl. Run setup_release_workstation.ps1 -InstallMissing or set WSL_ANDROID_HOME to a Linux Android SDK with build-tools and NDK."
    }
}

function Assert-WslQifReady {
    $wslQifRoot = Resolve-WslQifRootPath
    if ([string]::IsNullOrWhiteSpace($wslQifRoot)) {
        throw "Linux .run builds require Qt Installer Framework inside WSL. Run setup_release_workstation.ps1 -InstallMissing to install qt.tools.ifw.47, or set WSL_QIF_ROOT_PATH to a Linux IFW root containing bin/binarycreator."
    }
    $script = 'test -x ' + (Quote-Sh ($wslQifRoot.TrimEnd("/") + "/bin/binarycreator"))
    try {
        Invoke-WslBash $script
    } catch {
        throw "WSL_QIF_ROOT_PATH must point to a Linux Qt Installer Framework root containing bin/binarycreator: $wslQifRoot"
    }
}

function Assert-LocalReleasePrerequisites {
    Write-Step "Preflight local release prerequisites"
    Assert-Command "python"
    Assert-Command "cmd.exe"
    Assert-ReleaseInputs

    if ($BuildPlatform -contains "linux" -or $BuildPlatform -contains "android") {
        Assert-WslReady
        Assert-WslCommand "conan"
    }
    $qtRootPath = Resolve-QtRootPath
    if ($BuildPlatform -contains "linux" -or $BuildPlatform -contains "android") {
        if ([string]::IsNullOrWhiteSpace($qtRootPath)) {
            throw "QT_ROOT_PATH or QT_INSTALL_DIR must point to a Qt installation for Linux/Android local release builds"
        }
    }
    if ($BuildPlatform -contains "linux") {
        Assert-QtTargetKit $qtRootPath "gcc_64"
        Assert-QtTargetModule $qtRootPath "gcc_64" "Qt6RemoteObjects" "Install Qt module qtremoteobjects for linux desktop gcc_64."
        Assert-QtTargetModule $qtRootPath "gcc_64" "Qt6Core5Compat" "Install Qt module qt5compat for linux desktop gcc_64."
        $qifRootPath = Resolve-QifRootPath
        Assert-WslQifReady
    }
    if ($BuildPlatform -contains "android") {
        Assert-AndroidSigningEnvironment
        Assert-WslAndroidSdkReady
        Assert-JavaForWsl
        Assert-AndroidQtKit $qtRootPath
        Assert-QtTargetModule $qtRootPath "gcc_64" "Qt6RemoteObjectsTools" "Install Qt module qtremoteobjects for linux desktop gcc_64 host tools."
        Assert-QtTargetModule $qtRootPath "gcc_64" "Qt6Core5Compat" "Install Qt module qt5compat for linux desktop gcc_64 host tools."
        Convert-ToWslPath $env:QT_ANDROID_KEYSTORE_PATH | Out-Null
        if (-not [string]::IsNullOrWhiteSpace($env:QT_INSTALL_DIR)) {
            Convert-ToWslPath $env:QT_INSTALL_DIR | Out-Null
        }
    }
}

if ([string]::IsNullOrWhiteSpace($Version)) {
    $Version = Get-ProjectVersion
}

if ([string]::IsNullOrWhiteSpace($ArtifactDir)) {
    $ArtifactDir = Join-Path $RepoRoot "dist\selfhosted-local-artifacts\$Version"
}
if ([string]::IsNullOrWhiteSpace($OutDir)) {
    $OutDir = Join-Path $RepoRoot "dist\selfhosted-updates\$Version"
}

New-Item -ItemType Directory -Force -Path $ArtifactDir | Out-Null

if ($Preflight) {
    Assert-LocalReleasePrerequisites
    Write-Step "Preflight OK"
    return
}

Assert-ReleaseInputs

if (-not $SkipBuild) {
    $qtRootPath = Resolve-QtRootPath
    $qifRootPath = Resolve-QifRootPath
    $buildJobs = Resolve-BuildJobs
    Write-Step "Use parallel build jobs: $buildJobs"

    if ($BuildPlatform -contains "windows") {
        Write-Step "Build Windows x64 installer locally"
        Build-WindowsInstaller ""
        Copy-Artifact (Join-Path $RepoRoot "deploy\build") "AmneziaVPN_${Version}_windows_x64.exe" $ArtifactDir
        Copy-Artifact (Join-Path $RepoRoot "deploy\build") "AmneziaVPN_${Version}_windows_x64.msi" $ArtifactDir -Optional
    }

    if ($BuildPlatform -contains "linux") {
        Write-Step "Build Linux x64 installer locally through WSL"
        $repoWsl = Convert-ToWslPath $RepoRoot
        $buildWsl = "$repoWsl/deploy/build-linux"
        $linuxExports = @()
        if (-not [string]::IsNullOrWhiteSpace($qtRootPath)) {
            $linuxExports += "export QT_ROOT_PATH=$(Quote-Sh (Convert-ToWslPath $qtRootPath))"
        }
        $wslQifRootPath = Resolve-WslQifRootPath
        $linuxExports += "export QIF_ROOT_PATH=$(Quote-Sh $wslQifRootPath)"
        $linuxExports += "export AMNEZIA_BUILD_JOBS=$(Quote-Sh ([string] $buildJobs))"
        $linuxExports += "export CMAKE_BUILD_PARALLEL_LEVEL=$(Quote-Sh ([string] $buildJobs))"
        $linuxExports += "export MAKEFLAGS=$(Quote-Sh ("-j$buildJobs"))"
        $linuxExports += "export CONAN_NO_REMOTE=1"
        $linuxExports += "export SELFHOSTED_UPDATE_PUBLIC_KEY_PEM_BASE64=$(Quote-Sh $PublicKeyBase64)"
        $linuxExports += "export SELFHOSTED_UPDATE_SYNC_HOST=$(Quote-Sh $SyncHost)"
        Invoke-WslBash (("{0}; cd {1} && run_repo_build_sh --source {1} --build {2} --target linux --installer IFW --jobs {3}" -f ($linuxExports -join "; "), (Quote-Sh $repoWsl), (Quote-Sh $buildWsl), $buildJobs).TrimStart("; "))
        Copy-Artifact (Join-Path $RepoRoot "deploy\build-linux") "AmneziaVPN_${Version}_linux_x64.run" $ArtifactDir
    }

    if ($BuildPlatform -contains "android") {
        Write-Step "Build signed Android APKs locally through WSL"
        Assert-AndroidSigningEnvironment
        $repoWsl = Convert-ToWslPath $RepoRoot
        $keystoreWsl = Convert-ToWslPath $env:QT_ANDROID_KEYSTORE_PATH
        $androidHomeWsl = Resolve-WslAndroidHome
        $androidExports = @(
            "export QT_ANDROID_KEYSTORE_PATH=$(Quote-Sh $keystoreWsl)",
            "export QT_ANDROID_KEYSTORE_ALIAS=$(Quote-Sh $env:QT_ANDROID_KEYSTORE_ALIAS)",
            "export QT_ANDROID_KEYSTORE_STORE_PASS=$(Quote-Sh $env:QT_ANDROID_KEYSTORE_STORE_PASS)",
            "export ANDROID_HOME=$(Quote-Sh $androidHomeWsl)",
            "export ANDROID_SDK_ROOT=$(Quote-Sh $androidHomeWsl)",
            "export AMNEZIA_BUILD_JOBS=$(Quote-Sh ([string] $buildJobs))",
            "export CMAKE_BUILD_PARALLEL_LEVEL=$(Quote-Sh ([string] $buildJobs))",
            "export MAKEFLAGS=$(Quote-Sh ("-j$buildJobs"))",
            "export GRADLE_OPTS=$(Quote-Sh ("-Dorg.gradle.workers.max=$buildJobs"))",
            "export CONAN_NO_REMOTE=1",
            'export AWG_ANDROID_GRADLE_USER_HOME="$HOME/.cache/amnezia/awg-android-gradle"',
            "export SELFHOSTED_UPDATE_PUBLIC_KEY_PEM_BASE64=$(Quote-Sh $PublicKeyBase64)",
            "export SELFHOSTED_UPDATE_SYNC_HOST=$(Quote-Sh $SyncHost)"
        )
        if (-not [string]::IsNullOrWhiteSpace($env:QT_ANDROID_KEYSTORE_KEY_PASS)) {
            $androidExports += "export QT_ANDROID_KEYSTORE_KEY_PASS=$(Quote-Sh $env:QT_ANDROID_KEYSTORE_KEY_PASS)"
        }
        if (-not [string]::IsNullOrWhiteSpace($env:QT_INSTALL_DIR)) {
            $androidExports += "export QT_INSTALL_DIR=$(Quote-Sh (Convert-ToWslPath $env:QT_INSTALL_DIR))"
        }
        if (-not [string]::IsNullOrWhiteSpace($qtRootPath)) {
            $androidExports += "export QT_ROOT_PATH=$(Quote-Sh (Convert-ToWslPath $qtRootPath))"
        }
        $androidShaderToolsLib = Resolve-AndroidShaderToolsLib $qtRootPath
        if ([string]::IsNullOrWhiteSpace($androidShaderToolsLib)) {
            throw "Android Qt ShaderTools runtime library is required for Qt5Compat GraphicalEffects. Set QT_ANDROID_SHADERTOOLS_LIB to libQt6ShaderTools_arm64-v8a.so."
        }
        $androidExports += "export QT_ANDROID_SHADERTOOLS_LIB=$(Quote-Sh (Convert-ToWslPath $androidShaderToolsLib))"
        $awgAndroidSourceDir = Join-Path $RepoRoot ".tmp\awg-android-src"
        if (Test-Path -LiteralPath $awgAndroidSourceDir -PathType Container) {
            $androidExports += "export AWG_ANDROID_SOURCE_DIR=$(Quote-Sh (Convert-ToWslPath $awgAndroidSourceDir))"
        }
        & wsl.exe bash -lc 'export PATH="$HOME/.local/jdk-17/bin:$HOME/.local/bin:$PATH"; command -v java >/dev/null'
        if ($LASTEXITCODE -ne 0 -and (Test-WindowsJavaHome)) {
            $androidExports += "export JAVA_HOME=$(Quote-Sh (Convert-ToWslPath $env:JAVA_HOME))"
        }
        $androidExportScript = $androidExports -join "; "

        $androidScript = @(
            $androidExportScript,
            "cd $(Quote-Sh $repoWsl)",
            'mkdir -p "$AWG_ANDROID_GRADLE_USER_HOME" "$HOME/.conan2/p/t" && find "$HOME/.conan2/p/t" -mindepth 1 -maxdepth 1 -exec rm -rf {} +',
            'rename_artifact() { src="$1"; dst="$2"; if [ ! -f "$src" ]; then echo "Missing fresh Android artifact: $src" >&2; return 1; fi; rm -f "$dst"; mv -f "$src" "$dst"; }',
            'if [ -n "${JAVA_HOME:-}" ] && [ -f "$JAVA_HOME/bin/java.exe" ]; then windows_java_home="$JAVA_HOME"; java_shim_dir="$PWD/deploy/build/java-home-shim"; mkdir -p "$java_shim_dir/bin"; for tool in java javac keytool jar; do printf ''#!/bin/sh\nexec "%s/bin/%s.exe" "$@"\n'' "$windows_java_home" "$tool" > "$java_shim_dir/bin/$tool"; chmod +x "$java_shim_dir/bin/$tool"; done; export JAVA_HOME="$java_shim_dir"; export PATH="$JAVA_HOME/bin:$PATH"; fi',
            'sed -i ''s/\r$//'' client/android/gradlew && chmod +x client/android/gradlew',
            'build_dir=./deploy/build-android-arm64-v8a',
            'rm -f deploy/build-android-arm64-v8a/client/android-build/AmneziaVPN_*_android9+_arm64-v8a.apk',
            "run_repo_build_sh --target android --sign --abi arm64-v8a --build `"`$build_dir`" --jobs $buildJobs",
            "version=`$(grep CMAKE_PROJECT_VERSION:STATIC deploy/build-android-arm64-v8a/CMakeCache.txt | cut -d= -f2)",
            "cd deploy/build-android-arm64-v8a/client/android-build && rename_artifact AmneziaVPN.apk AmneziaVPN_`${version}_android9+_arm64-v8a.apk && cd - >/dev/null"
        ) -join "; "
        Invoke-WslBash $androidScript

        Copy-Artifact (Join-Path $RepoRoot "deploy\build-android-arm64-v8a") "AmneziaVPN_${Version}_android9+_arm64-v8a.apk" $ArtifactDir
    }
}

Remove-UnsupportedAndroidArtifacts $ArtifactDir $Version

Write-Step "Create and verify self-hosted update manifest"
if ([string]::IsNullOrWhiteSpace($PrivateKey)) {
    throw "SELFHOSTED_UPDATE_PRIVATE_KEY_PATH or -PrivateKey is required"
}
if ([string]::IsNullOrWhiteSpace($PublicKeyBase64)) {
    throw "SELFHOSTED_UPDATE_PUBLIC_KEY_PEM_BASE64 or -PublicKeyBase64 is required"
}
if ([string]::IsNullOrWhiteSpace($BaseUrl)) {
    throw "SELFHOSTED_UPDATE_BASE_URL or -BaseUrl is required"
}

$requiredArtifactNames = @{
    "windows-x64" = "AmneziaVPN_${Version}_windows_x64.exe"
    "linux-x64" = "AmneziaVPN_${Version}_linux_x64.run"
    "android-arm64-v8a" = "AmneziaVPN_${Version}_android9+_arm64-v8a.apk"
}
$manifestArgs = @(
    "deploy/selfhosted_updates/make_manifest.py",
    "--version", $Version,
    "--base-url", $BaseUrl,
    "--private-key", $PrivateKey,
    "--public-key-base64", $PublicKeyBase64,
    "--out-dir", $OutDir,
    "--auto-install"
)
foreach ($platform in $RequirePlatform) {
    if (-not $requiredArtifactNames.ContainsKey($platform)) {
        throw "Unsupported local self-hosted release platform: $platform"
    }
    $artifactPath = Join-Path $ArtifactDir $requiredArtifactNames[$platform]
    Assert-ExistingFile $artifactPath "Self-hosted update artifact $platform"
    $manifestArgs += @("--require-platform", $platform)
    $manifestArgs += @("--artifact", "$platform=$artifactPath")
}

Invoke-External "python" $manifestArgs

if (-not $NoBundleUpdatesInWindowsClient -and ($BuildPlatform -contains "windows")) {
    Write-Step "Build Windows release client with bundled update payload"
    Build-WindowsInstaller $OutDir
    $adminInstallerDir = Join-Path $RepoRoot "dist\selfhosted-windows-client\$Version"
    New-Item -ItemType Directory -Force -Path $adminInstallerDir | Out-Null
    $adminInstallerSource = Join-Path $RepoRoot "deploy\build\AmneziaVPN_${Version}_windows_x64.exe"
    Assert-ExistingFile $adminInstallerSource "Bundled Windows release client"
    $adminInstallerTarget = Join-Path $adminInstallerDir "AmneziaVPN_${Version}_windows_x64_selfhosted.exe"
    Copy-Item -LiteralPath $adminInstallerSource -Destination $adminInstallerTarget -Force
    Write-Host "Bundled Windows release client: $adminInstallerTarget"
}

Write-Step "Done"
Write-Host "Artifacts: $ArtifactDir"
Write-Host "Manifest output: $OutDir"

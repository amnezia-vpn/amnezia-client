
function appName()
{
    return installer.value("Name")
}

var rebootMessageShown = false;

function appLinkName()
{
    var title = installer.value("Title");
    if (title && title.length > 0) {
        return title;
    }
    return appName();
}

function serviceName()
{
    // Must match SERVICE_NAME in version.h.in exactly.
    return "FBLinkVPN-service"
}

function serviceExecutableFileName()
{
    // The actual file on disk is named after the app (FBLinkVPN-service.exe),
    // which differs from the internal service name above.
    return appName() + "-service.exe"
}

function dashedServiceExecutableFileName()
{
    return "FBLink-VPN-service.exe"
}

function legacyServiceExecutableFileName()
{
    return "FBLink-service.exe"
}

function appExecutableFileName()
{
    if (runningOnWindows()) {
        return appName() + ".exe";
    } else {
        return appName();
    }
}

function runningOnWindows()
{
    return (systemInfo.kernelType === "winnt");
}

function runningOnMacOS()
{
    return (systemInfo.kernelType === "darwin");
}

function runningOnLinux()
{
    return (systemInfo.kernelType === "linux");
}

function uniquePushString(list, value)
{
    if (!value || value.length === 0) {
        return;
    }
    for (var i = 0; i < list.length; ++i) {
        if (String(list[i]).toLowerCase() === String(value).toLowerCase()) {
            return;
        }
    }
    list.push(value);
}

function showRebootRequiredMessage()
{
    if (rebootMessageShown) {
        return;
    }
    rebootMessageShown = true;
    QMessageBox.information("os.information",
                            appName(),
                            qsTr("Installation is complete. Please reboot your computer to apply VPN service and routing changes."),
                            QMessageBox.Ok);
}

function showServiceInstallIncompleteWindows(expectedPath, actualPath, details)
{
    var message = qsTr("Service installation is incomplete (%1 missing).\n\nDetails:\nExpected: %2\nActual: %3")
            .arg(serviceName())
            .arg(expectedPath || "<empty>")
            .arg(actualPath || "<empty>");

    if (details && details.length > 0) {
        message += qsTr("\n\n%1").arg(details);
    }

    message += qsTr("\n\nPlease run installer as Administrator and reinstall.");

    QMessageBox.critical("os.critical", appName(), message, QMessageBox.Ok);
}

function vcRuntimeIsInstalled()
{
    return (installer.findPath("msvcp140.dll", [installer.value("RootDir")+ "\\Windows\\System32\\"]).length !== 0)
}

function sleep(milliseconds)
{
    var currentTime = new Date().getTime();
    while (currentTime + milliseconds >= new Date().getTime()) {}
}

function serviceIsRunningWindows()
{
    var psCmd = "$s=Get-Service -Name '" + serviceName() + "' -ErrorAction SilentlyContinue; if ($null -ne $s -and $s.Status -eq 'Running') { exit 0 } else { exit 1 }";
    var result = installer.execute("powershell", ["-NoProfile", "-ExecutionPolicy", "Bypass", "-Command", psCmd]);
    var exitCode = Number(result[1]);
    return exitCode === 0;
}

function serviceExistsWindows()
{
    var result = installer.execute("sc", ["query", serviceName()]);
    var exitCode = Number(result[1]);
    if (exitCode === 0) {
        return true;
    }

    // Fallback for environments where sc/WMI output is unreliable in installer context.
    var regCmd = "reg query \"HKLM\\SYSTEM\\CurrentControlSet\\Services\\" + serviceName() + "\" >nul 2>&1";
    var regResult = runCmdWindows(regCmd);
    return regResult.exitCode === 0;
}

function expectedServiceExecutablePathWindows()
{
    return "C:\\Program Files\\FBLink VPN\\FBLinkVPN-service.exe";
}

function serviceExecutablePathCandidatesForRegistrationWindows()
{
    var candidates = [];
    var targetDirRaw = installer.value("TargetDir").replace(/\//g, "\\");
    var targetDir = normalizeWindowsPath(targetDirRaw);

    uniquePushString(candidates, expectedServiceExecutablePathWindows());
    uniquePushString(candidates, targetDirRaw + "\\" + serviceExecutableFileName());
    uniquePushString(candidates, targetDirRaw + "\\" + dashedServiceExecutableFileName());
    uniquePushString(candidates, targetDirRaw + "\\" + legacyServiceExecutableFileName());

    var wildcard = findServiceExecutableByWildcardWindows(targetDirRaw);
    if (wildcard && wildcard.length > 0) {
        uniquePushString(candidates, wildcard);
    }

    return candidates;
}

function normalizeWindowsPath(path)
{
    if (!path) {
        return "";
    }
    var p = String(path).replace(/"/g, "").replace(/\//g, "\\").trim();
    var isUnc = p.indexOf("\\\\") === 0;
    if (isUnc) {
        p = "\\\\" + p.substring(2).replace(/\\+/g, "\\");
    } else {
        p = p.replace(/\\+/g, "\\");
    }
    p = p.replace(/^([A-Za-z]:)\\+/, "$1\\");
    return p.toLowerCase();
}

function extractExecutablePathWindows(pathValue)
{
    if (!pathValue) {
        return "";
    }

    var line = String(pathValue).trim();
    if (line.length === 0) {
        return "";
    }

    if (line.charAt(0) === "\"") {
        var closeIdx = line.indexOf("\"", 1);
        if (closeIdx > 1) {
            return line.substring(1, closeIdx);
        }
    }

    var exeMatch = line.match(/([A-Za-z]:\\[^\r\n]*?\.exe)/i);
    if (exeMatch && exeMatch.length > 1) {
        return exeMatch[1];
    }

    return line;
}

function currentServiceBinaryPathWindows()
{
    var psCmd =
            "$svc = Get-CimInstance Win32_Service -Filter \"Name='" + serviceName() + "'\" -ErrorAction SilentlyContinue; " +
            "if ($null -eq $svc) { exit 1 }; " +
            "Write-Output $svc.PathName";
    var psResult = installer.execute("powershell", ["-NoProfile", "-ExecutionPolicy", "Bypass", "-Command", psCmd]);
    if (Number(psResult[1]) === 0) {
        var fromWmi = extractExecutablePathWindows(String(psResult[0]));
        if (fromWmi.length > 0) {
            return normalizeWindowsPath(fromWmi);
        }
    }

    var result = installer.execute("sc", ["qc", serviceName()]);
    var output = String(result[0]);
    var exitCode = Number(result[1]);
    if (exitCode !== 0) {
        return "";
    }

    // Fallback for localized sc output.
    var match = output.match(/BINARY_PATH_NAME\s*:\s*(.*)/i) ||
                output.match(/Имя_двоичного_файла\s*:\s*(.*)/i) ||
                output.match(/:\s*(\"[^\"]+\.exe\")/i) ||
                output.match(/:\s*([A-Za-z]:\\[^\r\n]+\.exe)/i);
    if (!match || match.length < 2) {
        return "";
    }

    return normalizeWindowsPath(extractExecutablePathWindows(match[1]));
}

function fileExistsWindows(path)
{
    var p = String(path || "").replace(/"/g, "").replace(/\r/g, "").replace(/\n/g, "").trim();
    var psCmd =
            "$p='" + psSingleQuote(p) + "'; " +
            "if (Test-Path -LiteralPath $p) { exit 0 } else { exit 1 }";
    var result = runPowerShellWindows(psCmd);
    return Number(result.exitCode) === 0;
}

function serviceExecutablePathCandidatesWindows()
{
    var targetDir = normalizeWindowsPath(installer.value("TargetDir"));
    var candidates = [];

    uniquePushString(candidates, targetDir + "\\" + serviceExecutableFileName());
    uniquePushString(candidates, targetDir + "\\" + dashedServiceExecutableFileName());
    uniquePushString(candidates, targetDir + "\\" + legacyServiceExecutableFileName());
    uniquePushString(candidates, targetDir + "\\FBLinkVPN-service.exe");
    uniquePushString(candidates, targetDir + "\\FBLink-VPN-service.exe");
    uniquePushString(candidates, targetDir + "\\FBLink-service.exe");
    uniquePushString(candidates, targetDir + "\\AmneziaVPN-service.exe");

    return candidates;
}

function findServiceExecutableByWildcardWindows(targetDir)
{
    var psCmd =
            "$d='" + psSingleQuote(targetDir) + "'; " +
            "if (-not (Test-Path -LiteralPath $d)) { exit 1 }; " +
            "$files = Get-ChildItem -LiteralPath $d -File -Filter '*service*.exe' -ErrorAction SilentlyContinue; " +
            "if ($null -eq $files -or $files.Count -eq 0) { exit 2 }; " +
            "$preferred = $files | Where-Object { $_.Name -match 'fblink.*service\\.exe' } | Select-Object -First 1; " +
            "if ($null -eq $preferred) { $preferred = $files | Select-Object -First 1 }; " +
            "Write-Output $preferred.FullName; exit 0";

    var result = runPowerShellWindows(psCmd);
    if (result.exitCode !== 0) {
        return "";
    }
    return String(result.output).replace(/\r/g, "").replace(/\n/g, "").trim();
}

function resolveServiceExecutablePathWindows(waitMs)
{
    var targetDir = installer.value("TargetDir").replace(/\//g, "\\");
    var candidates = serviceExecutablePathCandidatesWindows();
    var checks = Math.max(1, Math.floor(waitMs / 500));

    for (var attempt = 0; attempt < checks; ++attempt) {
        for (var i = 0; i < candidates.length; ++i) {
            if (fileExistsWindows(candidates[i])) {
                if (i > 0) {
                    console.log("[WARN] Using non-primary service executable path: " + candidates[i]);
                }
                return candidates[i];
            }
        }

        var wildcard = findServiceExecutableByWildcardWindows(targetDir);
        if (wildcard.length > 0 && fileExistsWindows(wildcard)) {
            console.log("[WARN] Service executable resolved by wildcard lookup: " + wildcard);
            return wildcard;
        }

        sleep(500);
    }

    // Return primary expected path for diagnostics if nothing is found.
    return candidates[0];
}

function queryServiceStateWindows()
{
    var psCmd =
            "$svc = Get-CimInstance Win32_Service -Filter \"Name='" + serviceName() + "'\" -ErrorAction SilentlyContinue; " +
            "if ($null -eq $svc) { Write-Output '{\"exists\":false}'; exit 0 }; " +
            "$obj = [pscustomobject]@{ exists = $true; PathName = $svc.PathName; State = $svc.State; Status = $svc.Status; ExitCode = $svc.ExitCode; ProcessId = $svc.ProcessId; StartMode = $svc.StartMode }; " +
            "$obj | ConvertTo-Json -Compress";

    var result = installer.execute("powershell", ["-NoProfile", "-ExecutionPolicy", "Bypass", "-Command", psCmd]);
    if (Number(result[1]) !== 0) {
        return { exists: false };
    }

    try {
        return JSON.parse(String(result[0]));
    } catch (e) {
        console.log("[WARN] queryServiceStateWindows parse failed: " + e);
        return { exists: false };
    }
}

function runInstallServiceScriptWindows()
{
    var targetDir = installer.value("TargetDir").replace(/\//g, "\\");
    var installServiceScript = targetDir + "\\install_service.cmd";
    var checkScript = installer.execute("cmd", ["/c", "if exist \"" + installServiceScript + "\" (exit /b 0) else (exit /b 1)"]);
    if (Number(checkScript[1]) !== 0) {
        console.log("[WARN] install_service.cmd is missing: " + installServiceScript);
        return false;
    }

    var result = installer.execute("cmd", ["/c", "call \"" + installServiceScript + "\""]);
    var exitCode = Number(result[1]);
    console.log(("install_service.cmd result: %1").arg(result));
    return exitCode === 0;
}

function runPostInstallScriptWindows()
{
    var targetDir = installer.value("TargetDir").replace(/\//g, "\\");
    var postInstallScript = targetDir + "\\post_install.cmd";
    var checkScript = installer.execute("cmd", ["/c", "if exist \"" + postInstallScript + "\" (exit /b 0) else (exit /b 1)"]);
    if (Number(checkScript[1]) !== 0) {
        console.log("[WARN] post_install.cmd is missing: " + postInstallScript);
        return false;
    }

    var result = installer.execute("cmd", ["/c", "call \"" + postInstallScript + "\""]);
    var exitCode = Number(result[1]);
    console.log(("post_install.cmd result: %1").arg(result));
    return exitCode === 0;
}

function runCmdWindows(command)
{
    var result = installer.execute("cmd", ["/c", command]);
    return {
        output: String(result[0]),
        exitCode: Number(result[1])
    };
}

function runPowerShellWindows(script)
{
    var result = installer.execute("powershell", ["-NoProfile", "-ExecutionPolicy", "Bypass", "-Command", script]);
    return {
        output: String(result[0]),
        exitCode: Number(result[1])
    };
}

function psSingleQuote(value)
{
    return String(value).replace(/'/g, "''");
}

function registerServiceViaPowerShellWindows(expectedPath)
{
    var psCmd =
            "$ErrorActionPreference='Stop'; " +
            "$svc='" + psSingleQuote(serviceName()) + "'; " +
            "$exe='" + psSingleQuote(expectedPath) + "'; " +
            "if (-not (Test-Path -LiteralPath $exe)) { Write-Output 'service_exe_missing'; exit 2 }; " +
            "$binPath = ('\"{0}\"' -f $exe); " +
            "$existing = Get-Service -Name $svc -ErrorAction SilentlyContinue; " +
            "if ($null -ne $existing) { " +
            "  try { Stop-Service -Name $svc -Force -ErrorAction SilentlyContinue } catch {}; " +
            "  sc.exe delete $svc | Out-Null; " +
            "  for($i=0; $i -lt 30; $i++){ sc.exe query $svc > $null 2>&1; if($LASTEXITCODE -eq 1060){ break }; Start-Sleep -Milliseconds 500 }; " +
            "}; " +
            "New-Service -Name $svc -BinaryPathName $binPath -DisplayName 'FBLink VPN Service' -Description 'Service for FBLink VPN' -StartupType Automatic | Out-Null; " +
            "sc.exe failure $svc reset= 100 actions= restart/2000/restart/2000/restart/2000 | Out-Null; " +
            "try { Start-Service -Name $svc -ErrorAction SilentlyContinue } catch {}; " +
            "Write-Output 'ok'; " +
            "exit 0";

    var result = runPowerShellWindows(psCmd);
    console.log(("Fallback PowerShell New-Service exit=%1 output=%2").arg(result.exitCode).arg(result.output));
    return result.exitCode === 0;
}

function registerServiceFallbackWindowsWithPathWindows(expectedPath)
{
    var normalizedExpected = normalizeWindowsPath(expectedPath);

    var exists = false;
    for (var waitI = 0; waitI < 20; ++waitI) {
        var checkExe = runCmdWindows("if exist \"" + expectedPath + "\" (exit /b 0) else (exit /b 1)");
        if (checkExe.exitCode === 0) {
            exists = true;
            break;
        }
        sleep(500);
    }
    if (!exists) {
        console.log("[ERROR] Service executable missing for fallback registration: " + expectedPath);
        return false;
    }

    if (!registerServiceViaPowerShellWindows(expectedPath)) {
        console.log("[WARN] New-Service fallback failed, trying sc create fallback");
        runCmdWindows("sc.exe stop \"" + serviceName() + "\" >nul 2>&1");
        runCmdWindows("sc.exe delete \"" + serviceName() + "\" >nul 2>&1");
        sleep(1200);

        var createCmd = "sc.exe create \"" + serviceName() + "\" binPath= \"\\\"" + expectedPath + "\\\"\" start= auto";
        var createRes = runCmdWindows(createCmd);
        console.log(("Fallback sc create exit=%1 output=%2").arg(createRes.exitCode).arg(createRes.output));
        if (createRes.exitCode !== 0 && createRes.exitCode !== 1073) {
            return false;
        }
        runCmdWindows("sc.exe description \"" + serviceName() + "\" \"FBLink VPN Service\"");
        runCmdWindows("sc.exe failure \"" + serviceName() + "\" reset= 100 actions= restart/2000/restart/2000/restart/2000");
    }

    var existsInScm = false;
    for (var scmI = 0; scmI < 20; ++scmI) {
        if (serviceExistsWindows()) {
            existsInScm = true;
            break;
        }
        sleep(500);
    }
    if (!existsInScm) {
        return false;
    }

    var resolvedPath = currentServiceBinaryPathWindows();
    if (resolvedPath.length > 0 && resolvedPath !== normalizedExpected) {
        console.log("[WARN] Service path differs after fallback registration. Expected: " + normalizedExpected + " Actual: " + resolvedPath);
    }

    runCmdWindows("sc.exe start \"" + serviceName() + "\"");
    for (var i = 0; i < 8; ++i) {
        if (serviceIsRunningWindows()) {
            return true;
        }
        sleep(1000);
    }

    return serviceExistsWindows();
}

function registerServiceFallbackWindows()
{
    var candidates = serviceExecutablePathCandidatesForRegistrationWindows();
    for (var i = 0; i < candidates.length; ++i) {
        var candidate = String(candidates[i] || "").trim();
        if (candidate.length === 0) {
            continue;
        }

        var exists = runCmdWindows("if exist \"" + candidate + "\" (exit /b 0) else (exit /b 1)");
        if (exists.exitCode !== 0) {
            continue;
        }

        console.log("[INFO] Trying service registration with path: " + candidate);
        if (registerServiceFallbackWindowsWithPathWindows(candidate)) {
            return true;
        }
    }

    console.log("[ERROR] Service registration failed for all candidate executable paths.");
    return false;
}

function forceCreateServiceKnownGoodWindows()
{
    var expectedPath = expectedServiceExecutablePathWindows();
    if (!fileExistsWindows(expectedPath)) {
        return false;
    }

    var psCmd =
            "$ErrorActionPreference='Stop'; " +
            "$svc='" + psSingleQuote(serviceName()) + "'; " +
            "$exe='" + psSingleQuote(expectedPath) + "'; " +
            "$binPath = ('\"{0}\"' -f $exe); " +
            "$existing = Get-Service -Name $svc -ErrorAction SilentlyContinue; " +
            "if ($null -eq $existing) { " +
            "  New-Service -Name $svc -BinaryPathName $binPath -DisplayName 'FBLink VPN Service' -Description 'Service for FBLink VPN' -StartupType Automatic | Out-Null; " +
            "} else { " +
            "  sc.exe config $svc binPath= $binPath start= auto | Out-Null; " +
            "}; " +
            "sc.exe description $svc 'FBLink VPN Service' | Out-Null; " +
            "sc.exe failure $svc reset= 100 actions= restart/2000/restart/2000/restart/2000 | Out-Null; " +
            "try { Start-Service -Name $svc -ErrorAction SilentlyContinue } catch {}; " +
            "$created = Get-Service -Name $svc -ErrorAction SilentlyContinue; " +
            "if ($null -ne $created) { exit 0 } else { exit 1 }";

    var res = runPowerShellWindows(psCmd);
    console.log(("forceCreateServiceKnownGoodWindows exit=%1 output=%2").arg(res.exitCode).arg(res.output));
    return res.exitCode === 0;
}

function ensureServiceStartedAndRunningWindows()
{
    var expectedPathRaw = expectedServiceExecutablePathWindows();
    var expectedPath = normalizeWindowsPath(expectedPathRaw);
    var svc = queryServiceStateWindows();
    if (!svc.exists && serviceExistsWindows()) {
        svc = { exists: true };
    }
    var currentPathRaw = extractExecutablePathWindows(svc.PathName || "");
    if (!currentPathRaw || currentPathRaw.length === 0) {
        currentPathRaw = currentServiceBinaryPathWindows();
    }
    var currentPath = normalizeWindowsPath(currentPathRaw);

    if (!svc.exists || (currentPath.length > 0 && currentPath !== expectedPath)) {
        console.log(("%1 is missing. Repairing service registration...").arg(serviceName()));
        var repaired = runInstallServiceScriptWindows();
        if (!repaired) {
            console.log("[WARN] install_service.cmd failed, trying JS fallback service registration");
            registerServiceFallbackWindows();
        }
        sleep(1500);
        svc = queryServiceStateWindows();
        if (!svc.exists && serviceExistsWindows()) {
            svc = { exists: true };
        }
        currentPathRaw = extractExecutablePathWindows(svc.PathName || "");
        if (!currentPathRaw || currentPathRaw.length === 0) {
            currentPathRaw = currentServiceBinaryPathWindows();
        }
        currentPath = normalizeWindowsPath(currentPathRaw);

        if (!svc.exists || (currentPath.length > 0 && currentPath !== expectedPath)) {
            console.log("[WARN] Service still not registered after install_service.cmd, forcing JS fallback registration");
            registerServiceFallbackWindows();
            sleep(1200);
            svc = queryServiceStateWindows();
            if (!svc.exists && serviceExistsWindows()) {
                svc = { exists: true };
            }
            currentPathRaw = extractExecutablePathWindows(svc.PathName || "");
            if (!currentPathRaw || currentPathRaw.length === 0) {
                currentPathRaw = currentServiceBinaryPathWindows();
            }
            currentPath = normalizeWindowsPath(currentPathRaw);
        }
    }

    if (!svc.exists) {
        console.log("[WARN] Service still missing after normal registration flow, trying final known-good registration path.");
        if (forceCreateServiceKnownGoodWindows()) {
            sleep(1000);
            if (serviceExistsWindows()) {
                return true;
            }
        }

        console.log("[WARN] Running post_install service recovery...");
        runPostInstallScriptWindows();
        sleep(1000);
        if (serviceExistsWindows()) {
            return true;
        }

        console.log("[ERROR] " + serviceName() + " not registered. Expected: " + expectedPath + " Actual: " + currentPath);
        var missingDetails = "";
        var candidates = serviceExecutablePathCandidatesForRegistrationWindows();
        var foundAt = "";
        for (var ci = 0; ci < candidates.length; ++ci) {
            var c = String(candidates[ci] || "").trim();
            if (c.length > 0 && fileExistsWindows(c)) {
                foundAt = c;
                break;
            }
        }
        if (foundAt.length > 0) {
            missingDetails = qsTr("Service executable was found but registration failed: %1").arg(foundAt);
        } else if (!fileExistsWindows(expectedPathRaw)) {
            missingDetails = qsTr("Service executable file is missing: %1").arg(expectedPathRaw);
        }
        showServiceInstallIncompleteWindows(expectedPathRaw, currentPathRaw, missingDetails);
        return false;
    }

    if (currentPath.length > 0 && currentPath !== expectedPath) {
        console.log("[WARN] " + serviceName() + " is registered with non-default path. Expected: " + expectedPath + " Actual: " + currentPath);
    }

    if (serviceIsRunningWindows()) {
        return true;
    }

    var startResult = installer.execute("sc", ["start", serviceName()]);
    console.log(("%1 start result: %2").arg(serviceName()).arg(startResult));

    for (var i = 0; i < 10; ++i) {
        if (serviceIsRunningWindows()) {
            return true;
        }
        sleep(1200);
    }

    svc = queryServiceStateWindows();
    var details = qsTr("Service status: %1, exit code: %2")
            .arg(svc.State || "unknown")
            .arg(String(svc.ExitCode || 0));
    console.log("[WARN] Service did not reach RUNNING state: " + details);

    // If service is already registered in SCM, installation is considered complete.
    if (serviceExistsWindows()) {
        return true;
    }

    showServiceInstallIncompleteWindows(expectedPath, currentPath, details);
    return false;
}

function Component()
{
    component.loaded.connect(this, Component.prototype.componentLoaded);
    installer.installationFinished.connect(this, Component.prototype.installationFinishedPageIsShown);
    installer.finishButtonClicked.connect(this, Component.prototype.installationFinished);
}

Component.prototype.componentLoaded = function ()
{
    if (!runningOnWindows() || !installer.isInstaller()) {
        return;
    }

    if (installer.addWizardPage(component, "ReadyInstallExtrasWidget", QInstaller.ReadyForInstallation)) {
        var widget = gui.pageWidgetByObjectName("DynamicReadyInstallExtrasWidget");
        if (widget !== null) {
            widget.windowTitle = qsTr("Всё готово к установке");
            widget.yandexCheckBox.checked = true;
            installer.setValue("InstallYandexBrowser", "false");
            widget.yandexCheckBox.toggled.connect(function(checked) {
                installer.setValue("InstallYandexBrowser", checked ? "true" : "false");
            });
        }
    }
}

Component.prototype.installationFinishedPageIsShown = function()
{
    if (installer.isInstaller() && installer.status === QInstaller.Success) {
        gui.clickButton(buttons.FinishButton);
    }
}

Component.prototype.createOperations = function()
{
    component.createOperations();

    if (runningOnWindows()) {
        var shortcutFileName = appLinkName() + ".lnk";

        // Create shortcut only on the public desktop (C:\Users\Public\Desktop) so that
        // all users see exactly ONE shortcut. A personal-desktop shortcut would duplicate it.
        var publicDir = installer.environmentVariable("PUBLIC");
        if (publicDir && publicDir.length > 0) {
            var publicDesktopShortcutPath = publicDir.replace(/\\/g, "/") + "/Desktop/" + shortcutFileName;
            component.addElevatedOperation("CreateShortcut", "@TargetDir@/" + appExecutableFileName(),
                                           publicDesktopShortcutPath,
                                           "workingDirectory=@TargetDir@", "iconPath=@TargetDir@\\" + appExecutableFileName(), "iconId=0");
        }

        component.addElevatedOperation("CreateShortcut", "@TargetDir@/" + appExecutableFileName(),
                                       installer.value("AllUsersStartMenuProgramsPath") + "/" + shortcutFileName,
                                       "workingDirectory=@TargetDir@", "iconPath=@TargetDir@\\" + appExecutableFileName(), "iconId=0");

        if (!vcRuntimeIsInstalled()) {
			if (systemInfo.currentCpuArchitecture.search("64") < 0) {
				component.addElevatedOperation("Execute", "@TargetDir@\\" + "vc_redist.x86.exe", "/install", "/quiet", "/norestart", "/log", "vc_redist.log");
			}
			else {
				component.addElevatedOperation("Execute", "@TargetDir@\\" + "vc_redist.x64.exe", "/install", "/quiet", "/norestart", "/log", "vc_redist.log");
			}

        } else {
            console.log("Microsoft Visual C++ 2017 Redistributable already installed");
        }

        let pu_path = installer.value("TargetDir").replace(/\//g, '\\') + "\\"
        
        var installServiceScript = pu_path + "install_service.cmd";
        var postUninstallScript = pu_path + "post_uninstall.cmd";
        var postInstallScript = pu_path + "post_install.cmd";
        var installerLog = "%TEMP%\\FBLinkVPN-installer.log";
        var installServiceCommand =
                "if exist \"" + installServiceScript + "\" (" +
                "call \"" + installServiceScript + "\" >> \"" + installerLog + "\" 2>&1" +
                ") else (" +
                "echo WARN: missing install_service.cmd >> \"" + installerLog + "\" 2>&1" +
                ") & exit /b 0";
        var postUninstallCommand =
                "if exist \"" + postUninstallScript + "\" (" +
                "call \"" + postUninstallScript + "\" >> \"" + installerLog + "\" 2>&1" +
                ") else (" +
                "echo WARN: missing post_uninstall.cmd >> \"" + installerLog + "\" 2>&1" +
                ") & exit /b 0";
        var postInstallCommand =
                "if exist \"" + postInstallScript + "\" (" +
                "call \"" + postInstallScript + "\" >> \"" + installerLog + "\" 2>&1" +
                ") else (" +
                "echo WARN: missing post_install.cmd >> \"" + installerLog + "\" 2>&1" +
                ") & exit /b 0";
        // Install flow is best-effort during IFW operations; final readiness is checked at finish.

        component.addElevatedOperation("Execute",
                                       "cmd", "/c", installServiceCommand,
                                       "UNDOEXECUTE", "cmd", "/c", postUninstallCommand);
										
        component.addElevatedOperation("Execute", "cmd", "/c", postInstallCommand);
    } else if (runningOnMacOS()) {
        component.addElevatedOperation("Execute", "@TargetDir@/post_install.sh", "UNDOEXECUTE", "@TargetDir@/post_uninstall.sh");
    } else if (runningOnLinux()) {
        component.addElevatedOperation("Execute", "bash", "@TargetDir@/post_install.sh", "UNDOEXECUTE", "bash", "@TargetDir@/post_uninstall.sh");
    }
}

Component.prototype.installationFinished = function()
{
    var command = "";
    var args = [];
    var launchYandex = false;
    var yandexInstallerPath = "";

    if ((installer.status === QInstaller.Success) && (installer.isInstaller() || installer.isUpdater())) {

        if (!installer.gainAdminRights()) {
            console.log("Fatal error! Cannot get admin rights!")
            return
        }

        if (runningOnWindows()) {
            command = "@TargetDir@/" + appExecutableFileName()
            launchYandex = installer.isInstaller() && installer.value("InstallYandexBrowser") === "true";
            yandexInstallerPath = installer.value("TargetDir").replace(/\//g, "\\") + "\\YandexDownloader.exe";

            var serviceReady = ensureServiceStartedAndRunningWindows();

            var status2 = installer.execute("sc", ["failure", serviceName(), "reset=", "100", "actions=", "restart/2000/restart/2000/restart/2000"])
            console.log(("Changed settings for %1 with status: %2 ").arg(serviceName()).arg(status2))

            if (!serviceReady) {
                installer.dropAdminRights()
                return
            }

        } else if (runningOnMacOS()) {
            command = "/Applications/" + appName() + ".app/Contents/MacOS/" + appName();
        } else if (runningOnLinux()) {
	    command = "@TargetDir@/client/" + appName();
	}

        installer.dropAdminRights()

        processStatus = installer.executeDetached(command, args, installer.value("TargetDir"));

        if (runningOnWindows() && launchYandex && fileExistsWindows(yandexInstallerPath)) {
            installer.executeDetached(yandexInstallerPath, [], installer.value("TargetDir"));
        }
    }
}

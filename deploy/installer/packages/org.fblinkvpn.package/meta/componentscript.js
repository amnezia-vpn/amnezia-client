
function appName()
{
    return installer.value("Name")
}

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

function showRebootRequiredMessage()
{
    QMessageBox.information("os.information",
                            appName(),
                            qsTr("Installation is complete. Please reboot your computer to apply VPN service and routing changes."),
                            QMessageBox.Ok);
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
    var result = installer.execute("sc", ["query", serviceName()]);
    var output = String(result[0]);
    var exitCode = Number(result[1]);

    if (exitCode !== 0) {
        return false;
    }

    // `sc query` output can be localized, but "RUNNING" and numeric state 4
    // are stable markers for a healthy service state.
    return (output.indexOf("RUNNING") !== -1) || (output.match(/:\s*4\s+/) !== null);
}

function serviceExistsWindows()
{
    var result = installer.execute("sc", ["query", serviceName()]);
    var exitCode = Number(result[1]);
    return exitCode === 0;
}

function runInstallServiceScriptWindows()
{
    var targetDir = installer.value("TargetDir").replace(/\//g, "\\");
    var installServiceScript = targetDir + "\\install_service.cmd";
    var result = installer.execute("cmd", ["/c", "call \"" + installServiceScript + "\""]);
    var exitCode = Number(result[1]);
    console.log(("install_service.cmd result: %1").arg(result));
    return exitCode === 0;
}

function ensureServiceStartedAndRunningWindows()
{
    if (!serviceExistsWindows()) {
        console.log(("%1 is missing, trying to install service again").arg(serviceName()));
        runInstallServiceScriptWindows();
        sleep(1200);
    }

    if (!serviceExistsWindows()) {
        QMessageBox.critical("service.missing",
                             appName(),
                             qsTr("Service installation is incomplete: VPN service was not created.\n\nPlease run installer as Administrator and reinstall."),
                             QMessageBox.Ok);
        return false;
    }

    var startResult = installer.execute("net", ["start", serviceName()]);
    console.log(("%1 start result: %2").arg(serviceName()).arg(startResult));

    for (var i = 0; i < 8; ++i) {
        if (serviceIsRunningWindows()) {
            return true;
        }
        sleep(1500);
    }

    if (serviceExistsWindows()) {
        console.log(("%1 installed but not RUNNING yet; continuing").arg(serviceName()));
        return true;
    }

    QMessageBox.critical("service.start.failed",
                         appName(),
                         qsTr("Service installation is incomplete: the VPN service did not enter RUNNING state.\n\nPlease reinstall the app (full installer) and reboot the computer before first launch."),
                         QMessageBox.Ok);
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
        var desktopShortcutPath = QDesktopServices.storageLocation(QDesktopServices.DesktopLocation) + "/" + shortcutFileName;

        component.addOperation("CreateShortcut", "@TargetDir@/" + appExecutableFileName(),
                               desktopShortcutPath,
                               "workingDirectory=@TargetDir@", "iconPath=@TargetDir@\\" + appExecutableFileName(), "iconId=0");

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

    if ((installer.status === QInstaller.Success) && (installer.isInstaller() || installer.isUpdater())) {

        if (!installer.gainAdminRights()) {
            console.log("Fatal error! Cannot get admin rights!")
            return
        }

        showRebootRequiredMessage()

        if (runningOnWindows()) {
            command = "@TargetDir@/" + appExecutableFileName()

            if (!ensureServiceStartedAndRunningWindows()) {
                installer.dropAdminRights()
                return
            }

            var status2 = installer.execute("sc", ["failure", serviceName(), "reset=", "100", "actions=", "restart/2000/restart/2000/restart/2000"])
            console.log(("Changed settings for %1 with status: %2 ").arg(serviceName()).arg(status2))

        } else if (runningOnMacOS()) {
            command = "/Applications/" + appName() + ".app/Contents/MacOS/" + appName();
        } else if (runningOnLinux()) {
	    command = "@TargetDir@/client/" + appName();
	}

        installer.dropAdminRights()

        processStatus = installer.executeDetached(command, args, installer.value("TargetDir"));
    }
}

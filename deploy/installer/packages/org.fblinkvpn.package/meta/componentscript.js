
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

function vcRuntimeIsInstalled()
{
    return (installer.findPath("msvcp140.dll", [installer.value("RootDir")+ "\\Windows\\System32\\"]).length !== 0)
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
        
        // Clean service state before (re)install to keep updates idempotent.
        // Also remove legacy names from previous branding/builds.
        var cleanupCmd = "net stop " + serviceName() + " 2>nul & sc delete " + serviceName() + " 2>nul"
            + " & net stop AmneziaVPN-service 2>nul & sc delete AmneziaVPN-service 2>nul"
            + " & net stop AmneziaWGTunnel$AmneziaVPN 2>nul & sc delete AmneziaWGTunnel$AmneziaVPN 2>nul"
            + " & net stop AmneziaWGTunnel$FBLink 2>nul & sc delete AmneziaWGTunnel$FBLink 2>nul"
            + " & net stop AmneziaVPNSplitTunnel 2>nul & sc delete AmneziaVPNSplitTunnel 2>nul"
            + " & net stop FBLinkSplitTunnel 2>nul & sc delete FBLinkSplitTunnel 2>nul"
            + " & exit 0";
        component.addElevatedOperation("Execute", "cmd", "/c", cleanupCmd);

        component.addElevatedOperation("Execute",
                                       "sc", "create", serviceName(),
                                       "binpath=", "\"" + pu_path + serviceExecutableFileName() + "\"",
                                       "start=", "auto", "depend=", "BFE/nsi",
                                       "UNDOEXECUTE", "cmd", "/c", pu_path + "post_uninstall.cmd");
										
        component.addElevatedOperation("Execute", "cmd", "/c", pu_path + "post_install.cmd");
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

        if (runningOnWindows()) {
            command = "@TargetDir@/" + appExecutableFileName()

            var status1 = installer.execute("net", ["start", serviceName()])
            console.log(("%1 started with status: %2 ").arg(serviceName()).arg(status1))

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

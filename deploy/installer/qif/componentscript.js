
function appName()
{
    return installer.value("Name")
}

function serviceName()
{
    return (appName() + "-service")
}

function tunnelServiceName()
{
    return ("AmneziaWGTunnel$" + appName())
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
    installer.installationStarted.connect(this, Component.prototype.installationStarted);
    installer.installationFinished.connect(this, Component.prototype.installationFinishedPageIsShown);
    installer.finishButtonClicked.connect(this, Component.prototype.installationFinished);
}

Component.prototype.installationStarted = function()
{
    if (!installer.isInstaller() || !runningOnWindows()) {
        return;
    }

    if (!installer.gainAdminRights()) {
        console.log("Fatal error! Cannot get admin rights!")
        return;
    }

    var commands = ["sc stop " + tunnelServiceName(),
                    "sc delete " + tunnelServiceName(),
                    "sc stop " + serviceName(),
                    "taskkill /F /IM " + serviceName() + ".exe",
                    "sc delete " + serviceName(),
                    "exit 0"];

    var status = installer.execute("cmd", ["/c", commands.join(" & ")]);
    console.log(("Removed installed services with status: %1").arg(status))
}

Component.prototype.componentLoaded = function ()
{
    if (!installer.isInstaller() || !runningOnWindows()) {
        return;
    }

    var pageTitle = installer.value("ClosingApplicationsPageTitle");
    if (pageTitle === "") {
        return;
    }

    var widget = component.userInterface("ClosingApplicationsWidget");
    if (widget === null) {
        return;
    }

    widget.windowTitle = pageTitle;
    widget.descriptionLabel.text = installer.value("ClosingApplicationsPageDescription");
    widget.applicationsBrowser.html = appName();

    installer.addWizardPage(component, "ClosingApplicationsWidget", QInstaller.ReadyForInstallation);
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

        component.addOperation("CreateShortcut", "@TargetDir@/" + appExecutableFileName(),
                               QDesktopServices.storageLocation(QDesktopServices.DesktopLocation) + "/" + appName() + ".lnk",
                               "workingDirectory=@TargetDir@", "iconPath=@TargetDir@\\" + appExecutableFileName(), "iconId=0");


        component.addElevatedOperation("CreateShortcut", "@TargetDir@/" + appExecutableFileName(),
                                       installer.value("AllUsersStartMenuProgramsPath") + "/" + appName() + ".lnk",
                                       "workingDirectory=@TargetDir@", "iconPath=@TargetDir@\\" + appExecutableFileName(), "iconId=0");

        if (!vcRuntimeIsInstalled()) {
            var vcRedistFileName = (systemInfo.currentCpuArchitecture.search("64") < 0) ? "vc_redist.x86.exe"
                                                                                       : "vc_redist.x64.exe";
            if (installer.findPath(vcRedistFileName, [installer.value("TargetDir").replace(/\//g, '\\')]).length !== 0) {
                component.addElevatedOperation("Execute", "@TargetDir@\\" + vcRedistFileName, "/install", "/quiet", "/norestart", "/log", "vc_redist.log");
            } else {
                console.log(vcRedistFileName + " is not bundled, relying on the application-local MSVC runtime");
            }
        } else {
            console.log("Microsoft Visual C++ Redistributable already installed");
        }

        let pu_path = installer.value("TargetDir").replace(/\//g, '\\') + "\\"
        component.addElevatedOperation("Execute",
                                       ["sc", "create", serviceName(), "binpath=", pu_path + serviceName() + ".exe",
                                        "start=", "auto", "depend=", "BFE/nsi"],
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

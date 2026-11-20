var requestToQuitFromApp = false;
var updaterCompleted = 0;
var desktopAppProcessRunning = false;
var appInstalledUninstallerPath;
var appInstalledUninstallerPath_x86;

function appName()
{
    return installer.value("Name");
}

function appExecutableFileName()
{
    if (runningOnWindows()) {
        return appName() + ".exe";
    } else {
        return appName();
    }
}

function appInstalled()
{
    if (runningOnWindows()) {
        appInstalledUninstallerPath = installer.value("RootDir") + "Program Files/AmneziaVPN/maintenancetool.exe";
        appInstalledUninstallerPath_x86 = installer.value("RootDir") + "Program Files (x86)/AmneziaVPN/maintenancetool.exe";
    } else if (runningOnMacOS()){
        appInstalledUninstallerPath = "/Applications/" + appName() + ".app/maintenancetool.app/Contents/MacOS/maintenancetool";
    } else if (runningOnLinux()){
        appInstalledUninstallerPath = "/opt/" + appName() + "/maintenancetool";
    }

    return installer.fileExists(appInstalledUninstallerPath) || installer.fileExists(appInstalledUninstallerPath_x86);
}

function endsWith(str, suffix)
{
    return str.indexOf(suffix, str.length - suffix.length) !== -1;
}

function runningOnWindows()
{
    return (installer.value("os") === "win");
}

function runningOnMacOS()
{
    return (installer.value("os") === "mac");
}

function runningOnLinux()
{
    return ((installer.value("os") === "linux") || (installer.value("os") === "x11"));
}

function raiseInstallerWindow()
{
    if (!runningOnMacOS()) {
        return;
    }

    var result = installer.execute("/bin/bash", ["-c", "ps -A | grep -m1 '" + appName() + "' | awk '{print $1}'"]);
    if (Number(result[0]) > 0) {
        var arg = 'tell application \"System Events\" ' +
                '\n      set frontmost of the first process whose unix id is ' + Number(result[0]) + ' to true ' +
                '\n      end tell' +
                '\n       ';
        installer.execute("osascript", ["-e", arg]);
    }
}

function appProcessIsRunning()
{
    if (runningOnWindows()) {
        var result = installer.execute("tasklist");
        if ( Number(result[1]) === 0 ) {
            if (result[0].indexOf(appExecutableFileName()) !== -1) {
                return true;
            }
        }
    } else {
        return checkProcessIsRunning("pgrep -x '" + appName() + "'")
    }

    return false;
}

function checkProcessIsRunning(arg)
{
    var cmdArgs = ["-c", arg];
    var result = installer.execute("/bin/bash", cmdArgs);
    var lines = result[0].trim().split(/\n+/);
    var resultArg1 = Number(lines[0])
    if (resultArg1 >= 2) {
        return true;
    }
    return false;
}

function requestToQuit(installer,gui)
{
    requestToQuitFromApp = true;

    installer.setDefaultPageVisible(QInstaller.IntroductionPage, false);
    installer.setDefaultPageVisible(QInstaller.TargetDirectory, false);
    installer.setDefaultPageVisible(QInstaller.ComponentSelection, false);
    installer.setDefaultPageVisible(QInstaller.LicenseCheck, false);
    installer.setDefaultPageVisible(QInstaller.StartMenuSelection, false);
    installer.setDefaultPageVisible(QInstaller.ReadyForInstallation, false);
    installer.setDefaultPageVisible(QInstaller.PerformInstallation, false);
    installer.setDefaultPageVisible(QInstaller.FinishedPage, false);

    gui.clickButton(buttons.NextButton);
    gui.clickButton(buttons.FinishButton);
    gui.clickButton(buttons.CancelButton);

    if (runningOnWindows()) {
        installer.setCancelled();
    }
}

function prepareClosingApplicationsPage()
{
    desktopAppProcessRunning = appProcessIsRunning();

    if (!desktopAppProcessRunning) {
        return;
    }

    installer.setValue("ClosingApplicationsPageRequested", "true");
}

function removePreviousInstallation()
{
    if (!appInstalled()) {
        return true;
    }

    if (QMessageBox.Ok !== QMessageBox.information("os.information", appName(),
                                                   qsTr("The application is already installed.") + " " +
                                                   qsTr("We need to remove the old installation first. Do you wish to proceed?"),
                                                   QMessageBox.Ok | QMessageBox.Cancel)) {
        console.log("Request to quit from user");
        installer.setCancelled();
        return false;
    }

    var resultArray = [];

    if (installer.fileExists(appInstalledUninstallerPath_x86)) {
        console.log("Starting uninstallation " + appInstalledUninstallerPath_x86);
        resultArray = installer.execute(appInstalledUninstallerPath_x86);
    }

    if (installer.fileExists(appInstalledUninstallerPath)) {
        console.log("Starting uninstallation " + appInstalledUninstallerPath);
        resultArray = installer.execute(appInstalledUninstallerPath);
    }

    console.log("Uninstaller finished with code: " + resultArray[1]);

    if (Number(resultArray[1]) !== 0) {
        console.log("Uninstallation aborted by user");
        installer.setCancelled();
        return false;
    }

    raiseInstallerWindow();
    return true;
}


Controller.prototype.PerformInstallationPageCallback = function()
{
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.LicenseAgreementPageCallback = function()
{
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.FinishedPageCallback = function ()
{
    if (desktopAppProcessRunning) {
        gui.clickButton(buttons.FinishButton);
    } else if (installer.isUpdater()) {
        installer.autoAcceptMessageBoxes();
        gui.clickButton(buttons.FinishButton);
    }
}

Controller.prototype.RestartPageCallback = function ()
{
    updaterCompleted = 1;
    gui.clickButton(buttons.FinishButton);
}

Controller.prototype.StartMenuDirectoryPageCallback = function()
{
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.ComponentSelectionPageCallback = function()
{
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.ReadyForInstallationPageCallback = function()
{
    if (installer.isUpdater()) {
        gui.clickButton(buttons.CommitButton);
        return;
    }

    if (!installer.isInstaller() || !runningOnWindows() || !appProcessIsRunning()) {
        return;
    }

    closeDesktopApp();
}

Controller.prototype.TargetDirectoryPageCallback = function ()
{
    var widget = gui.pageById(QInstaller.TargetDirectory);

    if (widget !== null) {
        widget.BrowseDirectoryButton.clicked.disconnect(onBrowseButtonClicked);
        widget.BrowseDirectoryButton.clicked.connect(onBrowseButtonClicked);

        gui.clickButton(buttons.NextButton);
    }
}

Controller.prototype.IntroductionPageCallback = function ()
{
    var widget = gui.currentPageWidget();
    if (installer.isUpdater() && updaterCompleted === 1) {
        gui.clickButton(buttons.FinishButton);
        gui.clickButton(buttons.CancelButton);
        return;
    }

    if (installer.isUninstaller()) {
        if (widget !== null) {
            widget.findChild("PackageManagerRadioButton").visible = false;
            widget.findChild("UpdaterRadioButton").visible = false;
        }
    }

    if (installer.isUpdater()) {
        gui.clickButton(buttons.NextButton);
    }
}

onBrowseButtonClicked = function()
{
    var widget = gui.pageById(QInstaller.TargetDirectory);
    if (widget !== null) {
        if (runningOnWindows()) {
            // On Windows we are appending \<APP_NAME> if selected path don't ends with <APP_NAME>
            var targetDir = widget.TargetDirectoryLineEdit.text;
            if (! endsWith(targetDir, appName())) {
                targetDir = targetDir + "\\" + appName();
            }
            installer.setValue("TargetDir", targetDir);
            widget.TargetDirectoryLineEdit.setText(installer.value("TargetDir"));
        }
    }
}

onNextButtonClicked = function()
{
    var widget = gui.pageById(QInstaller.TargetDirectory);
    if (widget !== null) {
        installer.setValue("APP_BUNDLE_TARGET_DIR", widget.TargetDirectoryLineEdit.text);
    }
}

function Controller () {
    console.log("OS: %1, architecture: %2".arg(systemInfo.prettyProductName).arg(systemInfo.currentCpuArchitecture));

    if (installer.isInstaller() || installer.isUpdater()) {
        console.log("Check if app already installed: " + appInstalled());
    }

    if (runningOnWindows()) {
        installer.setValue("AllUsers", "true");
    }

    if (installer.isInstaller()) {
        installer.setDefaultPageVisible(QInstaller.ComponentSelection, false);
        installer.setDefaultPageVisible(QInstaller.TargetDirectory, false);
        installer.setDefaultPageVisible(QInstaller.StartMenuDirectoryPage, false);
        installer.setDefaultPageVisible(QInstaller.LicenseCheck, false);

        if (runningOnWindows()) {
            prepareClosingApplicationsPage();
            return;
        }

        isDesktopAppProcessRunningMessageLoop();

        if (requestToQuitFromApp === true) {
            requestToQuit(installer, gui);
            return;
        }

        if (runningOnMacOS()) {
            installer.setMessageBoxAutomaticAnswer("OverwriteTargetDirectory", QMessageBox.Yes);
        }

        removePreviousInstallation();

    } else if (installer.isUninstaller()) {
        isDesktopAppProcessRunningMessageLoop();

        if (requestToQuitFromApp === true) {
            requestToQuit(installer, gui);
            return;
        }

    } else if (installer.isUpdater()) {
        installer.setMessageBoxAutomaticAnswer("cancelInstallation", QMessageBox.No);
        installer.installationFinished.connect(function() {
            gui.clickButton(buttons.NextButton);
        });
    }
}

isDesktopAppProcessRunningMessageLoop = function ()
{
    if (requestToQuitFromApp === true) {
        return;
    }

    if (installer.isUpdater()) {
        for (var i = 0; i < 400; i++) {
            desktopAppProcessRunning = appProcessIsRunning();
            if (!desktopAppProcessRunning) {
                break;
            }
        }
    }
    desktopAppProcessRunning = appProcessIsRunning();

    while (desktopAppProcessRunning) {
        if (!userAgreedToCloseDesktopApp()) {
            requestToQuitFromApp = true;
            return;
        }
        if (runningOnWindows()) {
            closeDesktopApp();
        }
        desktopAppProcessRunning = appProcessIsRunning();
    }
}

function userAgreedToCloseDesktopApp()
{
    var title = appName() + " installer";
    if (runningOnWindows()) {
        return QMessageBox.question("QMessageBox", title,
                                    qsTr("%1 is running. Close it and continue the installation?").arg(appName()),
                                    QMessageBox.Yes | QMessageBox.No) === QMessageBox.Yes;
    }
    return QMessageBox.warning("QMessageBox", title,
                               qsTr("%1 is running. Close it to continue the installation.").arg(appName()),
                               QMessageBox.Retry | QMessageBox.Abort) === QMessageBox.Retry;
}

function waitForDesktopAppExit(timeoutSeconds)
{
    installer.execute("powershell", ["-NoProfile", "-NonInteractive", "-Command",
                                     "Wait-Process -Name " + appName() + " -Timeout " + timeoutSeconds
                                     + " -ErrorAction SilentlyContinue"]);
    return !appProcessIsRunning();
}

function closeDesktopApp()
{
    installer.execute("taskkill", ["/IM", appExecutableFileName()]);
    if (waitForDesktopAppExit(10)) {
        return;
    }
    installer.execute("taskkill", ["/F", "/IM", appExecutableFileName()]);
    waitForDesktopAppExit(5);
}

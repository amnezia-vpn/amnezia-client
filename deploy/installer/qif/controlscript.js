var requestToQuitFromApp = false;
var updaterCompleted = 0;
var desktopAppProcessRunning = false;
var appInstalledUninstallerPath;
var appInstalledUninstallerPath_x86;
var cachedSystemLanguage;

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

function sleep(milliseconds) {
    var currentTime = new Date().getTime();
    while (currentTime + milliseconds >= new Date().getTime()) {}
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

    installer.setValue("ClosingApplicationsPageTitle", localizedText("preparingToInstallTitle"));
    installer.setValue("ClosingApplicationsPageDescription", localizedText("appRunningCloseDescription"));
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

    for (var i = 0; i < 300; i++) {
        sleep(100);
        if (!installer.fileExists(appInstalledUninstallerPath)) {
            break;
        }
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

    if (!installer.isInstaller() || !runningOnWindows()) {
        return;
    }

    if (appProcessIsRunning()) {
        closeDesktopApp();
    }
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
        return QMessageBox.question("QMessageBox", title, localizedText("appRunningCloseQuestion"),
                                    QMessageBox.Yes | QMessageBox.No) === QMessageBox.Yes;
    }
    return QMessageBox.warning("QMessageBox", title, localizedText("appRunningCloseHint"),
                               QMessageBox.Retry | QMessageBox.Abort) === QMessageBox.Retry;
}

function waitForDesktopAppExit(attempts)
{
    for (var i = 0; i < attempts; i++) {
        if (!appProcessIsRunning()) {
            return true;
        }
        sleep(500);
    }
    return !appProcessIsRunning();
}

function closeDesktopApp()
{
    installer.execute("taskkill", ["/IM", appExecutableFileName()]);
    if (waitForDesktopAppExit(20)) {
        return;
    }
    installer.execute("taskkill", ["/F", "/IM", appExecutableFileName()]);
    waitForDesktopAppExit(10);
}

function systemLanguage()
{
    if (cachedSystemLanguage === undefined) {
        var result = runningOnWindows()
                ? installer.execute("powershell", ["-NoProfile", "-NonInteractive", "-Command", "(Get-UICulture).Name"])
                : installer.execute("/bin/bash", ["-c", "echo ${LC_ALL:-${LC_MESSAGES:-$LANG}}"]);
        cachedSystemLanguage = String(result[0]).trim().split(/[-_.@]/)[0].toLowerCase();
    }
    return cachedSystemLanguage;
}

function localizedText(key)
{
    var texts = installerTexts[key];
    return (texts[systemLanguage()] || texts.en).arg(appName());
}

var installerTexts = {
    preparingToInstallTitle: {
        en: "Preparing to Install",
        ar: "التحضير للتثبيت",
        es: "Preparando la instalación",
        fa: "آماده‌سازی برای نصب",
        fr: "Préparation de l'installation",
        hi: "इंस्टॉलेशन की तैयारी",
        id: "Menyiapkan Instalasi",
        ko: "설치 준비",
        my: "ထည့်သွင်းရန် ပြင်ဆင်နေသည်",
        ru: "Подготовка к установке",
        th: "กำลังเตรียมการติดตั้ง",
        uk: "Підготовка до встановлення",
        ur: "انسٹالیشن کی تیاری",
        zh: "准备安装"
    },
    appRunningCloseDescription: {
        en: "The following applications are using files that need to be updated. Setup will close them and start %1 again when the installation is complete.",
        ar: "التطبيقات التالية تستخدم ملفات يجب تحديثها. سيقوم المثبّت بإغلاقها ثم تشغيل %1 مرة أخرى بعد اكتمال التثبيت.",
        es: "Las siguientes aplicaciones están usando archivos que deben actualizarse. El instalador las cerrará y volverá a iniciar %1 cuando finalice la instalación.",
        fa: "برنامه‌های زیر از فایل‌هایی استفاده می‌کنند که باید به‌روزرسانی شوند. نصب‌کننده آن‌ها را می‌بندد و پس از پایان نصب، %1 را دوباره اجرا می‌کند.",
        fr: "Les applications suivantes utilisent des fichiers qui doivent être mis à jour. Le programme d'installation va les fermer, puis relancer %1 une fois l'installation terminée.",
        hi: "निम्नलिखित ऐप्लिकेशन उन फ़ाइलों का उपयोग कर रहे हैं जिन्हें अपडेट करना आवश्यक है। सेटअप उन्हें बंद करेगा और इंस्टॉलेशन पूरा होने पर %1 को फिर से शुरू करेगा।",
        id: "Aplikasi berikut menggunakan berkas yang perlu diperbarui. Penginstal akan menutupnya dan menjalankan %1 kembali setelah instalasi selesai.",
        ko: "다음 응용 프로그램이 업데이트해야 하는 파일을 사용 중입니다. 설치 프로그램이 해당 프로그램을 종료하고 설치가 완료되면 %1을(를) 다시 시작합니다.",
        my: "အောက်ပါ အက်ပ်များသည် အပ်ဒိတ်လုပ်ရန် လိုအပ်သော ဖိုင်များကို အသုံးပြုနေသည်။ Setup က ၎င်းတို့ကို ပိတ်ပြီး ထည့်သွင်းမှု ပြီးဆုံးသောအခါ %1 ကို ပြန်လည်စတင်ပါမည်။",
        ru: "Перечисленные приложения используют файлы, которые нужно обновить. Программа установки закроет их и снова запустит %1 после завершения установки.",
        th: "แอปต่อไปนี้กำลังใช้ไฟล์ที่ต้องอัปเดต ตัวติดตั้งจะปิดแอปเหล่านี้และเปิด %1 อีกครั้งเมื่อการติดตั้งเสร็จสิ้น",
        uk: "Перелічені застосунки використовують файли, які потрібно оновити. Програма встановлення закриє їх і знову запустить %1 після завершення встановлення.",
        ur: "درج ذیل ایپلیکیشنز ایسی فائلیں استعمال کر رہی ہیں جنہیں اپ ڈیٹ کرنا ضروری ہے۔ سیٹ اپ انہیں بند کر دے گا اور انسٹالیشن مکمل ہونے پر %1 دوبارہ شروع کرے گا۔",
        zh: "以下应用程序正在使用需要更新的文件。安装程序将关闭它们，并在安装完成后重新启动 %1。"
    },
    appRunningCloseQuestion: {
        en: "%1 is running. Close it and continue the installation?",
        ar: "%1 قيد التشغيل. هل تريد إغلاقه ومتابعة التثبيت؟",
        es: "%1 se está ejecutando. ¿Cerrarlo y continuar con la instalación?",
        fa: "%1 در حال اجرا است. برنامه بسته شود و نصب ادامه یابد؟",
        fr: "%1 est en cours d'exécution. Le fermer et poursuivre l'installation ?",
        hi: "%1 चल रहा है। क्या इसे बंद करके इंस्टॉलेशन जारी रखें?",
        id: "%1 sedang berjalan. Tutup aplikasi dan lanjutkan instalasi?",
        ko: "%1이(가) 실행 중입니다. 종료하고 설치를 계속하시겠습니까?",
        my: "%1 လည်ပတ်နေသည်။ ၎င်းကို ပိတ်ပြီး ထည့်သွင်းခြင်းကို ဆက်လုပ်မလား။",
        ru: "%1 запущен. Закрыть приложение и продолжить установку?",
        th: "%1 กำลังทำงานอยู่ ปิดแอปและดำเนินการติดตั้งต่อหรือไม่",
        uk: "%1 запущено. Закрити застосунок і продовжити встановлення?",
        ur: "%1 چل رہا ہے۔ کیا اسے بند کر کے انسٹالیشن جاری رکھیں؟",
        zh: "%1 正在运行。是否关闭它并继续安装？"
    },
    appRunningCloseHint: {
        en: "%1 is running. Close it to continue the installation.",
        ar: "%1 قيد التشغيل. أغلق التطبيق لمتابعة التثبيت.",
        es: "%1 se está ejecutando. Ciérrelo para continuar con la instalación.",
        fa: "%1 در حال اجرا است. برای ادامه نصب، برنامه را ببندید.",
        fr: "%1 est en cours d'exécution. Fermez-le pour poursuivre l'installation.",
        hi: "%1 चल रहा है। इंस्टॉलेशन जारी रखने के लिए इसे बंद करें।",
        id: "%1 sedang berjalan. Tutup aplikasi untuk melanjutkan instalasi.",
        ko: "%1이(가) 실행 중입니다. 설치를 계속하려면 앱을 종료하세요.",
        my: "%1 လည်ပတ်နေသည်။ ထည့်သွင်းခြင်းကို ဆက်လုပ်ရန် ၎င်းကို ပိတ်ပါ။",
        ru: "%1 запущен. Закройте приложение, чтобы продолжить установку.",
        th: "%1 กำลังทำงานอยู่ โปรดปิดแอปเพื่อดำเนินการติดตั้งต่อ",
        uk: "%1 запущено. Закрийте застосунок, щоб продовжити встановлення.",
        ur: "%1 چل رہا ہے۔ انسٹالیشن جاری رکھنے کے لیے اسے بند کریں۔",
        zh: "%1 正在运行。请关闭它以继续安装。"
    }
};

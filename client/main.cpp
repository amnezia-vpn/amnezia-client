#include <QDebug>
#include <QTimer>

#include "fblink_application.h"
#include "core/osSignalHandler.h"
#include "migrations.h"
#include "version.h"

#include <QTimer>

#ifdef Q_OS_WIN
    #include "Windows.h"
    #include "shellapi.h"
#endif

#if defined(Q_OS_IOS)
    #include "platforms/ios/QtAppDelegate-C-Interface.h"
#endif

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS) && !defined(MACOS_NE)
bool isAnotherInstanceRunning()
{
    QLocalSocket socket;
    socket.connectToServer("FBLinkInstance");
    if (socket.waitForConnected(500)) {
        qWarning() << "FBLink is already running";
        return true;
    }
    return false;
}
#endif

int main(int argc, char *argv[])
{
    Migrations migrationsManager;
    migrationsManager.doMigrations();

#ifdef Q_OS_WIN
    // Check for administrator privileges
    BOOL isAdmin = FALSE;
    PSID administratorsGroup = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &administratorsGroup)) {
        CheckTokenMembership(NULL, administratorsGroup, &isAdmin);
        FreeSid(administratorsGroup);
    }
    
    // Relaunch with elevated privileges if not admin
    if (!isAdmin) {
        qDebug() << "Application is not running as administrator. Relaunching with UAC prompt...";
        wchar_t szPath[MAX_PATH];
        if (GetModuleFileNameW(NULL, szPath, ARRAYSIZE(szPath))) {
            SHELLEXECUTEINFOW sei = { sizeof(sei) };
            sei.lpVerb = L"runas";
            sei.lpFile = szPath;
            sei.hwnd = NULL;
            sei.nShow = SW_NORMAL;
            if (ShellExecuteExW(&sei)) {
                // Successfully started the elevated process, exit the current one.
                return 0;
            } else {
                qWarning() << "Failed to elevate privileges. The application may not function correctly.";
            }
        }
    }

    AllowSetForegroundWindow(ASFW_ANY);
#endif

#ifdef Q_OS_ANDROID
    // QTBUG-95974 QTBUG-95764 QTBUG-102168
    qputenv("QT_ANDROID_DISABLE_ACCESSIBILITY", "1");
    qputenv("ANDROID_OPENSSL_SUFFIX", "_3");
#endif

    FBLinkApplication app(argc, argv);
    OsSignalHandler::setup();

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS) && !defined(MACOS_NE)
    if (isAnotherInstanceRunning()) {
        QTimer::singleShot(1000, &app, [&]() { app.quit(); });
        return app.exec();
    }
    app.startLocalServer();
#endif

// Allow to raise app window if secondary instance launched
#ifdef Q_OS_WIN
    AllowSetForegroundWindow(0);
#endif

    app.registerTypes();

    app.setApplicationName(APPLICATION_NAME);
    app.setOrganizationName(ORGANIZATION_NAME);
    app.setApplicationDisplayName("FBLink VPN");

    app.loadFonts();

    bool doExec = app.parseCommands();

    if (doExec) {
        app.init();

        qInfo().noquote() << QString("Started %1 version %2 %3").arg(APPLICATION_NAME, APP_VERSION, GIT_COMMIT_HASH);
        qInfo().noquote() << QString("%1 (%2)").arg(QSysInfo::prettyProductName(), QSysInfo::currentCpuArchitecture());
        qInfo().noquote() << QString("SSL backend: %1").arg(QSslSocket::sslLibraryVersionString());

        return app.exec();
    }
    return 0;
}

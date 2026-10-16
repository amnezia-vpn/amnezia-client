#include <QDebug>
#include <QTimer>
#include <libssh/libssh.h>
#include <openssl/ssl.h>

#include "amneziaApplication.h"
#include "core/utils/osSignalHandler.h"
#include "core/utils/migrations.h"
#include "core/utils/appUiConfig.h"
#include "version.h"


// use openssl symbols to prevent linker throwing-off the OpenSSL dependency
void anchorOpenSSL() {
    SSL_CTX_free(SSL_CTX_new(TLS_method()));
}

#ifdef Q_OS_WIN
    #include "Windows.h"
#endif

#if defined(Q_OS_IOS)
    #include "platforms/ios/QtAppDelegate-C-Interface.h"
#endif

int main(int argc, char *argv[])
{
    Migrations migrationsManager;
    migrationsManager.doMigrations();

#ifdef Q_OS_WIN
    AllowSetForegroundWindow(ASFW_ANY);
#endif

#ifdef Q_OS_ANDROID
    // QTBUG-95974 QTBUG-95764 QTBUG-102168
    qputenv("QT_ANDROID_DISABLE_ACCESSIBILITY", "1");
    qputenv("ANDROID_OPENSSL_SUFFIX", "_3");
#endif

    AmneziaApplication app(argc, argv);
    app.setApplicationName(APPLICATION_NAME);
    app.setOrganizationName(ORGANIZATION_NAME);
    app.setApplicationDisplayName(APPLICATION_NAME);

    OsSignalHandler::setup();

    anchorOpenSSL();

    ssh_init();
    QObject::connect(&app, &QCoreApplication::aboutToQuit, []() {
        ssh_finalize();
    });

    const bool doExec = app.parseCommands();
    if (!doExec) {
        return 0;
    }

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS) && !defined(MACOS_NE)
    if (const auto forwarded = app.forwardToRunningInstance()) {
        return *forwarded;
    }
    if (const auto handled = app.handleControlCommandWithoutRunningInstance()) {
        return *handled;
    }
    if (!app.startLocalServer()) {
        return 2;
    }
#endif

// Allow to raise app window if secondary instance launched
#ifdef Q_OS_WIN
    AllowSetForegroundWindow(0);
#endif

    app.registerTypes();
    app.loadFonts();
    app.init();

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS) && !defined(MACOS_NE)
    app.executeStartupControlCommand();
#endif

    qInfo().noquote() << QString("Started %1 version %2 %3").arg(APPLICATION_NAME, APP_VERSION, GIT_COMMIT_HASH);
    qInfo().noquote() << QString("%1 (%2)").arg(QSysInfo::prettyProductName(), QSysInfo::currentCpuArchitecture());

    return app.exec();
}

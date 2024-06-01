#include <QDebug>
#include <QTimer>
#include <QIcon>

#include "amnezia_application.h"
#include "migrations.h"
#include "version.h"

#include <QTimer>

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

    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);

#ifdef Q_OS_WIN
    AllowSetForegroundWindow(ASFW_ANY);
#endif

#ifdef Q_OS_ANDROID
    // QTBUG-95974 QTBUG-95764 QTBUG-102168
    qputenv("QT_ANDROID_DISABLE_ACCESSIBILITY", "1");
    qputenv("ANDROID_OPENSSL_SUFFIX", "_3");
#endif

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    AmneziaApplication app(argc, argv);
#else
    AmneziaApplication app(argc, argv, true,
                           SingleApplication::Mode::User | SingleApplication::Mode::SecondaryNotification);

    if (!app.isPrimary()) {
        QTimer::singleShot(1000, &app, [&]() { app.quit(); });
        return app.exec();
    }
#endif

// Allow to raise app window if secondary instance launched
#ifdef Q_OS_WIN
    AllowSetForegroundWindow(0);
#endif

    app.registerTypes();

    app.setApplicationName(APPLICATION_NAME);
    app.setOrganizationName(ORGANIZATION_NAME);
    app.setApplicationDisplayName(APPLICATION_NAME);
    app.setWindowIcon(QIcon(":/images/app.ico"));

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

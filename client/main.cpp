#include <QDebug>
#include <QTimer>

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

// QTBUG-95974 QTBUG-95764 QTBUG-102168
#ifdef Q_OS_ANDROID
    qputenv("QT_ANDROID_DISABLE_ACCESSIBILITY", "1");
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

    app.loadFonts();

    bool doExec = app.parseCommands();

    if (doExec) {
        app.init();

        qInfo().noquote() << QString("Started %1 version %2").arg(APPLICATION_NAME, APP_VERSION);
        qInfo().noquote() << QString("%1 (%2)").arg(QSysInfo::prettyProductName(), QSysInfo::currentCpuArchitecture());

        return app.exec();
    }
    return 0;
}

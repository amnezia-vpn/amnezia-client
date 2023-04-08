#include <QLoggingCategory>
#include <QDebug>
#include <QTimer>

#include "amnezia_application.h"
#include "defines.h"
#include "migrations.h"

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

    QLoggingCategory::setFilterRules(QStringLiteral("qtc.ssh=false"));
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);

#ifdef Q_OS_WIN
    AllowSetForegroundWindow(ASFW_ANY);
#endif


#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    AmneziaApplication app(argc, argv);
#else
    AmneziaApplication app(argc, argv, true, SingleApplication::Mode::User | SingleApplication::Mode::SecondaryNotification);

    if (!app.isPrimary()) {
        QTimer::singleShot(1000, &app, [&](){
            app.quit();
        });
        return app.exec();
    }
#endif

// Allow to raise app window if secondary instance launched
#ifdef Q_OS_WIN
    AllowSetForegroundWindow(0);
#endif

#if defined(Q_OS_IOS)
    QtAppDelegateInitialize();
#endif

    app.registerTypes();

    app.setApplicationName(APPLICATION_NAME);
    app.setOrganizationName(ORGANIZATION_NAME);
    app.setApplicationDisplayName(APPLICATION_NAME);

    app.loadTranslator();
    app.loadFonts();

    bool doExec = app.parseCommands();

    if (doExec) {
        app.init();
        return app.exec();
    }
    return 0;
}

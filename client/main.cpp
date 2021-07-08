#include <QApplication>
#include <QFontDatabase>
#include <QCommandLineParser>
#include <QMessageBox>
#include <QTranslator>
#include <QTimer>
#include <QLoggingCategory>

#include "debug.h"
#include "defines.h"
#include "singleapplication.h"

#include "ui/mainwindow.h"

#ifdef Q_OS_WIN
#include "Windows.h"
#endif

static void loadTranslator()
{
    QTranslator* translator = new QTranslator;
    if (translator->load(QLocale(), QString("amneziavpn"), QLatin1String("_"), QLatin1String(":/translations"))) {
        qApp->installTranslator(translator);
    }
}

int main(int argc, char *argv[])
{
    QLoggingCategory::setFilterRules(QStringLiteral("qtc.ssh=false"));

    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);

#ifdef Q_OS_WIN
    AllowSetForegroundWindow(ASFW_ANY);
#endif

    SingleApplication app(argc, argv, true, SingleApplication::Mode::User | SingleApplication::Mode::SecondaryNotification);

    if (!app.isPrimary()) {
        QTimer::singleShot(1000, &app, [&](){
            app.quit();
        });
        return app.exec();
    }
#ifdef Q_OS_WIN
    AllowSetForegroundWindow(0);
#endif

    loadTranslator();

    QFontDatabase::addApplicationFont(":/fonts/Lato-Black.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Lato-BlackItalic.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Lato-Bold.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Lato-BoldItalic.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Lato-Italic.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Lato-Light.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Lato-LightItalic.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Lato-Regular.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Lato-Thin.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Lato-ThinItalic.ttf");

    app.setApplicationName(APPLICATION_NAME);
    app.setOrganizationName(ORGANIZATION_NAME);
    app.setApplicationDisplayName(APPLICATION_NAME);

    QCommandLineParser parser;
    parser.setApplicationDescription(APPLICATION_NAME);
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption c_autostart {{"a", "autostart"}, "System autostart"};
    parser.addOption(c_autostart);

    parser.process(app);

    if (!Debug::init()) {
        qWarning() << "Initialization of debug subsystem failed";
    }

    app.setQuitOnLastWindowClosed(false);

    MainWindow mainWindow;

#ifdef Q_OS_WIN
    if (parser.isSet("a")) mainWindow.showOnStartup();
    else mainWindow.show();
#else
    mainWindow.showOnStartup();
#endif

    if (app.isPrimary()) {
        QObject::connect(&app, &SingleApplication::instanceStarted, &mainWindow, [&](){
            qDebug() << "Secondary instance started, showing this window instead";
            mainWindow.show();
            mainWindow.showNormal();
            mainWindow.raise();
        });
    }

    return app.exec();
}

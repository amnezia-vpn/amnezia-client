#include <QApplication>
#include <QFontDatabase>
#include <QCommandLineParser>
#include <QMessageBox>
#include <QTranslator>

#include "debug.h"
#include "defines.h"
#include "runguard.h"

#include "ui/mainwindow.h"

static void loadTranslator()
{
    QTranslator* translator = new QTranslator;
    if (translator->load(QLocale(), QString("amneziavpn"), QLatin1String("_"), QLatin1String(":/translations"))) {
        qApp->installTranslator(translator);
    }
}

int main(int argc, char *argv[])
{
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
    RunGuard::instance(APPLICATION_NAME).activate();

    QApplication app(argc, argv);
    loadTranslator();

    if (!RunGuard::instance().tryToRun()) {
        qDebug() << "Tried to run second instance. Exiting...";
        QMessageBox::information(NULL, QObject::tr("Notification"), QObject::tr("AmneziaVPN is already running."));
        return 0;
    }

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

    if (!Debug::init()) {
        qWarning() << "Initialization of debug subsystem failed";
    }

    QFont f("Lato Regular", 10);
    f.setStyleStrategy(QFont::PreferAntialias);
    app.setFont(f);

    app.setQuitOnLastWindowClosed(false);

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}

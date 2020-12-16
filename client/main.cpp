#include <QApplication>
#include <QFontDatabase>
#include <QCommandLineParser>
#include <QMessageBox>
#include <QTranslator>

#include "debug.h"
#include "defines.h"
#include "runguard.h"

#include "ui/mainwindow.h"


int main(int argc, char *argv[])
{
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
    RunGuard::instance(APPLICATION_NAME).activate();

    QApplication app(argc, argv);

    if (! RunGuard::instance().tryToRun()) {
        qDebug() << "Tried to run second instance. Exiting...";
        QMessageBox::information(NULL, QObject::tr("Notify"), QObject::tr("AmneziaVPN is already running."));
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

    {
        QTranslator *translator = new QTranslator;
        QLocale ru(QLocale("ru_RU"));
        QLocale::setDefault(ru);
        if (translator->load(QLocale(), "amneziavpn", ".", QLatin1String(":/translations"))) {
            bool ok = qApp->installTranslator(translator);
            qDebug().noquote() << "Main: Installing translator for locale" << ru.name() << ok;
        }
        else {
            qDebug().noquote() << "Main: Failed to install translator for locale" << ru.name();
        }
    }

    app.setApplicationName(APPLICATION_NAME);
    app.setOrganizationName(ORGANIZATION_NAME);
    app.setApplicationDisplayName(APPLICATION_NAME);

    //app.setQuitOnLastWindowClosed(false);

    QCommandLineParser parser;
    parser.setApplicationDescription(APPLICATION_NAME);
    parser.addHelpOption();
    parser.addVersionOption();

    if (!Debug::init()) {
        qCritical() << "Initialization of debug subsystem failed";
    }

    QFont f("Lato Regular", 10);
    f.setStyleStrategy(QFont::PreferAntialias);
    app.setFont(f);

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}

#ifndef AMNEZIA_APPLICATION_H
#define AMNEZIA_APPLICATION_H

#include <QApplication>
#include <QGuiApplication>

#include <QCommandLineParser>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "settings.h"

#include "ui/uilogic.h"
#include "configurators/vpn_configurator.h"

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
  #define AMNEZIA_BASE_CLASS SingleApplication
  #define QAPPLICATION_CLASS QGuiApplication
  #include "singleapplication.h"
  //#undef QAPPLICATION_CLASS
#else
  #define AMNEZIA_BASE_CLASS QApplication
#endif

class AmneziaApplication : public AMNEZIA_BASE_CLASS
{
    Q_OBJECT
public:
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    AmneziaApplication(int &argc, char *argv[]);
#else
    AmneziaApplication(int &argc, char *argv[], bool allowSecondary = false,
        SingleApplication::Options options = SingleApplication::User, int timeout = 1000, const QString &userData = {} );
#endif
    virtual ~AmneziaApplication();

    void init();
    void registerTypes();
    void loadFonts();
    void loadTranslator();
    void parseCommands();

private:
    QQmlApplicationEngine *m_engine;
    UiLogic *m_uiLogic;
    std::shared_ptr<Settings> m_settings;
    std::shared_ptr<VpnConfigurator> m_configurator;
    std::shared_ptr<ServerController> m_serverController;

    QTranslator* m_translator;
    QCommandLineParser m_parser;

};

#endif // AMNEZIA_APPLICATION_H

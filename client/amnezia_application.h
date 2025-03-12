#ifndef AMNEZIA_APPLICATION_H
#define AMNEZIA_APPLICATION_H

#include <QCommandLineParser>
#include <QNetworkAccessManager>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QThread>
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    #include <QGuiApplication>
#else
    #include <QApplication>
#endif
#include <QClipboard>

#include "core/controllers/coreController.h"
#include "settings.h"
#include "vpnconnection.h"

#define amnApp (static_cast<AmneziaApplication *>(QCoreApplication::instance()))

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    #define AMNEZIA_BASE_CLASS QGuiApplication
#else
    #define AMNEZIA_BASE_CLASS QApplication
#endif

class AmneziaApplication : public AMNEZIA_BASE_CLASS
{
    Q_OBJECT
public:
    AmneziaApplication(int &argc, char *argv[]);
    virtual ~AmneziaApplication();

    void init();
    void registerTypes();
    void loadFonts();
    bool parseCommands();

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    void startLocalServer();
#endif

    QQmlApplicationEngine *qmlEngine() const;
    QNetworkAccessManager *networkManager();
    QClipboard *getClipboard();

private:
    QQmlApplicationEngine *m_engine {};
    std::shared_ptr<Settings> m_settings;

    QScopedPointer<CoreController> m_coreController;

    QSharedPointer<ContainerProps> m_containerProps;
    QSharedPointer<ProtocolProps> m_protocolProps;

    QCommandLineParser m_parser;

    QSharedPointer<VpnConnection> m_vpnConnection;
    QThread m_vpnConnectionThread;

    QNetworkAccessManager *m_nam;
};

#endif // AMNEZIA_APPLICATION_H

#ifndef LOCALSERVICESCONTROLLER_H
#define LOCALSERVICESCONTROLLER_H

#include <QObject>

#ifdef Q_OS_WINDOWS
    #include "localServices/goodByeDpi.h"
#endif
#include "protocols/vpnprotocol.h"
#include "settings.h"
#include "ui/models/servers_model.h"

class LocalServicesController : public QObject
{
    Q_OBJECT

public:
    LocalServicesController(const QSharedPointer<ServersModel> &serversModel, const std::shared_ptr<Settings> &settings,
                            QObject *parent = nullptr);
    ~LocalServicesController();

    Q_PROPERTY(bool isGoodbyeDpiEnabled READ isGoodbyeDpiEnabled NOTIFY toggleGoodbyeDpiFinished)

public slots:
    void toggleGoodbyeDpi(bool enable);
    bool isGoodbyeDpiEnabled();

    void setGoodbyeDpiBlackListFile(const QString &file);
    QString getGoodbyeDpiBlackListFile();
    void resetGoodbyeDpiBlackListFile();

    void setGoodbyeDpiModset(const int modset);
    int getGoodbyeDpiModset();

    void start();
    void stop();

signals:
    void errorOccurred(ErrorCode errorCode);
    void toggleGoodbyeDpiFinished(const QString &message);
    void serviceStateChanged(Vpn::ConnectionState state);

private:
    std::shared_ptr<Settings> m_settings;
    QSharedPointer<ServersModel> m_serversModel;

#ifdef Q_OS_WINDOWS
    GoodByeDpi m_goodbyeDpiService;
#endif
    bool m_isGoodbyeDpiServiceEnabled = false;
#ifdef Q_OS_WINDOWS
    QString m_defaultBlackListFile = QCoreApplication::applicationDirPath() + "/goodbyedpi/blacklist.txt";
#else
    QString m_defaultBlackListFile;
#endif
};

#endif // LOCALSERVICESCONTROLLER_H

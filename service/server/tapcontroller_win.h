#ifndef TAPCONTROLLER_H
#define TAPCONTROLLER_H

#include <QObject>
#include <QString>
#include <QMap>

#define IPv6_DEBUG

//! The TapController class verifies Windows Tap Controller for existance on Windows platform.

class TapController
{
public:
    static TapController& Instance();
    static bool checkAndSetup();
    static QString getOpenVpnPath();


    bool checkInstaller();

    static QStringList getTapList();
    static bool enableTapAdapter(const QString &tapInstanceId);
    static bool disableTapAdapter(const QString &tapInstanceId);
private:
    explicit TapController();
    TapController(TapController const &) = delete;
    TapController& operator= (TapController const&) = delete;

    static bool checkDriver(const QString& tapInstanceId);
    static bool checkOpenVpn();
    static QString getTapInstallPath();
    static QString getTapDriverDir();
    static bool setupDriver();
    static bool setupDriverCertificate();
    static bool removeDriver(const QString& tapInstanceId);


};

#endif // TAPCONTROLLER_H

#ifndef OPENVPNCONFIGURATOR_H
#define OPENVPNCONFIGURATOR_H

#include <QObject>
#include <QProcessEnvironment>

#include "defs.h"
#include "servercontroller.h"


class OpenVpnConfigurator
{
public:

    struct ConnectionData {
        QString clientId;
        QString request; // certificate request
        QString privKey; // client private key
        QString clientCert; // client signed certificate
        QString caCert; // server certificate
        QString taKey; // tls-auth key
        QString host; // host ip
    };

    static QString genOpenVpnConfig(const ServerCredentials &credentials, ErrorCode *errorCode = nullptr);

private:
    static QString getRandomString(int len);
    static QString getEasyRsaShPath();

    static QProcessEnvironment prepareEnv();
    static ErrorCode initPKI(const QString &path);
    static ErrorCode genReq(const QString &path, const QString &clientId);

    static ConnectionData createCertRequest();

    static ConnectionData prepareOpenVpnConfig(const ServerCredentials &credentials, ErrorCode *errorCode = nullptr);

};

#endif // OPENVPNCONFIGURATOR_H

#include "shadowsocksInstaller.h"

#include <QJsonDocument>

#include "containers/containers_defs.h"
#include "protocols/protocols_defs.h"
#include "core/utils/sshSession.h"

using namespace amnezia;

ShadowSocksInstaller::ShadowSocksInstaller(QObject *parent)
    : InstallerBase(parent)
{
}

ErrorCode ShadowSocksInstaller::extractConfigFromContainer(DockerContainer container, const ServerCredentials &credentials,
                                                            SshSession* sshSession, QJsonObject &config)
{
    ErrorCode errorCode = ErrorCode::NoError;
    QString shadowsocksConfig = sshSession->getTextFileFromContainer(container, credentials,
                                                                           "/opt/amnezia/shadowsocks/ss-config.json", errorCode);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    QJsonDocument doc = QJsonDocument::fromJson(shadowsocksConfig.toUtf8());
    
    if (!doc.isNull() && doc.isObject()) {
        QJsonObject ssConfigObj = doc.object();
        QString cipher = ssConfigObj.value("method").toString();
        if (!cipher.isEmpty()) {
            auto mainProto = ContainerProps::defaultProtocol(container);
            QJsonObject containerConfig = config.value(ProtocolProps::protoToString(mainProto)).toObject();
            containerConfig[config_key::cipher] = cipher;
            config.insert(ProtocolProps::protoToString(mainProto), containerConfig);
        }
    }
    
    return ErrorCode::NoError;
}


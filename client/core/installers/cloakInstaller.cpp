#include "cloakInstaller.h"

#include <QJsonDocument>

#include "containers/containers_defs.h"
#include "protocols/protocols_defs.h"
#include "core/utils/sshSession.h"

using namespace amnezia;

CloakInstaller::CloakInstaller(QObject *parent)
    : InstallerBase(parent)
{
}

ErrorCode CloakInstaller::extractConfigFromContainer(DockerContainer container, const ServerCredentials &credentials,
                                                      SshSession* sshSession, QJsonObject &config)
{
    ErrorCode errorCode = ErrorCode::NoError;
    QString cloakConfig = sshSession->getTextFileFromContainer(container, credentials,
                                                                     "/opt/amnezia/cloak/ck-config.json", errorCode);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    QJsonDocument doc = QJsonDocument::fromJson(cloakConfig.toUtf8());
    
    if (!doc.isNull() && doc.isObject()) {
        QJsonObject cloakConfigObj = doc.object();
        
        QString site = cloakConfigObj.value("RedirAddr").toString();
        if (!site.isEmpty()) {
            auto mainProto = ContainerProps::defaultProtocol(container);
            QJsonObject containerConfig = config.value(ProtocolProps::protoToString(mainProto)).toObject();
            containerConfig[config_key::site] = site;
            config.insert(ProtocolProps::protoToString(mainProto), containerConfig);
        }
    }
    
    return ErrorCode::NoError;
}


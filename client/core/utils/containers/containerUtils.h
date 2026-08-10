#ifndef CONTAINERUTILS_H
#define CONTAINERUTILS_H

#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QJsonObject>

#include "core/utils/containerEnum.h"
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"

namespace amnezia
{
    namespace ContainerUtils
    {
        DockerContainer containerFromString(const QString &container);
        QString containerToString(DockerContainer container);
        QString containerTypeToString(DockerContainer c);
        QString containerTypeToProtocolString(DockerContainer c);

        QList<DockerContainer> allContainers();

        QMap<DockerContainer, QString> containerHumanNames();
        QMap<DockerContainer, QString> containerDescriptions();
        QMap<DockerContainer, QString> containerDetailedDescriptions();

        ServiceType containerService(DockerContainer c);

        // binding between Docker container and main protocol of given container
        // it may be changed fot future containers :)
        Proto defaultProtocol(DockerContainer c);

        bool isSupportedByCurrentPlatform(DockerContainer c);
        QStringList fixedPortsForContainer(DockerContainer c);

        bool isEasySetupContainer(DockerContainer container);
        QString easySetupHeader(DockerContainer container);
        QString easySetupDescription(DockerContainer container);
        int easySetupOrder(DockerContainer container);

        bool isShareable(DockerContainer container);

        bool isAwgContainer(DockerContainer container);

        bool isUnsupportedContainer(DockerContainer container);

        QJsonObject getProtocolConfigFromContainer(const Proto protocol, const QJsonObject &containerConfig);

        int installPageOrder(DockerContainer container);
    }
}

#endif // CONTAINERUTILS_H



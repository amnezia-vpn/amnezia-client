#ifndef CONTAINERS_DEFS_H
#define CONTAINERS_DEFS_H

#include <QObject>
#include <QQmlEngine>

#include "../protocols/protocols_defs.h"

using namespace fblink;

namespace fblink
{

    namespace ContainerEnumNS
    {
        Q_NAMESPACE
        enum DockerContainer {
            None = 0,
            Awg,
            Awg2,
            WireGuard,
            OpenVpn,
            Cloak,
            ShadowSocks,
            Ipsec,
            Xray,
            SSXray,

            // non-vpn
            TorWebSite,
            Dns,
            Sftp,
            Socks5Proxy
        };
        Q_ENUM_NS(DockerContainer)
    } // namespace ContainerEnumNS

    using namespace ContainerEnumNS;
    using namespace ProtocolEnumNS;

    class ContainerProps : public QObject
    {
        Q_OBJECT

    public:
        Q_INVOKABLE static fblink::DockerContainer containerFromString(const QString &container);
        Q_INVOKABLE static QString containerToString(fblink::DockerContainer container);
        Q_INVOKABLE static QString containerTypeToString(fblink::DockerContainer c);
        Q_INVOKABLE static QString containerTypeToProtocolString(fblink::DockerContainer c);

        Q_INVOKABLE static QList<fblink::DockerContainer> allContainers();

        Q_INVOKABLE static QMap<fblink::DockerContainer, QString> containerHumanNames();
        Q_INVOKABLE static QMap<fblink::DockerContainer, QString> containerDescriptions();
        Q_INVOKABLE static QMap<fblink::DockerContainer, QString> containerDetailedDescriptions();

        // these protocols will be displayed in container settings
        Q_INVOKABLE static QVector<fblink::Proto> protocolsForContainer(fblink::DockerContainer container);

        Q_INVOKABLE static fblink::ServiceType containerService(fblink::DockerContainer c);

        // binding between Docker container and main protocol of given container
        // it may be changed fot future containers :)
        Q_INVOKABLE static fblink::Proto defaultProtocol(fblink::DockerContainer c);

        Q_INVOKABLE static bool isSupportedByCurrentPlatform(fblink::DockerContainer c);
        Q_INVOKABLE static QStringList fixedPortsForContainer(fblink::DockerContainer c);

        static bool isEasySetupContainer(fblink::DockerContainer container);
        static QString easySetupHeader(fblink::DockerContainer container);
        static QString easySetupDescription(fblink::DockerContainer container);
        static int easySetupOrder(fblink::DockerContainer container);

        static bool isShareable(fblink::DockerContainer container);

        static bool isAwgContainer(fblink::DockerContainer container);


        static QJsonObject getProtocolConfigFromContainer(const fblink::Proto protocol, const QJsonObject &containerConfig);

        static int installPageOrder(fblink::DockerContainer container);
    };

    static void declareQmlContainerEnum()
    {
        qmlRegisterUncreatableMetaObject(ContainerEnumNS::staticMetaObject, "ContainerEnum", 1, 0, "ContainerEnum",
                                         "Error: only enums");
    }

} // namespace fblink

QDebug operator<<(QDebug debug, const fblink::DockerContainer &c);

#endif // CONTAINERS_DEFS_H

#include "importController.h"

#include <QFile>

namespace {
    enum class ConfigTypes {
        Amnezia,
        OpenVpn,
        WireGuard
    };

    ConfigTypes checkConfigFormat(const QString &config)
    {
        const QString openVpnConfigPatternCli = "client";
        const QString openVpnConfigPatternProto1 = "proto tcp";
        const QString openVpnConfigPatternProto2 = "proto udp";
        const QString openVpnConfigPatternDriver1 = "dev tun";
        const QString openVpnConfigPatternDriver2 = "dev tap";

        const QString wireguardConfigPatternSectionInterface = "[Interface]";
        const QString wireguardConfigPatternSectionPeer = "[Peer]";

        if (config.contains(openVpnConfigPatternCli) &&
            (config.contains(openVpnConfigPatternProto1) || config.contains(openVpnConfigPatternProto2)) &&
            (config.contains(openVpnConfigPatternDriver1) || config.contains(openVpnConfigPatternDriver2))) {
            return ConfigTypes::OpenVpn;
        } else if (config.contains(wireguardConfigPatternSectionInterface) && config.contains(wireguardConfigPatternSectionPeer)) {
            return ConfigTypes::WireGuard;
        }
        return ConfigTypes::Amnezia;
    }
}

ImportController::ImportController(const QSharedPointer<ServersModel> &serversModel,
                                   const QSharedPointer<ContainersModel> &containersModel,
                                   const std::shared_ptr<Settings> &settings,
                                   QObject *parent) : QObject(parent), m_serversModel(serversModel), m_containersModel(containersModel),  m_settings(settings)
{

}

bool ImportController::importFromFile(const QUrl &fileUrl)
{
    QFile file(fileUrl.toLocalFile());
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();

        auto configFormat = checkConfigFormat(data);
        if (configFormat == ConfigTypes::OpenVpn) {
            return importOpenVpnConfig(data);
        } else if (configFormat == ConfigTypes::WireGuard) {
            return importWireGuardConfig(data);
        } else {
            return importAmneziaConfig(data);
        }
    }
    return false;
}

bool ImportController::import(const QJsonObject &config)
{
    ServerCredentials credentials;
    credentials.hostName = config.value(config_key::hostName).toString();
    credentials.port = config.value(config_key::port).toInt();
    credentials.userName = config.value(config_key::userName).toString();
    credentials.secretData = config.value(config_key::password).toString();

    if (credentials.isValid() || config.contains(config_key::containers)) {
        m_settings->addServer(config);

        if (config.value(config_key::containers).toArray().isEmpty()) {
            m_settings->setDefaultServer(m_settings->serversCount() - 1);
        }

        emit importFinished();
    } else {
        qDebug() << "Failed to import profile";
        qDebug().noquote() << QJsonDocument(config).toJson();
        return false;
    }

    return true;
}

bool ImportController::importAmneziaConfig(QString data)
{
    data.replace("vpn://", "");
    QByteArray ba = QByteArray::fromBase64(data.toUtf8(), QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);

    QByteArray ba_uncompressed = qUncompress(ba);
    if (!ba_uncompressed.isEmpty()) {
        ba = ba_uncompressed;
    }

    QJsonObject config;
    config = QJsonDocument::fromJson(ba).object();
    if (!config.isEmpty()) {
        return import(config);
    }

    return false;
}

//bool ImportController::importConnectionFromQr(const QByteArray &data)
//{
//    QJsonObject dataObj = QJsonDocument::fromJson(data).object();
//    if (!dataObj.isEmpty()) {
//        return importConnection(dataObj);
//    }

//    QByteArray ba_uncompressed = qUncompress(data);
//    if (!ba_uncompressed.isEmpty()) {
//        return importConnection(QJsonDocument::fromJson(ba_uncompressed).object());
//    }

//    return false;
//}

bool ImportController::importOpenVpnConfig(const QString &data)
{
    QJsonObject openVpnConfig;
    openVpnConfig[config_key::config] = data;

    QJsonObject lastConfig;
    lastConfig[config_key::last_config] = QString(QJsonDocument(openVpnConfig).toJson());
    lastConfig[config_key::isThirdPartyConfig] = true;

    QJsonObject containers;
    containers.insert(config_key::container, QJsonValue("amnezia-openvpn"));
    containers.insert(config_key::openvpn, QJsonValue(lastConfig));

    QJsonArray arr;
    arr.push_back(containers);

    QString hostName;
    const static QRegularExpression hostNameRegExp("remote (.*) [0-9]*");
    QRegularExpressionMatch hostNameMatch = hostNameRegExp.match(data);
    if (hostNameMatch.hasMatch()) {
        hostName = hostNameMatch.captured(1);
    }

    QJsonObject config;
    config[config_key::containers] = arr;
    config[config_key::defaultContainer] = "amnezia-openvpn";
    config[config_key::description] = m_settings->nextAvailableServerName();


    const static QRegularExpression dnsRegExp("dhcp-option DNS (\\b\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\b)");
    QRegularExpressionMatchIterator dnsMatch = dnsRegExp.globalMatch(data);
    if (dnsMatch.hasNext()) {
        config[config_key::dns1] = dnsMatch.next().captured(1);
    }
    if (dnsMatch.hasNext()) {
        config[config_key::dns2] = dnsMatch.next().captured(1);
    }

    config[config_key::hostName] = hostName;

    return import(config);
}

bool ImportController::importWireGuardConfig(const QString &data)
{
    QJsonObject lastConfig;
    lastConfig[config_key::config] = data;

    const static QRegularExpression hostNameAndPortRegExp("Endpoint = (.*):([0-9]*)");
    QRegularExpressionMatch hostNameAndPortMatch = hostNameAndPortRegExp.match(data);
    QString hostName;
    QString port;
    if (hostNameAndPortMatch.hasMatch()) {
        hostName = hostNameAndPortMatch.captured(1);
        port = hostNameAndPortMatch.captured(2);
    }

    QJsonObject wireguardConfig;
    wireguardConfig[config_key::last_config] = QString(QJsonDocument(lastConfig).toJson());
    wireguardConfig[config_key::isThirdPartyConfig] = true;
    wireguardConfig[config_key::port] = port;
    wireguardConfig[config_key::transport_proto] = "udp";

    QJsonObject containers;
    containers.insert(config_key::container, QJsonValue("amnezia-wireguard"));
    containers.insert(config_key::wireguard, QJsonValue(wireguardConfig));

    QJsonArray arr;
    arr.push_back(containers);

    QJsonObject config;
    config[config_key::containers] = arr;
    config[config_key::defaultContainer] = "amnezia-wireguard";
    config[config_key::description] = m_settings->nextAvailableServerName();

    const static QRegularExpression dnsRegExp("DNS = (\\b\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\b).*(\\b\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\b)");
    QRegularExpressionMatch dnsMatch = dnsRegExp.match(data);
    if (dnsMatch.hasMatch()) {
        config[config_key::dns1] = dnsMatch.captured(1);
        config[config_key::dns2] = dnsMatch.captured(2);
    }

    config[config_key::hostName] = hostName;

    return import(config);
}

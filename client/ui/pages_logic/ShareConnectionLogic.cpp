#include <QBuffer>
#include <QImage>
#include <QDataStream>
#include <QMessageBox>

#include "qrcodegen.hpp"

#include "ShareConnectionLogic.h"

#include "configurators/cloak_configurator.h"
#include "configurators/vpn_configurator.h"
#include "configurators/openvpn_configurator.h"
#include "configurators/shadowsocks_configurator.h"
#include "configurators/wireguard_configurator.h"
#include "configurators/ikev2_configurator.h"
#include "configurators/ssh_configurator.h"

#include "defines.h"
#include "core/defs.h"
#include "core/errorstrings.h"
#include "core/servercontroller.h"
#include <functional>

#include "../uilogic.h"

#ifdef __linux__
    #include <math.h>
#endif

using namespace qrcodegen;

ShareConnectionLogic::ShareConnectionLogic(UiLogic *logic, QObject *parent):
    PageLogicBase(logic, parent),
    m_textEditShareOpenVpnCodeText{},
    m_lineEditShareShadowSocksStringText{},
    m_shareShadowSocksQrCodeText{},
    m_textEditShareCloakText{},
    m_textEditShareAmneziaCodeText{}
{
}

void ShareConnectionLogic::onUpdatePage()
{
    set_textEditShareAmneziaCodeText(tr(""));
    set_shareAmneziaQrCodeTextSeries({});
    set_shareAmneziaQrCodeTextSeriesLength(0);

    set_textEditShareOpenVpnCodeText("");

    set_shareShadowSocksQrCodeText("");
    set_textEditShareShadowSocksText("");
    set_lineEditShareShadowSocksStringText("");

    set_textEditShareCloakText("");

    set_textEditShareWireGuardCodeText("");
    set_shareWireGuardQrCodeText("");

    set_textEditShareIkev2CertText("");
    set_textEditShareIkev2MobileConfigText("");
    set_textEditShareIkev2StrongSwanConfigText("");
}

void ShareConnectionLogic::onPushButtonShareAmneziaGenerateClicked()
{
    set_textEditShareAmneziaCodeText("");
    set_shareAmneziaQrCodeTextSeries({});
    set_shareAmneziaQrCodeTextSeriesLength(0);

    QJsonObject serverConfig;
    int serverIndex = uiLogic()->selectedServerIndex;
    DockerContainer container = uiLogic()->selectedDockerContainer;

    // Full access
    if (shareFullAccess()) {
        serverConfig = m_settings->server(serverIndex);
    }
    // Container share
    else {
        ServerCredentials credentials = m_settings->serverCredentials(serverIndex);
        QJsonObject containerConfig = m_settings->containerConfig(serverIndex, container);
        containerConfig.insert(config_key::container, ContainerProps::containerToString(container));

        ErrorCode e = ErrorCode::NoError;
        for (Proto p: ContainerProps::protocolsForContainer(container)) {
            QJsonObject protoConfig = m_settings->protocolConfig(serverIndex, container, p);

            QString cfg = m_configurator->genVpnProtocolConfig(credentials, container, containerConfig, p, &e);
            if (e) {
                cfg = "Error generating config";
                break;
            }
            protoConfig.insert(config_key::last_config, cfg);
            containerConfig.insert(ProtocolProps::protoToString(p), protoConfig);
        }

        QByteArray ba;
        if (!e) {
            serverConfig = m_settings->server(serverIndex);
            serverConfig.remove(config_key::userName);
            serverConfig.remove(config_key::password);
            serverConfig.remove(config_key::port);
            serverConfig.insert(config_key::containers, QJsonArray {containerConfig});
            serverConfig.insert(config_key::defaultContainer, ContainerProps::containerToString(container));

            auto dns = m_configurator->getDnsForConfig(serverIndex);
            serverConfig.insert(config_key::dns1, dns.first);
            serverConfig.insert(config_key::dns2, dns.second);

        }
        else {
            set_textEditShareAmneziaCodeText(tr("Error while generating connection profile"));
            return;
        }
    }

    QByteArray ba = QJsonDocument(serverConfig).toJson();
    ba = qCompress(ba, 8);
    QString code = QString("vpn://%1").arg(QString(ba.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)));
    set_textEditShareAmneziaCodeText(code);


    QList<QString> qrChunks = genQrCodeImageSeries(ba);
    set_shareAmneziaQrCodeTextSeries(qrChunks);
    set_shareAmneziaQrCodeTextSeriesLength(qrChunks.size());
}

void ShareConnectionLogic::onPushButtonShareOpenVpnGenerateClicked()
{
    int serverIndex = uiLogic()->selectedServerIndex;
    DockerContainer container = uiLogic()->selectedDockerContainer;
    ServerCredentials credentials = m_settings->serverCredentials(serverIndex);

    const QJsonObject &containerConfig = m_settings->containerConfig(serverIndex, container);

    ErrorCode e = ErrorCode::NoError;
    QString cfg = m_configurator->openVpnConfigurator->genOpenVpnConfig(credentials, container, containerConfig, &e);
    cfg = m_configurator->processConfigWithExportSettings(serverIndex, container, Proto::OpenVpn, cfg);

    set_textEditShareOpenVpnCodeText(QJsonDocument::fromJson(cfg.toUtf8()).object()[config_key::config].toString());
}

void ShareConnectionLogic::onPushButtonShareShadowSocksGenerateClicked()
{
    int serverIndex = uiLogic()->selectedServerIndex;
    DockerContainer container = uiLogic()->selectedDockerContainer;
    ServerCredentials credentials = m_settings->serverCredentials(serverIndex);

    QJsonObject protoConfig = m_settings->protocolConfig(serverIndex, container, Proto::ShadowSocks);
    QString cfg = protoConfig.value(config_key::last_config).toString();

    if (cfg.isEmpty()) {
        const QJsonObject &containerConfig = m_settings->containerConfig(serverIndex, container);

        ErrorCode e = ErrorCode::NoError;
        cfg = m_configurator->shadowSocksConfigurator->genShadowSocksConfig(credentials, container, containerConfig, &e);
    }

    QJsonObject ssConfig = QJsonDocument::fromJson(cfg.toUtf8()).object();

    QString ssString = QString("%1:%2@%3:%4")
            .arg(ssConfig.value("method").toString())
            .arg(ssConfig.value("password").toString())
            .arg(ssConfig.value("server").toString())
            .arg(ssConfig.value("server_port").toString());

    ssString = "ss://" + ssString.toUtf8().toBase64();
    set_lineEditShareShadowSocksStringText(ssString);

    QrCode qr = QrCode::encodeText(ssString.toUtf8(), QrCode::Ecc::LOW);
    QString svg = QString::fromStdString(toSvgString(qr, 0));

    set_shareShadowSocksQrCodeText(svgToBase64(svg));

    QString humanString = QString("Server: %3\n"
                                  "Port: %4\n"
                                  "Encryption: %1\n"
                                  "Password: %2")
            .arg(ssConfig.value("method").toString())
            .arg(ssConfig.value("password").toString())
            .arg(ssConfig.value("server").toString())
            .arg(ssConfig.value("server_port").toString());

    set_textEditShareShadowSocksText(humanString);
}

void ShareConnectionLogic::onPushButtonShareCloakGenerateClicked()
{
    int serverIndex = uiLogic()->selectedServerIndex;
    DockerContainer container = uiLogic()->selectedDockerContainer;
    ServerCredentials credentials = m_settings->serverCredentials(serverIndex);

    QJsonObject protoConfig = m_settings->protocolConfig(serverIndex, container, Proto::Cloak);
    QString cfg = protoConfig.value(config_key::last_config).toString();

    if (cfg.isEmpty()) {
        const QJsonObject &containerConfig = m_settings->containerConfig(serverIndex, container);

        ErrorCode e = ErrorCode::NoError;
        cfg = m_configurator->cloakConfigurator->genCloakConfig(credentials, container, containerConfig, &e);
    }

    QJsonObject cloakConfig = QJsonDocument::fromJson(cfg.toUtf8()).object();
    cloakConfig.remove(config_key::transport_proto);
    cloakConfig.insert("ProxyMethod", "shadowsocks");

    set_textEditShareCloakText(QJsonDocument(cloakConfig).toJson());
}

void ShareConnectionLogic::onPushButtonShareWireGuardGenerateClicked()
{
    int serverIndex = uiLogic()->selectedServerIndex;
    DockerContainer container = uiLogic()->selectedDockerContainer;
    ServerCredentials credentials = m_settings->serverCredentials(serverIndex);

    const QJsonObject &containerConfig = m_settings->containerConfig(serverIndex, container);

    ErrorCode e = ErrorCode::NoError;
    QString cfg = m_configurator->wireguardConfigurator->genWireguardConfig(credentials, container, containerConfig, &e);
    if (e) {
        QMessageBox::warning(nullptr, APPLICATION_NAME,
                             tr("Error occurred while configuring server.") + "\n" +
                             errorString(e));
        return;
    }
    cfg = m_configurator->processConfigWithExportSettings(serverIndex, container, Proto::WireGuard, cfg);
    cfg = QJsonDocument::fromJson(cfg.toUtf8()).object()[config_key::config].toString();

    set_textEditShareWireGuardCodeText(cfg);

    QrCode qr = QrCode::encodeText(cfg.toUtf8(), QrCode::Ecc::LOW);
    QString svg = QString::fromStdString(toSvgString(qr, 0));

    set_shareWireGuardQrCodeText(svgToBase64(svg));
}

void ShareConnectionLogic::onPushButtonShareIkev2GenerateClicked()
{
    int serverIndex = uiLogic()->selectedServerIndex;
    DockerContainer container = uiLogic()->selectedDockerContainer;
    ServerCredentials credentials = m_settings->serverCredentials(serverIndex);

    Ikev2Configurator::ConnectionData connData = m_configurator->ikev2Configurator->prepareIkev2Config(credentials, container);

    QString cfg = m_configurator->ikev2Configurator->genIkev2Config(connData);
    cfg = m_configurator->processConfigWithExportSettings(serverIndex, container, Proto::Ikev2, cfg);
    cfg = QJsonDocument::fromJson(cfg.toUtf8()).object()[config_key::cert].toString();

    set_textEditShareIkev2CertText(cfg);

    QString mobileCfg = m_configurator->ikev2Configurator->genMobileConfig(connData);
    set_textEditShareIkev2MobileConfigText(mobileCfg);

    QString strongSwanCfg = m_configurator->ikev2Configurator->genStrongSwanConfig(connData);
    set_textEditShareIkev2StrongSwanConfigText(strongSwanCfg);

}


void ShareConnectionLogic::updateSharingPage(int serverIndex, DockerContainer container)
{
    uiLogic()->selectedDockerContainer = container;
    uiLogic()->selectedServerIndex = serverIndex;
    set_shareFullAccess(container == DockerContainer::None);

    m_shareAmneziaQrCodeTextSeries.clear();
    set_shareAmneziaQrCodeTextSeriesLength(0);
}

QList<QString> ShareConnectionLogic::genQrCodeImageSeries(const QByteArray &data)
{
    double k = 850;

    quint8 chunksCount = std::ceil(data.size() / k);
    QList<QString> chunks;
    for (int i = 0; i < data.size(); i = i + k) {
        QByteArray chunk;
        QDataStream s(&chunk, QIODevice::WriteOnly);
        s << amnezia::qrMagicCode << chunksCount << (quint8)std::round(i/k) << data.mid(i, k);

        QByteArray ba = chunk.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);

        QrCode qr = QrCode::encodeText(ba, QrCode::Ecc::LOW);
        QString svg = QString::fromStdString(toSvgString(qr, 0));
        chunks.append(svgToBase64(svg));
    }

    return chunks;
}

QString ShareConnectionLogic::svgToBase64(const QString &image)
{
    return "data:image/svg;base64," + QString::fromLatin1(image.toUtf8().toBase64().data());
}

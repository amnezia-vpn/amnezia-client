#include <QApplication>
#include <QBuffer>
#include <QClipboard>
#include <QFileDialog>
#include <QTimer>
#include <QSaveFile>
#include <QStandardPaths>
#include <QImage>

#include "ShareConnectionLogic.h"

#include "configurators/cloak_configurator.h"
#include "configurators/vpn_configurator.h"
#include "configurators/openvpn_configurator.h"
#include "configurators/shadowsocks_configurator.h"
#include "configurators/wireguard_configurator.h"
#include "configurators/ikev2_configurator.h"
#include "configurators/ssh_configurator.h"

#include "defines.h"
#include <functional>

#include "../uilogic.h"

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
    set_shareAmneziaQrCodeText("");

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
    set_shareAmneziaQrCodeText("");

    QJsonObject serverConfig;
    // Full access
    if (shareFullAccess()) {
        serverConfig = m_settings.server(uiLogic()->selectedServerIndex);
    }
    // Container share
    else {
        ServerCredentials credentials = m_settings.serverCredentials(uiLogic()->selectedServerIndex);
        QJsonObject containerConfig = m_settings.containerConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer);
        containerConfig.insert(config_key::container, ContainerProps::containerToString(uiLogic()->selectedDockerContainer));

        ErrorCode e = ErrorCode::NoError;
        for (Proto p: ContainerProps::protocolsForContainer(uiLogic()->selectedDockerContainer)) {
            QJsonObject protoConfig = m_settings.protocolConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer, p);

            QString cfg = VpnConfigurator::genVpnProtocolConfig(credentials, uiLogic()->selectedDockerContainer, containerConfig, p, &e);
            if (e) {
                cfg = "Error generating config";
                break;
            }
            protoConfig.insert(config_key::last_config, cfg);
            containerConfig.insert(ProtocolProps::protoToString(p), protoConfig);
        }

        QByteArray ba;
        if (!e) {
            serverConfig = m_settings.server(uiLogic()->selectedServerIndex);
            serverConfig.remove(config_key::userName);
            serverConfig.remove(config_key::password);
            serverConfig.remove(config_key::port);
            serverConfig.insert(config_key::containers, QJsonArray {containerConfig});
            serverConfig.insert(config_key::defaultContainer, ContainerProps::containerToString(uiLogic()->selectedDockerContainer));
        }
        else {
            set_textEditShareAmneziaCodeText(tr("Error while generating connection profile"));
            return;
        }
    }

    QByteArray ba = QJsonDocument(serverConfig).toBinaryData();
    ba = qCompress(ba, 8);
    QString code = QString("vpn://%1").arg(QString(ba.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)));
    set_textEditShareAmneziaCodeText(code);

    if (ba.size() < 2900) {
        QImage qr = updateQRCodeImage(ba.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
        set_shareAmneziaQrCodeText(imageToBase64(qr));
    }
}

void ShareConnectionLogic::onPushButtonShareOpenVpnGenerateClicked()
{
    ServerCredentials credentials = m_settings.serverCredentials(uiLogic()->selectedServerIndex);
    const QJsonObject &containerConfig = m_settings.containerConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer);

    ErrorCode e = ErrorCode::NoError;
    QString cfg = OpenVpnConfigurator::genOpenVpnConfig(credentials, uiLogic()->selectedDockerContainer, containerConfig, &e);
    cfg = VpnConfigurator::processConfigWithExportSettings(uiLogic()->selectedDockerContainer, Proto::OpenVpn, cfg);

    set_textEditShareOpenVpnCodeText(QJsonDocument::fromJson(cfg.toUtf8()).object()[config_key::config].toString());
}

void ShareConnectionLogic::onPushButtonShareShadowSocksGenerateClicked()
{
    int serverIndex = uiLogic()->selectedServerIndex;
    DockerContainer container = uiLogic()->selectedDockerContainer;
    ServerCredentials credentials = m_settings.serverCredentials(serverIndex);

    QJsonObject protoConfig = m_settings.protocolConfig(serverIndex, container, Proto::ShadowSocks);
    QString cfg = protoConfig.value(config_key::last_config).toString();

    if (cfg.isEmpty()) {
        const QJsonObject &containerConfig = m_settings.containerConfig(serverIndex, container);

        ErrorCode e = ErrorCode::NoError;
        cfg = ShadowSocksConfigurator::genShadowSocksConfig(credentials, container, containerConfig, &e);
    }

    QJsonObject ssConfig = QJsonDocument::fromJson(cfg.toUtf8()).object();

    QString ssString = QString("%1:%2@%3:%4")
            .arg(ssConfig.value("method").toString())
            .arg(ssConfig.value("password").toString())
            .arg(ssConfig.value("server").toString())
            .arg(ssConfig.value("server_port").toString());

    ssString = "ss://" + ssString.toUtf8().toBase64();
    set_lineEditShareShadowSocksStringText(ssString);

    QImage qr = updateQRCodeImage(ssString.toUtf8());
    set_shareShadowSocksQrCodeText(imageToBase64(qr));

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
    ServerCredentials credentials = m_settings.serverCredentials(serverIndex);

    QJsonObject protoConfig = m_settings.protocolConfig(serverIndex, container, Proto::Cloak);
    QString cfg = protoConfig.value(config_key::last_config).toString();

    if (cfg.isEmpty()) {
        const QJsonObject &containerConfig = m_settings.containerConfig(serverIndex, container);

        ErrorCode e = ErrorCode::NoError;
        cfg = CloakConfigurator::genCloakConfig(credentials, container, containerConfig, &e);
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
    ServerCredentials credentials = m_settings.serverCredentials(serverIndex);

    const QJsonObject &containerConfig = m_settings.containerConfig(serverIndex, container);

    ErrorCode e = ErrorCode::NoError;
    QString cfg = WireguardConfigurator::genWireguardConfig(credentials, container, containerConfig, &e);
    cfg = VpnConfigurator::processConfigWithExportSettings(container, Proto::WireGuard, cfg);
    cfg = QJsonDocument::fromJson(cfg.toUtf8()).object()[config_key::config].toString();

    set_textEditShareWireGuardCodeText(cfg);

    QImage qr = updateQRCodeImage(cfg.toUtf8());
    set_shareWireGuardQrCodeText(imageToBase64(qr));
}

void ShareConnectionLogic::onPushButtonShareIkev2GenerateClicked()
{
    int serverIndex = uiLogic()->selectedServerIndex;
    DockerContainer container = uiLogic()->selectedDockerContainer;
    ServerCredentials credentials = m_settings.serverCredentials(serverIndex);

    //const QJsonObject &containerConfig = m_settings.containerConfig(serverIndex, container);

    Ikev2Configurator::ConnectionData connData = Ikev2Configurator::prepareIkev2Config(credentials, container);

    QString cfg = Ikev2Configurator::genIkev2Config(connData);
    cfg = VpnConfigurator::processConfigWithExportSettings(container, Proto::Ikev2, cfg);
    cfg = QJsonDocument::fromJson(cfg.toUtf8()).object()[config_key::cert].toString();

    set_textEditShareIkev2CertText(cfg);

    QString mobileCfg = Ikev2Configurator::genMobileConfig(connData);
    set_textEditShareIkev2MobileConfigText(mobileCfg);

    QString strongSwanCfg = Ikev2Configurator::genStrongSwanConfig(connData);
    set_textEditShareIkev2StrongSwanConfigText(strongSwanCfg);

}


void ShareConnectionLogic::updateSharingPage(int serverIndex, DockerContainer container)
{
    uiLogic()->selectedDockerContainer = container;
    uiLogic()->selectedServerIndex = serverIndex;
    set_shareFullAccess(container == DockerContainer::None);
}

QImage ShareConnectionLogic::updateQRCodeImage(const QByteArray &data)
{
    int levelIndex = 1;
    int versionIndex = 0;
    bool bExtent = true;
    int maskIndex = -1;

    m_qrEncode.EncodeData( levelIndex, versionIndex, bExtent, maskIndex, data.data() );

    int qrImageSize = m_qrEncode.m_nSymbleSize;

    int encodeImageSize = qrImageSize + ( QR_MARGIN * 2 );
    QImage encodeImage( encodeImageSize, encodeImageSize, QImage::Format_Mono );

    encodeImage.fill( 1 );

    for ( int i = 0; i < qrImageSize; i++ )
        for ( int j = 0; j < qrImageSize; j++ )
            if ( m_qrEncode.m_byModuleData[i][j] )
                encodeImage.setPixel( i + QR_MARGIN, j + QR_MARGIN, 0 );

    return encodeImage;
}

QString ShareConnectionLogic::imageToBase64(const QImage &image)
{
    QByteArray ba;
    QBuffer bu(&ba);
    bu.open(QIODevice::WriteOnly);
    image.save(&bu, "PNG");
    return "data:image/png;base64," + QString::fromLatin1(ba.toBase64().data());
}

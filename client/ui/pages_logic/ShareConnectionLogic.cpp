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
#include "configurators/ssh_configurator.h"

#include "defines.h"
#include <functional>

#include "../uilogic.h"

ShareConnectionLogic::ShareConnectionLogic(UiLogic *logic, QObject *parent):
    PageLogicBase(logic, parent),
    m_textEditShareOpenVpnCodeText{},
    m_lineEditShareShadowSocksStringText{},
    m_shareShadowSocksQrCodeText{},
    m_labelShareShadowSocksServerText{},
    m_labelShareShadowSocksPortText{},
    m_labelShareShadowSocksMethodText{},
    m_labelShareShadowSocksPasswordText{},
    m_textEditShareCloakText{},
    m_textEditShareAmneziaCodeText{},
    m_pushButtonShareOpenVpnGenerateText{tr("Generate config")}
{
    // TODO consider move to Component.onCompleted
    //updateSharingPage(uiLogic()->selectedServerIndex, m_settings.serverCredentials(uiLogic()->selectedServerIndex), uiLogic()->selectedDockerContainer);
}

void ShareConnectionLogic::onUpdatePage()
{
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
        for (Protocol p: ContainerProps::protocolsForContainer(uiLogic()->selectedDockerContainer)) {
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

    if (ba.size() < 1024) {
        QImage qr = updateQRCodeImage(ba.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
        set_shareAmneziaQrCodeText(imageToBase64(qr));
    }
}

void ShareConnectionLogic::onPushButtonShareOpenVpnGenerateClicked()
{
    set_pushButtonShareOpenVpnGenerateText(tr("Generating..."));

    ServerCredentials credentials = m_settings.serverCredentials(uiLogic()->selectedServerIndex);
    const QJsonObject &containerConfig = m_settings.containerConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer);

    ErrorCode e = ErrorCode::NoError;
    QString cfg = OpenVpnConfigurator::genOpenVpnConfig(credentials, uiLogic()->selectedDockerContainer, containerConfig, &e);
    cfg = OpenVpnConfigurator::processConfigWithExportSettings(cfg);

    set_textEditShareOpenVpnCodeText(QJsonDocument::fromJson(cfg.toUtf8()).object()[config_key::config].toString());

    set_pushButtonShareOpenVpnGenerateText(tr("Generate config"));
}

void ShareConnectionLogic::updateSharingPage(int serverIndex, const ServerCredentials &credentials,
                                DockerContainer container)
{
    uiLogic()->selectedDockerContainer = container;
    uiLogic()->selectedServerIndex = serverIndex;
    set_shareFullAccess(container == DockerContainer::None);

    if (! shareFullAccess()) {
        for (Protocol p : ContainerProps::protocolsForContainer(container)) {
            if (p == Protocol::ShadowSocks) {
                QJsonObject protoConfig = m_settings.protocolConfig(serverIndex, container, Protocol::ShadowSocks);
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

                set_labelShareShadowSocksServerText(ssConfig.value("server").toString());
                set_labelShareShadowSocksPortText(ssConfig.value("server_port").toString());
                set_labelShareShadowSocksMethodText(ssConfig.value("method").toString());
                set_labelShareShadowSocksPasswordText(ssConfig.value("password").toString());

            }
            else if (p == Protocol::Cloak) {
                set_textEditShareCloakText(QString(""));

                QJsonObject protoConfig = m_settings.protocolConfig(serverIndex, container, Protocol::Cloak);
                QString cfg = protoConfig.value(config_key::last_config).toString();

                if (cfg.isEmpty()) {
                    const QJsonObject &containerConfig = m_settings.containerConfig(serverIndex, container);

                    ErrorCode e = ErrorCode::NoError;
                    cfg = CloakConfigurator::genCloakConfig(credentials, container, containerConfig, &e);

                    //set_pushButtonShareCloakCopyEnabled(true);
                }

                QJsonObject cloakConfig = QJsonDocument::fromJson(cfg.toUtf8()).object();
                cloakConfig.remove(config_key::transport_proto);
                cloakConfig.insert("ProxyMethod", "shadowsocks");

                set_textEditShareCloakText(QJsonDocument(cloakConfig).toJson());
            }
        }

    }

    set_textEditShareAmneziaCodeText(tr(""));
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

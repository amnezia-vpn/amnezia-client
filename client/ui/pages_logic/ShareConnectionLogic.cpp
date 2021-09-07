#include <QApplication>
#include <QBuffer>
#include <QClipboard>
#include <QFileDialog>
#include <QTimer>
#include <QSaveFile>
#include <QStandardPaths>

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
    m_pageShareAmneziaVisible{true},
    m_pageShareOpenvpnVisible{true},
    m_pageShareShadowsocksVisible{true},
    m_pageShareCloakVisible{true},
    m_pageShareFullAccessVisible{true},
    m_textEditShareOpenvpnCodeText{},
    m_pushButtonShareOpenvpnCopyEnabled{false},
    m_pushButtonShareOpenvpnSaveEnabled{false},
    m_toolBoxShareConnectionCurrentIndex{-1},
    m_pushButtonShareSsCopyEnabled{false},
    m_lineEditShareSsStringText{},
    m_labelShareSsQrCodeText{},
    m_labelShareSsServerText{},
    m_labelShareSsPortText{},
    m_labelShareSsMethodText{},
    m_labelShareSsPasswordText{},
    m_plainTextEditShareCloakText{},
    m_pushButtonShareCloakCopyEnabled{false},
    m_textEditShareFullCodeText{},
    m_textEditShareAmneziaCodeText{},
    m_pushButtonShareFullCopyText{tr("Copy")},
    m_pushButtonShareAmneziaCopyText{tr("Copy")},
    m_pushButtonShareOpenvpnCopyText{tr("Copy")},
    m_pushButtonShareSsCopyText{tr("Copy")},
    m_pushButtonShareCloakCopyText{tr("Copy")},
    m_pushButtonShareAmneziaGenerateEnabled{true},
    m_pushButtonShareAmneziaCopyEnabled{true},
    m_pushButtonShareAmneziaGenerateText{tr("Generate config")},
    m_pushButtonShareOpenvpnGenerateEnabled{true},
    m_pushButtonShareOpenvpnGenerateText{tr("Generate config")}
{
    // TODO consider move to Component.onCompleted
    updateSharingPage(uiLogic()->selectedServerIndex, m_settings.serverCredentials(uiLogic()->selectedServerIndex), uiLogic()->selectedDockerContainer);
}


bool ShareConnectionLogic::getPageShareAmneziaVisible() const
{
    return m_pageShareAmneziaVisible;
}

void ShareConnectionLogic::setPageShareAmneziaVisible(bool pageShareAmneziaVisible)
{
    if (m_pageShareAmneziaVisible != pageShareAmneziaVisible) {
        m_pageShareAmneziaVisible = pageShareAmneziaVisible;
        emit pageShareAmneziaVisibleChanged();
    }
}

bool ShareConnectionLogic::getPageShareOpenvpnVisible() const
{
    return m_pageShareOpenvpnVisible;
}

void ShareConnectionLogic::setPageShareOpenvpnVisible(bool pageShareOpenvpnVisible)
{
    if (m_pageShareOpenvpnVisible != pageShareOpenvpnVisible) {
        m_pageShareOpenvpnVisible = pageShareOpenvpnVisible;
        emit pageShareOpenvpnVisibleChanged();
    }
}

bool ShareConnectionLogic::getPageShareShadowsocksVisible() const
{
    return m_pageShareShadowsocksVisible;
}

void ShareConnectionLogic::setPageShareShadowsocksVisible(bool pageShareShadowsocksVisible)
{
    if (m_pageShareShadowsocksVisible != pageShareShadowsocksVisible) {
        m_pageShareShadowsocksVisible = pageShareShadowsocksVisible;
        emit pageShareShadowsocksVisibleChanged();
    }
}

bool ShareConnectionLogic::getPageShareCloakVisible() const
{
    return m_pageShareCloakVisible;
}

void ShareConnectionLogic::setPageShareCloakVisible(bool pageShareCloakVisible)
{
    if (m_pageShareCloakVisible != pageShareCloakVisible) {
        m_pageShareCloakVisible = pageShareCloakVisible;
        emit pageShareCloakVisibleChanged();
    }
}

bool ShareConnectionLogic::getPageShareFullAccessVisible() const
{
    return m_pageShareFullAccessVisible;
}

void ShareConnectionLogic::setPageShareFullAccessVisible(bool pageShareFullAccessVisible)
{
    if (m_pageShareFullAccessVisible != pageShareFullAccessVisible) {
        m_pageShareFullAccessVisible = pageShareFullAccessVisible;
        emit pageShareFullAccessVisibleChanged();
    }
}

QString ShareConnectionLogic::getTextEditShareOpenvpnCodeText() const
{
    return m_textEditShareOpenvpnCodeText;
}

void ShareConnectionLogic::setTextEditShareOpenvpnCodeText(const QString &textEditShareOpenvpnCodeText)
{
    if (m_textEditShareOpenvpnCodeText != textEditShareOpenvpnCodeText) {
        m_textEditShareOpenvpnCodeText = textEditShareOpenvpnCodeText;
        emit textEditShareOpenvpnCodeTextChanged();
    }
}

bool ShareConnectionLogic::getPushButtonShareOpenvpnCopyEnabled() const
{
    return m_pushButtonShareOpenvpnCopyEnabled;
}

void ShareConnectionLogic::setPushButtonShareOpenvpnCopyEnabled(bool pushButtonShareOpenvpnCopyEnabled)
{
    if (m_pushButtonShareOpenvpnCopyEnabled != pushButtonShareOpenvpnCopyEnabled) {
        m_pushButtonShareOpenvpnCopyEnabled = pushButtonShareOpenvpnCopyEnabled;
        emit pushButtonShareOpenvpnCopyEnabledChanged();
    }
}

bool ShareConnectionLogic::getPushButtonShareOpenvpnSaveEnabled() const
{
    return m_pushButtonShareOpenvpnSaveEnabled;
}

void ShareConnectionLogic::setPushButtonShareOpenvpnSaveEnabled(bool pushButtonShareOpenvpnSaveEnabled)
{
    if (m_pushButtonShareOpenvpnSaveEnabled != pushButtonShareOpenvpnSaveEnabled) {
        m_pushButtonShareOpenvpnSaveEnabled = pushButtonShareOpenvpnSaveEnabled;
        emit pushButtonShareOpenvpnSaveEnabledChanged();
    }
}

int ShareConnectionLogic::getToolBoxShareConnectionCurrentIndex() const
{
    return m_toolBoxShareConnectionCurrentIndex;
}

void ShareConnectionLogic::setToolBoxShareConnectionCurrentIndex(int toolBoxShareConnectionCurrentIndex)
{
    if (m_toolBoxShareConnectionCurrentIndex != toolBoxShareConnectionCurrentIndex) {
        m_toolBoxShareConnectionCurrentIndex = toolBoxShareConnectionCurrentIndex;
        emit toolBoxShareConnectionCurrentIndexChanged();
    }
}

bool ShareConnectionLogic::getPushButtonShareSsCopyEnabled() const
{
    return m_pushButtonShareSsCopyEnabled;
}

void ShareConnectionLogic::setPushButtonShareSsCopyEnabled(bool pushButtonShareSsCopyEnabled)
{
    if (m_pushButtonShareSsCopyEnabled != pushButtonShareSsCopyEnabled) {
        m_pushButtonShareSsCopyEnabled = pushButtonShareSsCopyEnabled;
        emit pushButtonShareSsCopyEnabledChanged();
    }
}

QString ShareConnectionLogic::getLineEditShareSsStringText() const
{
    return m_lineEditShareSsStringText;
}

void ShareConnectionLogic::setLineEditShareSsStringText(const QString &lineEditShareSsStringText)
{
    if (m_lineEditShareSsStringText != lineEditShareSsStringText) {
        m_lineEditShareSsStringText = lineEditShareSsStringText;
        emit lineEditShareSsStringTextChanged();
    }
}

QString ShareConnectionLogic::getLabelShareSsQrCodeText() const
{
    return m_labelShareSsQrCodeText;
}

void ShareConnectionLogic::setLabelShareSsQrCodeText(const QString &labelShareSsQrCodeText)
{
    if (m_labelShareSsQrCodeText != labelShareSsQrCodeText) {
        m_labelShareSsQrCodeText = labelShareSsQrCodeText;
        emit labelShareSsQrCodeTextChanged();
    }
}

QString ShareConnectionLogic::getLabelShareSsServerText() const
{
    return m_labelShareSsServerText;
}

void ShareConnectionLogic::setLabelShareSsServerText(const QString &labelShareSsServerText)
{
    if (m_labelShareSsServerText != labelShareSsServerText) {
        m_labelShareSsServerText = labelShareSsServerText;
        emit labelShareSsServerTextChanged();
    }
}

QString ShareConnectionLogic::getLabelShareSsPortText() const
{
    return m_labelShareSsPortText;
}

void ShareConnectionLogic::setLabelShareSsPortText(const QString &labelShareSsPortText)
{
    if (m_labelShareSsPortText != labelShareSsPortText) {
        m_labelShareSsPortText = labelShareSsPortText;
        emit labelShareSsPortTextChanged();
    }
}

QString ShareConnectionLogic::getLabelShareSsMethodText() const
{
    return m_labelShareSsMethodText;
}

void ShareConnectionLogic::setLabelShareSsMethodText(const QString &labelShareSsMethodText)
{
    if (m_labelShareSsMethodText != labelShareSsMethodText) {
        m_labelShareSsMethodText = labelShareSsMethodText;
        emit labelShareSsMethodTextChanged();
    }
}

QString ShareConnectionLogic::getLabelShareSsPasswordText() const
{
    return m_labelShareSsPasswordText;
}

void ShareConnectionLogic::setLabelShareSsPasswordText(const QString &labelShareSsPasswordText)
{
    if (m_labelShareSsPasswordText != labelShareSsPasswordText) {
        m_labelShareSsPasswordText = labelShareSsPasswordText;
        emit labelShareSsPasswordTextChanged();
    }
}

QString ShareConnectionLogic::getPlainTextEditShareCloakText() const
{
    return m_plainTextEditShareCloakText;
}

void ShareConnectionLogic::setPlainTextEditShareCloakText(const QString &plainTextEditShareCloakText)
{
    if (m_plainTextEditShareCloakText != plainTextEditShareCloakText) {
        m_plainTextEditShareCloakText = plainTextEditShareCloakText;
        emit plainTextEditShareCloakTextChanged();
    }
}

bool ShareConnectionLogic::getPushButtonShareCloakCopyEnabled() const
{
    return m_pushButtonShareCloakCopyEnabled;
}

void ShareConnectionLogic::setPushButtonShareCloakCopyEnabled(bool pushButtonShareCloakCopyEnabled)
{
    if (m_pushButtonShareCloakCopyEnabled != pushButtonShareCloakCopyEnabled) {
        m_pushButtonShareCloakCopyEnabled = pushButtonShareCloakCopyEnabled;
        emit pushButtonShareCloakCopyEnabledChanged();
    }
}

QString ShareConnectionLogic::getTextEditShareFullCodeText() const
{
    return m_textEditShareFullCodeText;
}

void ShareConnectionLogic::setTextEditShareFullCodeText(const QString &textEditShareFullCodeText)
{
    if (m_textEditShareFullCodeText != textEditShareFullCodeText) {
        m_textEditShareFullCodeText = textEditShareFullCodeText;
        emit textEditShareFullCodeTextChanged();
    }
}

QString ShareConnectionLogic::getTextEditShareAmneziaCodeText() const
{
    return m_textEditShareAmneziaCodeText;
}

void ShareConnectionLogic::setTextEditShareAmneziaCodeText(const QString &textEditShareAmneziaCodeText)
{
    if (m_textEditShareAmneziaCodeText != textEditShareAmneziaCodeText) {
        m_textEditShareAmneziaCodeText = textEditShareAmneziaCodeText;
        emit textEditShareAmneziaCodeTextChanged();
    }
}

QString ShareConnectionLogic::getPushButtonShareFullCopyText() const
{
    return m_pushButtonShareFullCopyText;
}

void ShareConnectionLogic::setPushButtonShareFullCopyText(const QString &pushButtonShareFullCopyText)
{
    if (m_pushButtonShareFullCopyText != pushButtonShareFullCopyText) {
        m_pushButtonShareFullCopyText = pushButtonShareFullCopyText;
        emit pushButtonShareFullCopyTextChanged();
    }
}
QString ShareConnectionLogic::getPushButtonShareAmneziaCopyText() const
{
    return m_pushButtonShareAmneziaCopyText;
}

void ShareConnectionLogic::setPushButtonShareAmneziaCopyText(const QString &pushButtonShareAmneziaCopyText)
{
    if (m_pushButtonShareAmneziaCopyText != pushButtonShareAmneziaCopyText) {
        m_pushButtonShareAmneziaCopyText = pushButtonShareAmneziaCopyText;
        emit pushButtonShareAmneziaCopyTextChanged();
    }
}

QString ShareConnectionLogic::getPushButtonShareOpenvpnCopyText() const
{
    return m_pushButtonShareOpenvpnCopyText;
}

void ShareConnectionLogic::setPushButtonShareOpenvpnCopyText(const QString &pushButtonShareOpenvpnCopyText)
{
    if (m_pushButtonShareOpenvpnCopyText != pushButtonShareOpenvpnCopyText) {
        m_pushButtonShareOpenvpnCopyText = pushButtonShareOpenvpnCopyText;
        emit pushButtonShareOpenvpnCopyTextChanged();
    }
}

QString ShareConnectionLogic::getPushButtonShareSsCopyText() const
{
    return m_pushButtonShareSsCopyText;
}

void ShareConnectionLogic::setPushButtonShareSsCopyText(const QString &pushButtonShareSsCopyText)
{
    if (m_pushButtonShareSsCopyText != pushButtonShareSsCopyText) {
        m_pushButtonShareSsCopyText = pushButtonShareSsCopyText;
        emit pushButtonShareSsCopyTextChanged();
    }
}

QString ShareConnectionLogic::getPushButtonShareCloakCopyText() const
{
    return m_pushButtonShareCloakCopyText;
}

void ShareConnectionLogic::setPushButtonShareCloakCopyText(const QString &pushButtonShareCloakCopyText)
{
    if (m_pushButtonShareCloakCopyText != pushButtonShareCloakCopyText) {
        m_pushButtonShareCloakCopyText = pushButtonShareCloakCopyText;
        emit pushButtonShareCloakCopyTextChanged();
    }
}

bool ShareConnectionLogic::getPushButtonShareAmneziaGenerateEnabled() const
{
    return m_pushButtonShareAmneziaGenerateEnabled;
}

void ShareConnectionLogic::setPushButtonShareAmneziaGenerateEnabled(bool pushButtonShareAmneziaGenerateEnabled)
{
    if (m_pushButtonShareAmneziaGenerateEnabled != pushButtonShareAmneziaGenerateEnabled) {
        m_pushButtonShareAmneziaGenerateEnabled = pushButtonShareAmneziaGenerateEnabled;
        emit pushButtonShareAmneziaGenerateEnabledChanged();
    }
}

bool ShareConnectionLogic::getPushButtonShareAmneziaCopyEnabled() const
{
    return m_pushButtonShareAmneziaCopyEnabled;
}

void ShareConnectionLogic::setPushButtonShareAmneziaCopyEnabled(bool pushButtonShareAmneziaCopyEnabled)
{
    if (m_pushButtonShareAmneziaCopyEnabled != pushButtonShareAmneziaCopyEnabled) {
        m_pushButtonShareAmneziaCopyEnabled = pushButtonShareAmneziaCopyEnabled;
        emit pushButtonShareAmneziaCopyEnabledChanged();
    }
}

QString ShareConnectionLogic::getPushButtonShareAmneziaGenerateText() const
{
    return m_pushButtonShareAmneziaGenerateText;
}

void ShareConnectionLogic::setPushButtonShareAmneziaGenerateText(const QString &pushButtonShareAmneziaGenerateText)
{
    if (m_pushButtonShareAmneziaGenerateText != pushButtonShareAmneziaGenerateText) {
        m_pushButtonShareAmneziaGenerateText = pushButtonShareAmneziaGenerateText;
        emit pushButtonShareAmneziaGenerateTextChanged();
    }
}

bool ShareConnectionLogic::getPushButtonShareOpenvpnGenerateEnabled() const
{
    return m_pushButtonShareOpenvpnGenerateEnabled;
}

void ShareConnectionLogic::setPushButtonShareOpenvpnGenerateEnabled(bool pushButtonShareOpenvpnGenerateEnabled)
{
    if (m_pushButtonShareOpenvpnGenerateEnabled != pushButtonShareOpenvpnGenerateEnabled) {
        m_pushButtonShareOpenvpnGenerateEnabled = pushButtonShareOpenvpnGenerateEnabled;
        emit pushButtonShareOpenvpnGenerateEnabledChanged();
    }
}

QString ShareConnectionLogic::getPushButtonShareOpenvpnGenerateText() const
{
    return m_pushButtonShareOpenvpnGenerateText;
}

void ShareConnectionLogic::setPushButtonShareOpenvpnGenerateText(const QString &pushButtonShareOpenvpnGenerateText)
{
    if (m_pushButtonShareOpenvpnGenerateText != pushButtonShareOpenvpnGenerateText) {
        m_pushButtonShareOpenvpnGenerateText = pushButtonShareOpenvpnGenerateText;
        emit pushButtonShareOpenvpnGenerateTextChanged();
    }
}

void ShareConnectionLogic::onPushButtonShareFullCopyClicked()
{
    QGuiApplication::clipboard()->setText(getTextEditShareFullCodeText());
    setPushButtonShareFullCopyText(tr("Copied"));

    QTimer::singleShot(3000, this, [this]() {
        setPushButtonShareFullCopyText(tr("Copy"));
    });
}

void ShareConnectionLogic::onPushButtonShareFullSaveClicked()
{
    if (getTextEditShareFullCodeText().isEmpty()) return;

    QString fileName = QFileDialog::getSaveFileName(nullptr, tr("Save AmneziaVPN config"),
                                                    QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), "*.amnezia");
    QSaveFile save(fileName);
    save.open(QIODevice::WriteOnly);
    save.write(getTextEditShareFullCodeText().toUtf8());
    save.commit();
}

void ShareConnectionLogic::onPushButtonShareAmneziaCopyClicked()
{
    if (getTextEditShareAmneziaCodeText().isEmpty()) return;

    QGuiApplication::clipboard()->setText(getTextEditShareAmneziaCodeText());
    setPushButtonShareAmneziaCopyText(tr("Copied"));

    QTimer::singleShot(3000, this, [this]() {
        setPushButtonShareAmneziaCopyText(tr("Copy"));
    });
}

void ShareConnectionLogic::onPushButtonShareAmneziaSaveClicked()
{
    if (getTextEditShareAmneziaCodeText().isEmpty()) return;

    QString fileName = QFileDialog::getSaveFileName(nullptr, tr("Save AmneziaVPN config"),
                                                    QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), "*.amnezia");
    QSaveFile save(fileName);
    save.open(QIODevice::WriteOnly);
    save.write(getTextEditShareAmneziaCodeText().toUtf8());
    save.commit();
}

void ShareConnectionLogic::onPushButtonShareOpenvpnCopyClicked()
{
    QGuiApplication::clipboard()->setText(getTextEditShareOpenvpnCodeText());
    setPushButtonShareOpenvpnCopyText(tr("Copied"));

    QTimer::singleShot(3000, this, [this]() {
        setPushButtonShareOpenvpnCopyText(tr("Copy"));
    });
}

void ShareConnectionLogic::onPushButtonShareSsCopyClicked()
{
    QGuiApplication::clipboard()->setText(getLineEditShareSsStringText());
    setPushButtonShareSsCopyText(tr("Copied"));

    QTimer::singleShot(3000, this, [this]() {
        setPushButtonShareSsCopyText(tr("Copy"));
    });
}

void ShareConnectionLogic::onPushButtonShareCloakCopyClicked()
{
    QGuiApplication::clipboard()->setText(getPlainTextEditShareCloakText());
    setPushButtonShareCloakCopyText(tr("Copied"));

    QTimer::singleShot(3000, this, [this]() {
        setPushButtonShareCloakCopyText(tr("Copy"));
    });
}

void ShareConnectionLogic::onPushButtonShareAmneziaGenerateClicked()
{
    setPushButtonShareAmneziaGenerateEnabled(false);
    setPushButtonShareAmneziaCopyEnabled(false);
    setPushButtonShareAmneziaGenerateText(tr("Generating..."));
    qApp->processEvents();

    ServerCredentials credentials = m_settings.serverCredentials(uiLogic()->selectedServerIndex);
    QJsonObject containerConfig = m_settings.containerConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer);
    containerConfig.insert(config_key::container, containerToString(uiLogic()->selectedDockerContainer));

    ErrorCode e = ErrorCode::NoError;
    for (Protocol p: amnezia::protocolsForContainer(uiLogic()->selectedDockerContainer)) {
        QJsonObject protoConfig = m_settings.protocolConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer, p);

        QString cfg = VpnConfigurator::genVpnProtocolConfig(credentials, uiLogic()->selectedDockerContainer, containerConfig, p, &e);
        if (e) {
            cfg = "Error generating config";
            break;
        }
        protoConfig.insert(config_key::last_config, cfg);

        containerConfig.insert(protoToString(p), protoConfig);
    }

    QByteArray ba;
    if (!e) {
        QJsonObject serverConfig = m_settings.server(uiLogic()->selectedServerIndex);
        serverConfig.remove(config_key::userName);
        serverConfig.remove(config_key::password);
        serverConfig.remove(config_key::port);
        serverConfig.insert(config_key::containers, QJsonArray {containerConfig});
        serverConfig.insert(config_key::defaultContainer, containerToString(uiLogic()->selectedDockerContainer));


        ba = QJsonDocument(serverConfig).toJson().toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
        setTextEditShareAmneziaCodeText(QString("vpn://%1").arg(QString(ba)));
    }
    else {
        setTextEditShareAmneziaCodeText(tr("Error while generating connection profile"));
    }

    setPushButtonShareAmneziaGenerateEnabled(true);
    setPushButtonShareAmneziaCopyEnabled(true);
    setPushButtonShareAmneziaGenerateText(tr("Generate config"));
}

void ShareConnectionLogic::onPushButtonShareOpenvpnGenerateClicked()
{
    setPushButtonShareOpenvpnGenerateEnabled(false);
    setPushButtonShareOpenvpnCopyEnabled(false);
    setPushButtonShareOpenvpnSaveEnabled(false);
    setPushButtonShareOpenvpnGenerateText(tr("Generating..."));

    ServerCredentials credentials = m_settings.serverCredentials(uiLogic()->selectedServerIndex);
    const QJsonObject &containerConfig = m_settings.containerConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer);

    ErrorCode e = ErrorCode::NoError;
    QString cfg = OpenVpnConfigurator::genOpenVpnConfig(credentials, uiLogic()->selectedDockerContainer, containerConfig, &e);
    cfg = OpenVpnConfigurator::processConfigWithExportSettings(cfg);

    setTextEditShareOpenvpnCodeText(cfg);

    setPushButtonShareOpenvpnGenerateEnabled(true);
    setPushButtonShareOpenvpnCopyEnabled(true);
    setPushButtonShareOpenvpnSaveEnabled(true);
    setPushButtonShareOpenvpnGenerateText(tr("Generate config"));
}

void ShareConnectionLogic::onPushButtonShareOpenvpnSaveClicked()
{
    QString fileName = QFileDialog::getSaveFileName(nullptr, tr("Save OpenVPN config"),
                                                    QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), "*.ovpn");

    QSaveFile save(fileName);
    save.open(QIODevice::WriteOnly);
    save.write(getTextEditShareOpenvpnCodeText().toUtf8());
    save.commit();
}


void ShareConnectionLogic::updateSharingPage(int serverIndex, const ServerCredentials &credentials,
                                DockerContainer container)
{
    uiLogic()->selectedDockerContainer = container;
    uiLogic()->selectedServerIndex = serverIndex;

    //const QJsonObject &containerConfig = m_settings.containerConfig(serverIndex, container);

    setPageShareAmneziaVisible(false);
    setPageShareOpenvpnVisible(false);
    setPageShareShadowsocksVisible(false);
    setPageShareCloakVisible(false);
    setPageShareFullAccessVisible(false);

    enum currentWidget {
        full_access = 0,
        share_amezia,
        share_openvpn,
        share_shadowshock,
        share_cloak
    };

    if (container == DockerContainer::OpenVpn) {
        setPageShareAmneziaVisible(true);
        setPageShareOpenvpnVisible(true);

        QString cfg = tr("Press Generate config");
        setTextEditShareOpenvpnCodeText(cfg);
        setPushButtonShareOpenvpnCopyEnabled(false);
        setPushButtonShareOpenvpnSaveEnabled(false);

        setToolBoxShareConnectionCurrentIndex(share_openvpn);
    }

    if (container == DockerContainer::OpenVpnOverShadowSocks ||
            container == DockerContainer::OpenVpnOverCloak) {
        setPageShareAmneziaVisible(true);
        setPageShareShadowsocksVisible(true);

        QJsonObject protoConfig = m_settings.protocolConfig(serverIndex, container, Protocol::ShadowSocks);
        QString cfg = protoConfig.value(config_key::last_config).toString();

        if (cfg.isEmpty()) {
            const QJsonObject &containerConfig = m_settings.containerConfig(serverIndex, container);

            ErrorCode e = ErrorCode::NoError;
            cfg = ShadowSocksConfigurator::genShadowSocksConfig(credentials, container, containerConfig, &e);

            setPushButtonShareSsCopyEnabled(true);
        }

        QJsonObject ssConfig = QJsonDocument::fromJson(cfg.toUtf8()).object();

        QString ssString = QString("%1:%2@%3:%4")
                .arg(ssConfig.value("method").toString())
                .arg(ssConfig.value("password").toString())
                .arg(ssConfig.value("server").toString())
                .arg(ssConfig.value("server_port").toString());

        ssString = "ss://" + ssString.toUtf8().toBase64();
        setLineEditShareSsStringText(ssString);
        updateQRCodeImage(ssString, [this](const QString& labelText) ->void {
            setLabelShareSsQrCodeText(labelText);
        });

        setLabelShareSsServerText(ssConfig.value("server").toString());
        setLabelShareSsPortText(ssConfig.value("server_port").toString());
        setLabelShareSsMethodText(ssConfig.value("method").toString());
        setLabelShareSsPasswordText(ssConfig.value("password").toString());

        setToolBoxShareConnectionCurrentIndex(share_shadowshock);
    }

    if (container == DockerContainer::OpenVpnOverCloak) {
        //ui->toolBox_share_connection->addItem(ui->page_share_amnezia, tr("  Share for Amnezia client"));
        setPageShareCloakVisible(true);
        setPlainTextEditShareCloakText(QString(""));

        QJsonObject protoConfig = m_settings.protocolConfig(serverIndex, container, Protocol::Cloak);
        QString cfg = protoConfig.value(config_key::last_config).toString();

        if (cfg.isEmpty()) {
            const QJsonObject &containerConfig = m_settings.containerConfig(serverIndex, container);

            ErrorCode e = ErrorCode::NoError;
            cfg = CloakConfigurator::genCloakConfig(credentials, container, containerConfig, &e);

            setPushButtonShareCloakCopyEnabled(true);
        }

        QJsonObject cloakConfig = QJsonDocument::fromJson(cfg.toUtf8()).object();
        cloakConfig.remove(config_key::transport_proto);
        cloakConfig.insert("ProxyMethod", "shadowsocks");

        setPlainTextEditShareCloakText(QJsonDocument(cloakConfig).toJson());
    }

    // Full access
    if (container == DockerContainer::None) {
        setPageShareFullAccessVisible(true);

        const QJsonObject &server = m_settings.server(uiLogic()->selectedServerIndex);

        QByteArray ba = QJsonDocument(server).toJson().toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);

        setTextEditShareFullCodeText(QString("vpn://%1").arg(QString(ba)));
        setToolBoxShareConnectionCurrentIndex(full_access);
    }

    //ui->toolBox_share_connection->addItem(ui->page_share_amnezia, tr("  Share for Amnezia client"));

    // Amnezia sharing
    //    QJsonObject exportContainer;
    //    for (Protocol p: protocolsForContainer(container)) {
    //        QJsonObject protocolConfig = containerConfig.value(protoToString(p)).toObject();
    //        protocolConfig.remove(config_key::last_config);
    //        exportContainer.insert(protoToString(p), protocolConfig);
    //    }
    //    exportContainer.insert(config_key::container, containerToString(container));

    //    ui->textEdit_share_amnezia_code->setPlainText(QJsonDocument(exportContainer).toJson());

    setTextEditShareAmneziaCodeText(tr(""));
}


void ShareConnectionLogic::updateQRCodeImage(const QString &text, const std::function<void(const QString&)>& setLabelFunc)
{
    int levelIndex = 1;
    int versionIndex = 0;
    bool bExtent = true;
    int maskIndex = -1;

    m_qrEncode.EncodeData( levelIndex, versionIndex, bExtent, maskIndex, text.toUtf8().data() );

    int qrImageSize = m_qrEncode.m_nSymbleSize;

    int encodeImageSize = qrImageSize + ( QR_MARGIN * 2 );
    QImage encodeImage( encodeImageSize, encodeImageSize, QImage::Format_Mono );

    encodeImage.fill( 1 );

    for ( int i = 0; i < qrImageSize; i++ )
        for ( int j = 0; j < qrImageSize; j++ )
            if ( m_qrEncode.m_byModuleData[i][j] )
                encodeImage.setPixel( i + QR_MARGIN, j + QR_MARGIN, 0 );

    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    encodeImage.save(&buffer, "PNG"); // writes the image in PNG format inside the buffer
    QString iconBase64 = QString::fromLatin1(byteArray.toBase64().data());

    setLabelFunc(iconBase64);
}

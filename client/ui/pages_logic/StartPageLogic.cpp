#include "StartPageLogic.h"
#include "ViewConfigLogic.h"

#include "core/errorstrings.h"
#include "configurators/ssh_configurator.h"
#include "configurators/vpn_configurator.h"
#include "../uilogic.h"
#include "utilities.h"
#include "core/servercontroller.h"

#include <QFileDialog>
#include <QStandardPaths>

#ifdef Q_OS_ANDROID
#include <QJniObject>
#include "../../platforms/android/androidutils.h"
#include "../../platforms/android/android_controller.h"
#endif

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
    } else if (config.contains(wireguardConfigPatternSectionInterface) &&
               config.contains(wireguardConfigPatternSectionPeer))
        return ConfigTypes::WireGuard;
    return ConfigTypes::Amnezia;
}

}

StartPageLogic::StartPageLogic(UiLogic *logic, QObject *parent):
    PageLogicBase(logic, parent),
    m_pushButtonConnectEnabled{true},
    m_pushButtonConnectText{tr("Connect")},
    m_pushButtonConnectKeyChecked{false},
    m_labelWaitInfoVisible{true},
    m_pushButtonBackFromStartVisible{true},
    m_ipAddressPortRegex{Utils::ipAddressPortRegExp()}
{
#ifdef Q_OS_ANDROID
    // Set security screen for Android app
    AndroidUtils::runOnAndroidThreadSync([]() {
        QJniObject activity = AndroidUtils::getActivity();
        QJniObject window = activity.callObjectMethod("getWindow", "()Landroid/view/Window;");
        if (window.isValid()){
            const int FLAG_SECURE = 8192;
            window.callMethod<void>("addFlags", "(I)V", FLAG_SECURE);
        }
    });
#endif
}

void StartPageLogic::onUpdatePage()
{
    set_lineEditStartExistingCodeText("");
    set_textEditSshKeyText("");
    set_lineEditIpText("");
    set_lineEditPasswordText("");
    set_textEditSshKeyText("");
    set_lineEditLoginText("");

    set_labelWaitInfoVisible(false);
    set_labelWaitInfoText("");

    set_pushButtonConnectKeyChecked(false);

    set_pushButtonBackFromStartVisible(uiLogic()->pagesStackDepth() > 0);
}

void StartPageLogic::onPushButtonConnect()
{
    if (pushButtonConnectKeyChecked()){
        if (lineEditIpText().isEmpty() ||
                lineEditLoginText().isEmpty() ||
                textEditSshKeyText().isEmpty() ) {
            set_labelWaitInfoText(tr("Please fill in all fields"));
            return;
        }
    } else {
        if (lineEditIpText().isEmpty() ||
                lineEditLoginText().isEmpty() ||
                lineEditPasswordText().isEmpty() ) {
            set_labelWaitInfoText(tr("Please fill in all fields"));
            return;
        }
    }

    ServerCredentials serverCredentials;
    serverCredentials.hostName = lineEditIpText();
    if (serverCredentials.hostName.contains(":")) {
        serverCredentials.port = serverCredentials.hostName.split(":").at(1).toInt();
        serverCredentials.hostName = serverCredentials.hostName.split(":").at(0);
    }
    serverCredentials.userName = lineEditLoginText();
    if (pushButtonConnectKeyChecked()) {
        QString key = textEditSshKeyText();
        if (key.startsWith("ssh-rsa")) {
            emit uiLogic()->showPublicKeyWarning();
            return;
        }

        if (key.contains("OPENSSH") && key.contains("BEGIN") && key.contains("PRIVATE KEY")) {
            key = m_configurator->sshConfigurator->convertOpenSShKey(key);
        }

        serverCredentials.password = key;
    } else {
        serverCredentials.password = lineEditPasswordText();
    }

    set_pushButtonConnectEnabled(false);
    set_pushButtonConnectText(tr("Connecting..."));

    ErrorCode errorCode = ErrorCode::NoError;
#ifdef Q_DEBUG
    //QString output = m_serverController->checkSshConnection(serverCredentials, &e);
#else
    QString output;
#endif

    if (pushButtonConnectKeyChecked()) {
        QString decryptedPrivateKey;
        errorCode = uiLogic()->m_serverController->getDecryptedPrivateKey(serverCredentials, decryptedPrivateKey);
        if (errorCode == ErrorCode::NoError) {
            serverCredentials.password = decryptedPrivateKey;
        }
    }

    bool ok = true;
    if (errorCode) {
        set_labelWaitInfoVisible(true);
        set_labelWaitInfoText(errorString(errorCode));
        ok = false;
    } else {
        if (output.contains("Please login as the user")) {
            output.replace("\n", "");
            set_labelWaitInfoVisible(true);
            set_labelWaitInfoText(output);
            ok = false;
        }
    }

    set_pushButtonConnectEnabled(true);
    set_pushButtonConnectText(tr("Connect"));

    uiLogic()->m_installCredentials = serverCredentials;
    if (ok) emit uiLogic()->goToPage(Page::NewServer);
}

void StartPageLogic::onPushButtonImport()
{
    importConnectionFromCode(lineEditStartExistingCodeText());
}

void StartPageLogic::onPushButtonImportOpenFile()
{
    QString fileName = QFileDialog::getOpenFileName(Q_NULLPTR, tr("Open config file"),
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), "*.vpn *.ovpn *.conf");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    file.open(QIODevice::ReadOnly);
    QByteArray data = file.readAll();

    auto configFormat = checkConfigFormat(QString(data));
    if (configFormat == ConfigTypes::OpenVpn) {
        importConnectionFromOpenVpnConfig(QString(data));
    } else if (configFormat == ConfigTypes::WireGuard) {
        importConnectionFromWireguardConfig(QString(data));
    } else {
        importConnectionFromCode(QString(data));
    }
}

#ifdef Q_OS_ANDROID
void StartPageLogic::startQrDecoder()
{
    AndroidController::instance()->startQrReaderActivity();
}
#endif

bool StartPageLogic::importConnection(const QJsonObject &profile)
{
    ServerCredentials credentials;
    credentials.hostName = profile.value(config_key::hostName).toString();
    credentials.port = profile.value(config_key::port).toInt();
    credentials.userName = profile.value(config_key::userName).toString();
    credentials.password = profile.value(config_key::password).toString();

    if (credentials.isValid() || profile.contains(config_key::containers)) {
        // check config
        uiLogic()->pageLogic<ViewConfigLogic>()->set_configJson(profile);
        emit uiLogic()->goToPage(Page::ViewConfig);
    }
    else {
        qDebug() << "Failed to import profile";
        qDebug().noquote() << QJsonDocument(profile).toJson();
        return false;
    }

    return true;
}

bool StartPageLogic::importConnectionFromCode(QString code)
{
    code.replace("vpn://", "");
    QByteArray ba = QByteArray::fromBase64(code.toUtf8(), QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);

    QByteArray ba_uncompressed = qUncompress(ba);
    if (!ba_uncompressed.isEmpty()) {
        ba = ba_uncompressed;
    }

    QJsonObject o;
    o = QJsonDocument::fromJson(ba).object();
    if (!o.isEmpty()) {
        return importConnection(o);
    }

    return false;
}

bool StartPageLogic::importConnectionFromQr(const QByteArray &data)
{
    QJsonObject dataObj = QJsonDocument::fromJson(data).object();
    if (!dataObj.isEmpty()) {
        return importConnection(dataObj);
    }

    QByteArray ba_uncompressed = qUncompress(data);
    if (!ba_uncompressed.isEmpty()) {
        return importConnection(QJsonDocument::fromJson(ba_uncompressed).object());
    }

    return false;
}

bool StartPageLogic::importConnectionFromOpenVpnConfig(const QString &config)
{
    QJsonObject openVpnConfig;
    openVpnConfig[config_key::config] = config;

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
    QRegularExpressionMatch hostNameMatch = hostNameRegExp.match(config);
    if (hostNameMatch.hasMatch()) {
        hostName = hostNameMatch.captured(1);
    }

    QJsonObject o;
    o[config_key::containers] = arr;
    o[config_key::defaultContainer] = "amnezia-openvpn";
    o[config_key::description] = m_settings->nextAvailableServerName();


    const static QRegularExpression dnsRegExp("dhcp-option DNS (\\b\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\b)");
    QRegularExpressionMatchIterator dnsMatch = dnsRegExp.globalMatch(config);
    if (dnsMatch.hasNext()) {
        o[config_key::dns1] = dnsMatch.next().captured(1);
    }
    if (dnsMatch.hasNext()) {
        o[config_key::dns2] = dnsMatch.next().captured(1);
    }

    o[config_key::hostName] = hostName;

    return importConnection(o);
}

bool StartPageLogic::importConnectionFromWireguardConfig(const QString &config)
{
    QJsonObject lastConfig;
    lastConfig[config_key::config] = config;

    const static QRegularExpression hostNameAndPortRegExp("Endpoint = (.*):([0-9]*)");
    QRegularExpressionMatch hostNameAndPortMatch = hostNameAndPortRegExp.match(config);
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

    QJsonObject o;
    o[config_key::containers] = arr;
    o[config_key::defaultContainer] = "amnezia-wireguard";
    o[config_key::description] = m_settings->nextAvailableServerName();

    const static QRegularExpression dnsRegExp("DNS = (\\b\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\b).*(\\b\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\b)");
    QRegularExpressionMatch dnsMatch = dnsRegExp.match(config);
    if (dnsMatch.hasMatch()) {
        o[config_key::dns1] = dnsMatch.captured(1);
        o[config_key::dns2] = dnsMatch.captured(2);
    }

    o[config_key::hostName] = hostName;

    return importConnection(o);
}

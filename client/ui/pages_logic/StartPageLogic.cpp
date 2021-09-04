//#include <QApplication>
//#include <QClipboard>
//#include <QDebug>
//#include <QDesktopServices>
//#include <QFileDialog>
//#include <QHBoxLayout>
//#include <QHostInfo>
//#include <QItemSelectionModel>
//#include <QJsonDocument>
//#include <QJsonObject>
//#include <QKeyEvent>
//#include <QMenu>
//#include <QMessageBox>
//#include <QMetaEnum>
//#include <QSysInfo>
//#include <QThread>
//#include <QTimer>
//#include <QRegularExpression>
//#include <QSaveFile>



//#include "debug.h"
//#include "defines.h"
#include "StartPageLogic.h"
#include "core/errorstrings.h"
//#include "utils.h"
//#include "vpnconnection.h"
//#include <functional>

#include "configurators/ssh_configurator.h"

using namespace amnezia;
using namespace PageEnumNS;

#include "../uilogic.h"


StartPageLogic::StartPageLogic(UiLogic *uiLogic, QObject *parent):
    QObject(parent),
    m_uiLogic(uiLogic),
    m_pushButtonNewServerConnectEnabled{},
    m_pushButtonNewServerConnectText{tr("Connect")},
    m_pushButtonNewServerConnectKeyChecked{false},
    m_lineEditStartExistingCodeText{},
    m_textEditNewServerSshKeyText{},
    m_lineEditNewServerIpText{},
    m_lineEditNewServerPasswordText{},
    m_lineEditNewServerLoginText{},
    m_labelNewServerWaitInfoVisible{true},
    m_labelNewServerWaitInfoText{},
    m_progressBarNewServerConnectionMinimum{0},
    m_progressBarNewServerConnectionMaximum{100},
    m_pushButtonBackFromStartVisible{true},
    m_pushButtonNewServerConnectVisible{true}
{

}

void StartPageLogic::updateStartPage()
{
    setLineEditStartExistingCodeText("");
    setTextEditNewServerSshKeyText("");
    setLineEditNewServerIpText("");
    setLineEditNewServerPasswordText("");
    setTextEditNewServerSshKeyText("");
    setLineEditNewServerLoginText("");

    setLabelNewServerWaitInfoVisible(false);
    setLabelNewServerWaitInfoText("");
    setProgressBarNewServerConnectionMinimum(0);
    setProgressBarNewServerConnectionMaximum(300);
    setPushButtonNewServerConnectVisible(true);
}

bool StartPageLogic::getPushButtonNewServerConnectKeyChecked() const
{
    return m_pushButtonNewServerConnectKeyChecked;
}

void StartPageLogic::setPushButtonNewServerConnectKeyChecked(bool pushButtonNewServerConnectKeyChecked)
{
    if (m_pushButtonNewServerConnectKeyChecked != pushButtonNewServerConnectKeyChecked) {
        m_pushButtonNewServerConnectKeyChecked = pushButtonNewServerConnectKeyChecked;
        emit pushButtonNewServerConnectKeyCheckedChanged();
    }
}

QString StartPageLogic::getLineEditStartExistingCodeText() const
{
    return m_lineEditStartExistingCodeText;
}

void StartPageLogic::setLineEditStartExistingCodeText(const QString &lineEditStartExistingCodeText)
{
    if (m_lineEditStartExistingCodeText != lineEditStartExistingCodeText) {
        m_lineEditStartExistingCodeText = lineEditStartExistingCodeText;
        emit lineEditStartExistingCodeTextChanged();
    }
}

QString StartPageLogic::getTextEditNewServerSshKeyText() const
{
    return m_textEditNewServerSshKeyText;
}

void StartPageLogic::setTextEditNewServerSshKeyText(const QString &textEditNewServerSshKeyText)
{
    if (m_textEditNewServerSshKeyText != textEditNewServerSshKeyText) {
        m_textEditNewServerSshKeyText = textEditNewServerSshKeyText;
        emit textEditNewServerSshKeyTextChanged();
    }
}

QString StartPageLogic::getLineEditNewServerIpText() const
{
    return m_lineEditNewServerIpText;
}

void StartPageLogic::setLineEditNewServerIpText(const QString &lineEditNewServerIpText)
{
    if (m_lineEditNewServerIpText != lineEditNewServerIpText) {
        m_lineEditNewServerIpText = lineEditNewServerIpText;
        emit lineEditNewServerIpTextChanged();
    }
}

QString StartPageLogic::getLineEditNewServerPasswordText() const
{
    return m_lineEditNewServerPasswordText;
}

void StartPageLogic::setLineEditNewServerPasswordText(const QString &lineEditNewServerPasswordText)
{
    if (m_lineEditNewServerPasswordText != lineEditNewServerPasswordText) {
        m_lineEditNewServerPasswordText = lineEditNewServerPasswordText;
        emit lineEditNewServerPasswordTextChanged();
    }
}

QString StartPageLogic::getLineEditNewServerLoginText() const
{
    return m_lineEditNewServerLoginText;
}

void StartPageLogic::setLineEditNewServerLoginText(const QString &lineEditNewServerLoginText)
{
    if (m_lineEditNewServerLoginText != lineEditNewServerLoginText) {
        m_lineEditNewServerLoginText = lineEditNewServerLoginText;
        emit lineEditNewServerLoginTextChanged();
    }
}

bool StartPageLogic::getLabelNewServerWaitInfoVisible() const
{
    return m_labelNewServerWaitInfoVisible;
}

void StartPageLogic::setLabelNewServerWaitInfoVisible(bool labelNewServerWaitInfoVisible)
{
    if (m_labelNewServerWaitInfoVisible != labelNewServerWaitInfoVisible) {
        m_labelNewServerWaitInfoVisible = labelNewServerWaitInfoVisible;
        emit labelNewServerWaitInfoVisibleChanged();
    }
}

QString StartPageLogic::getLabelNewServerWaitInfoText() const
{
    return m_labelNewServerWaitInfoText;
}

void StartPageLogic::setLabelNewServerWaitInfoText(const QString &labelNewServerWaitInfoText)
{
    if (m_labelNewServerWaitInfoText != labelNewServerWaitInfoText) {
        m_labelNewServerWaitInfoText = labelNewServerWaitInfoText;
        emit labelNewServerWaitInfoTextChanged();
    }
}

double StartPageLogic::getProgressBarNewServerConnectionMinimum() const
{
    return m_progressBarNewServerConnectionMinimum;
}

void StartPageLogic::setProgressBarNewServerConnectionMinimum(double progressBarNewServerConnectionMinimum)
{
    if (m_progressBarNewServerConnectionMinimum != progressBarNewServerConnectionMinimum) {
        m_progressBarNewServerConnectionMinimum = progressBarNewServerConnectionMinimum;
        emit progressBarNewServerConnectionMinimumChanged();
    }
}

double StartPageLogic::getProgressBarNewServerConnectionMaximum() const
{
    return m_progressBarNewServerConnectionMaximum;
}

void StartPageLogic::setProgressBarNewServerConnectionMaximum(double progressBarNewServerConnectionMaximum)
{
    if (m_progressBarNewServerConnectionMaximum != progressBarNewServerConnectionMaximum) {
        m_progressBarNewServerConnectionMaximum = progressBarNewServerConnectionMaximum;
        emit progressBarNewServerConnectionMaximumChanged();
    }
}

bool StartPageLogic::getPushButtonNewServerConnectVisible() const
{
    return m_pushButtonNewServerConnectVisible;
}

void StartPageLogic::setPushButtonNewServerConnectVisible(bool pushButtonNewServerConnectVisible)
{
    if (m_pushButtonNewServerConnectVisible != pushButtonNewServerConnectVisible) {
        m_pushButtonNewServerConnectVisible = pushButtonNewServerConnectVisible;
        emit pushButtonNewServerConnectVisibleChanged();
    }
}

bool StartPageLogic::getPushButtonBackFromStartVisible() const
{
    return m_pushButtonBackFromStartVisible;
}

void StartPageLogic::setPushButtonBackFromStartVisible(bool pushButtonBackFromStartVisible)
{
    if (m_pushButtonBackFromStartVisible != pushButtonBackFromStartVisible) {
        m_pushButtonBackFromStartVisible = pushButtonBackFromStartVisible;
        emit pushButtonBackFromStartVisibleChanged();
    }
}

void StartPageLogic::onPushButtonNewServerConnect()
{
    if (getPushButtonNewServerConnectKeyChecked()){
        if (getLineEditNewServerIpText().isEmpty() ||
                getLineEditNewServerLoginText().isEmpty() ||
                getTextEditNewServerSshKeyText().isEmpty() ) {
            setLabelNewServerWaitInfoText(tr("Please fill in all fields"));
            return;
        }
    }
    else {
        if (getLineEditNewServerIpText().isEmpty() ||
                getLineEditNewServerLoginText().isEmpty() ||
                getLineEditNewServerPasswordText().isEmpty() ) {
            setLabelNewServerWaitInfoText(tr("Please fill in all fields"));
            return;
        }
    }
    qDebug() << "UiLogic::onPushButtonNewServerConnect checking new server";

    ServerCredentials serverCredentials;
    serverCredentials.hostName = getLineEditNewServerIpText();
    if (serverCredentials.hostName.contains(":")) {
        serverCredentials.port = serverCredentials.hostName.split(":").at(1).toInt();
        serverCredentials.hostName = serverCredentials.hostName.split(":").at(0);
    }
    serverCredentials.userName = getLineEditNewServerLoginText();
    if (getPushButtonNewServerConnectKeyChecked()){
        QString key = getTextEditNewServerSshKeyText();
        if (key.startsWith("ssh-rsa")) {
            emit m_uiLogic->showPublicKeyWarning();
            return;
        }

        if (key.contains("OPENSSH") && key.contains("BEGIN") && key.contains("PRIVATE KEY")) {
            key = SshConfigurator::convertOpenSShKey(key);
        }

        serverCredentials.password = key;
    }
    else {
        serverCredentials.password = getLineEditNewServerPasswordText();
    }

    setPushButtonNewServerConnectEnabled(false);
    setPushButtonNewServerConnectText(tr("Connecting..."));

    ErrorCode e = ErrorCode::NoError;
#ifdef Q_DEBUG
    //QString output = ServerController::checkSshConnection(serverCredentials, &e);
#else
    QString output;
#endif

    bool ok = true;
    if (e) {
        setLabelNewServerWaitInfoVisible(true);
        setLabelNewServerWaitInfoText(errorString(e));
        ok = false;
    }
    else {
        if (output.contains("Please login as the user")) {
            output.replace("\n", "");
            setLabelNewServerWaitInfoVisible(true);
            setLabelNewServerWaitInfoText(output);
            ok = false;
        }
    }

    setPushButtonNewServerConnectEnabled(true);
    setPushButtonNewServerConnectText(tr("Connect"));

    m_uiLogic->installCredentials = serverCredentials;
    if (ok) m_uiLogic->goToPage(Page::NewServer);
}

void StartPageLogic::onPushButtonNewServerImport()
{
    QString s = getLineEditStartExistingCodeText();
    s.replace("vpn://", "");
    QJsonObject o = QJsonDocument::fromJson(QByteArray::fromBase64(s.toUtf8(), QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)).object();

    ServerCredentials credentials;
    credentials.hostName = o.value("h").toString();
    if (credentials.hostName.isEmpty()) credentials.hostName = o.value(config_key::hostName).toString();

    credentials.port = o.value("p").toInt();
    if (credentials.port == 0) credentials.port = o.value(config_key::port).toInt();

    credentials.userName = o.value("u").toString();
    if (credentials.userName.isEmpty()) credentials.userName = o.value(config_key::userName).toString();

    credentials.password = o.value("w").toString();
    if (credentials.password.isEmpty()) credentials.password = o.value(config_key::password).toString();

    if (credentials.isValid()) {
        o.insert(config_key::hostName, credentials.hostName);
        o.insert(config_key::port, credentials.port);
        o.insert(config_key::userName, credentials.userName);
        o.insert(config_key::password, credentials.password);

        o.remove("h");
        o.remove("p");
        o.remove("u");
        o.remove("w");
    }
    qDebug() << QString("Added server %3@%1:%2").
                arg(credentials.hostName).
                arg(credentials.port).
                arg(credentials.userName);

    //qDebug() << QString("Password") << credentials.password;

    if (credentials.isValid() || o.contains(config_key::containers)) {
        m_settings.addServer(o);
        m_settings.setDefaultServer(m_settings.serversCount() - 1);

        m_uiLogic->setStartPage(Page::Vpn);
    }
    else {
        qDebug() << "Failed to import profile";
        qDebug().noquote() << QJsonDocument(o).toJson();
        return;
    }

    if (!o.contains(config_key::containers)) {
        m_uiLogic->selectedServerIndex = m_settings.defaultServerIndex();
        m_uiLogic->selectedDockerContainer = m_settings.defaultContainer(m_uiLogic->selectedServerIndex);
        m_uiLogic->goToPage(Page::ServerVpnProtocols);
    }
}

bool StartPageLogic::getPushButtonNewServerConnectEnabled() const
{
    return m_pushButtonNewServerConnectEnabled;
}

void StartPageLogic::setPushButtonNewServerConnectEnabled(bool pushButtonNewServerConnectEnabled)
{
    if (m_pushButtonNewServerConnectEnabled != pushButtonNewServerConnectEnabled) {
        m_pushButtonNewServerConnectEnabled = pushButtonNewServerConnectEnabled;
        emit pushButtonNewServerConnectEnabledChanged();
    }
}

QString StartPageLogic::getPushButtonNewServerConnectText() const
{
    return m_pushButtonNewServerConnectText;
}

void StartPageLogic::setPushButtonNewServerConnectText(const QString &pushButtonNewServerConnectText)
{
    if (m_pushButtonNewServerConnectText != pushButtonNewServerConnectText) {
        m_pushButtonNewServerConnectText = pushButtonNewServerConnectText;
        emit pushButtonNewServerConnectTextChanged();
    }
}

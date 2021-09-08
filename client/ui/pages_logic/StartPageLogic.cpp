#include "StartPageLogic.h"
#include "core/errorstrings.h"
#include "configurators/ssh_configurator.h"
#include "../uilogic.h"

StartPageLogic::StartPageLogic(UiLogic *logic, QObject *parent):
    PageLogicBase(logic, parent),
    m_pushButtonNewServerConnectEnabled{true},
    m_pushButtonNewServerConnectText{tr("Connect")},
    m_pushButtonNewServerConnectKeyChecked{false},
    m_lineEditStartExistingCodeText{},
    m_textEditNewServerSshKeyText{},
    m_lineEditNewServerIpText{},
    m_lineEditNewServerPasswordText{},
    m_lineEditNewServerLoginText{},
    m_labelNewServerWaitInfoVisible{true},
    m_labelNewServerWaitInfoText{},
    m_pushButtonBackFromStartVisible{true},
    m_pushButtonNewServerConnectVisible{true}
{

}

void StartPageLogic::updateStartPage()
{
    set_lineEditStartExistingCodeText("");
    set_textEditNewServerSshKeyText("");
    set_lineEditNewServerIpText("");
    set_lineEditNewServerPasswordText("");
    set_textEditNewServerSshKeyText("");
    set_lineEditNewServerLoginText("");

    set_labelNewServerWaitInfoVisible(false);
    set_labelNewServerWaitInfoText("");
    set_pushButtonNewServerConnectVisible(true);
}

void StartPageLogic::onPushButtonNewServerConnect()
{
    if (pushButtonNewServerConnectKeyChecked()){
        if (lineEditNewServerIpText().isEmpty() ||
                lineEditNewServerLoginText().isEmpty() ||
                textEditNewServerSshKeyText().isEmpty() ) {
            set_labelNewServerWaitInfoText(tr("Please fill in all fields"));
            return;
        }
    }
    else {
        if (lineEditNewServerIpText().isEmpty() ||
                lineEditNewServerLoginText().isEmpty() ||
                lineEditNewServerPasswordText().isEmpty() ) {
            set_labelNewServerWaitInfoText(tr("Please fill in all fields"));
            return;
        }
    }
    qDebug() << "UiLogic::onPushButtonNewServerConnect checking new server";

    ServerCredentials serverCredentials;
    serverCredentials.hostName = lineEditNewServerIpText();
    if (serverCredentials.hostName.contains(":")) {
        serverCredentials.port = serverCredentials.hostName.split(":").at(1).toInt();
        serverCredentials.hostName = serverCredentials.hostName.split(":").at(0);
    }
    serverCredentials.userName = lineEditNewServerLoginText();
    if (pushButtonNewServerConnectKeyChecked()){
        QString key = textEditNewServerSshKeyText();
        if (key.startsWith("ssh-rsa")) {
            emit uiLogic()->showPublicKeyWarning();
            return;
        }

        if (key.contains("OPENSSH") && key.contains("BEGIN") && key.contains("PRIVATE KEY")) {
            key = SshConfigurator::convertOpenSShKey(key);
        }

        serverCredentials.password = key;
    }
    else {
        serverCredentials.password = lineEditNewServerPasswordText();
    }

    set_pushButtonNewServerConnectEnabled(false);
    set_pushButtonNewServerConnectText(tr("Connecting..."));

    ErrorCode e = ErrorCode::NoError;
#ifdef Q_DEBUG
    //QString output = ServerController::checkSshConnection(serverCredentials, &e);
#else
    QString output;
#endif

    bool ok = true;
    if (e) {
        set_labelNewServerWaitInfoVisible(true);
        set_labelNewServerWaitInfoText(errorString(e));
        ok = false;
    }
    else {
        if (output.contains("Please login as the user")) {
            output.replace("\n", "");
            set_labelNewServerWaitInfoVisible(true);
            set_labelNewServerWaitInfoText(output);
            ok = false;
        }
    }

    set_pushButtonNewServerConnectEnabled(true);
    set_pushButtonNewServerConnectText(tr("Connect"));

    uiLogic()->installCredentials = serverCredentials;
    if (ok) uiLogic()->goToPage(Page::NewServer);
}

void StartPageLogic::onPushButtonNewServerImport()
{
    QString s = lineEditStartExistingCodeText();
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

        uiLogic()->setStartPage(Page::Vpn);
    }
    else {
        qDebug() << "Failed to import profile";
        qDebug().noquote() << QJsonDocument(o).toJson();
        return;
    }

    if (!o.contains(config_key::containers)) {
        uiLogic()->selectedServerIndex = m_settings.defaultServerIndex();
        uiLogic()->selectedDockerContainer = m_settings.defaultContainer(uiLogic()->selectedServerIndex);
        uiLogic()->goToPage(Page::ServerContainers);
    }
}

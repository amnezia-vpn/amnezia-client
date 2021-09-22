#include "StartPageLogic.h"
#include "core/errorstrings.h"
#include "configurators/ssh_configurator.h"
#include "../uilogic.h"

StartPageLogic::StartPageLogic(UiLogic *logic, QObject *parent):
    PageLogicBase(logic, parent),
    m_pushButtonConnectEnabled{true},
    m_pushButtonConnectText{tr("Connect")},
    m_pushButtonConnectKeyChecked{false},
    m_lineEditStartExistingCodeText{},
    m_textEditSshKeyText{},
    m_lineEditIpText{},
    m_lineEditPasswordText{},
    m_lineEditLoginText{},
    m_labelWaitInfoVisible{true},
    m_labelWaitInfoText{},
    m_pushButtonBackFromStartVisible{true},
    m_pushButtonConnectVisible{true}
{

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
    set_pushButtonConnectVisible(true);

    set_pushButtonConnectKeyChecked(false);
}

void StartPageLogic::onPushButtonConnect()
{
//    uiLogic()->goToPage(Page::NewServer);
//    return;
    if (pushButtonConnectKeyChecked()){
        if (lineEditIpText().isEmpty() ||
                lineEditLoginText().isEmpty() ||
                textEditSshKeyText().isEmpty() ) {
            set_labelWaitInfoText(tr("Please fill in all fields"));
            return;
        }
    }
    else {
        if (lineEditIpText().isEmpty() ||
                lineEditLoginText().isEmpty() ||
                lineEditPasswordText().isEmpty() ) {
            set_labelWaitInfoText(tr("Please fill in all fields"));
            return;
        }
    }
    qDebug() << "UiLogic::onPushButtonConnect checking new server";

    ServerCredentials serverCredentials;
    serverCredentials.hostName = lineEditIpText();
    if (serverCredentials.hostName.contains(":")) {
        serverCredentials.port = serverCredentials.hostName.split(":").at(1).toInt();
        serverCredentials.hostName = serverCredentials.hostName.split(":").at(0);
    }
    serverCredentials.userName = lineEditLoginText();
    if (pushButtonConnectKeyChecked()){
        QString key = textEditSshKeyText();
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
        serverCredentials.password = lineEditPasswordText();
    }

    set_pushButtonConnectEnabled(false);
    set_pushButtonConnectText(tr("Connecting..."));

    ErrorCode e = ErrorCode::NoError;
#ifdef Q_DEBUG
    //QString output = ServerController::checkSshConnection(serverCredentials, &e);
#else
    QString output;
#endif

    bool ok = true;
    if (e) {
        set_labelWaitInfoVisible(true);
        set_labelWaitInfoText(errorString(e));
        ok = false;
    }
    else {
        if (output.contains("Please login as the user")) {
            output.replace("\n", "");
            set_labelWaitInfoVisible(true);
            set_labelWaitInfoText(output);
            ok = false;
        }
    }

    set_pushButtonConnectEnabled(true);
    set_pushButtonConnectText(tr("Connect"));

    uiLogic()->installCredentials = serverCredentials;
    if (ok) uiLogic()->goToPage(Page::NewServer);
}

void StartPageLogic::onPushButtonImport()
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

        emit uiLogic()->setStartPage(Page::Vpn);
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

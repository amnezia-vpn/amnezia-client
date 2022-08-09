#include "StartPageLogic.h"
#include "core/errorstrings.h"
#include "configurators/ssh_configurator.h"
#include "../uilogic.h"
#include "utils.h"

#include <QFileDialog>
#include <QStandardPaths>

#ifdef Q_OS_ANDROID
#include "platforms/android/android_controller.h"
#endif

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
    m_pushButtonConnectVisible{true},
    m_ipAddressPortRegex{Utils::ipAddressPortRegExp()}
{
#ifdef Q_OS_LINUX
    QStringList config_path = QStandardPaths::standardLocations(QStandardPaths::ConfigLocation);

    QDir conf_path(config_path[0] + "/" + ORGANIZATION_NAME);
    if( !conf_path.exists() && config_path.size() > 0 && config_path[0].contains("home"))
    {
        //make config directory and set permissions
        conf_path.mkdir(config_path[0] + "/" + ORGANIZATION_NAME);
        QFile::setPermissions(config_path[0] + "/" + ORGANIZATION_NAME, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);

        QStringList config_location = QStandardPaths::standardLocations(QStandardPaths::AppConfigLocation);

        //make config file and set permissions
        QFile conf_file(config_location[0] + ".conf");
        conf_file.open(QIODevice::WriteOnly);
        conf_file.close();

        QFile::setPermissions(config_location[0] + ".conf", QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    }
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
    set_pushButtonConnectVisible(true);

    set_pushButtonConnectKeyChecked(false);

    set_pushButtonBackFromStartVisible(uiLogic()->pagesStackDepth() > 0);
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
    if (ok) emit uiLogic()->goToPage(Page::NewServer);
}

void StartPageLogic::onPushButtonImport()
{
    importConnectionFromCode(lineEditStartExistingCodeText());
}

void StartPageLogic::onPushButtonImportOpenFile()
{
    QString fileName = QFileDialog::getOpenFileName(nullptr, tr("Open profile"),
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), "*.vpn");

    if (fileName.isEmpty()) return;

    QFile file(fileName);
    file.open(QIODevice::ReadOnly);
    QByteArray data = file.readAll();

    importConnectionFromCode(QString(data));
}

bool StartPageLogic::importConnection(const QJsonObject &profile)
{
    ServerCredentials credentials;
    credentials.hostName = profile.value(config_key::hostName).toString();
    credentials.port = profile.value(config_key::port).toInt();
    credentials.userName = profile.value(config_key::userName).toString();
    credentials.password = profile.value(config_key::password).toString();

//    qDebug() << QString("Added server %3@%1:%2").
//                arg(credentials.hostName).
//                arg(credentials.port).
//                arg(credentials.userName);

    //qDebug() << QString("Password") << credentials.password;

    if (credentials.isValid() || profile.contains(config_key::containers)) {
        m_settings.addServer(profile);
        m_settings.setDefaultServer(m_settings.serversCount() - 1);

        emit uiLogic()->goToPage(Page::Vpn);
        emit uiLogic()->setStartPage(Page::Vpn);
    }
    else {
        qDebug() << "Failed to import profile";
        qDebug().noquote() << QJsonDocument(profile).toJson();
        return false;
    }

    if (!profile.contains(config_key::containers)) {
        uiLogic()->selectedServerIndex = m_settings.defaultServerIndex();
        uiLogic()->selectedDockerContainer = m_settings.defaultContainer(uiLogic()->selectedServerIndex);
        uiLogic()->onUpdateAllPages();

        emit uiLogic()->goToPage(Page::ServerContainers);
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

    o = QJsonDocument::fromBinaryData(ba).object();
    if (!o.isEmpty()) {
        return importConnection(o);
    }
    return false;
}

bool StartPageLogic::importConnectionFromQr(const QByteArray &data)
{
    qDebug() << "StartPageLogic::importConnectionFromQr" << data;
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

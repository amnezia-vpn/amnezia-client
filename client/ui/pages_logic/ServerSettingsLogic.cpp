#include "ServerSettingsLogic.h"
#include "vpnconnection.h"

#include "../uilogic.h"
#include "ServerListLogic.h"
#include "ShareConnectionLogic.h"
#include "VpnLogic.h"

#include "core/errorstrings.h"
#include <core/servercontroller.h>
#include <QTimer>

#if defined(Q_OS_ANDROID)
#include <QAndroidJniObject>
#include <QAndroidJniEnvironment>
#include <QtAndroid>
#endif

ServerSettingsLogic::ServerSettingsLogic(UiLogic *logic, QObject *parent):
    PageLogicBase(logic, parent),
    m_labelWaitInfoVisible{true},
    m_pushButtonClearVisible{true},
    m_pushButtonClearClientCacheVisible{true},
    m_pushButtonShareFullVisible{true},
    m_pushButtonClearText{tr("Clear server from Amnezia software")},
    m_pushButtonClearClientCacheText{tr("Clear client cached profile")}
{

}

void ServerSettingsLogic::onUpdatePage()
{
    set_labelWaitInfoVisible(false);
    set_labelWaitInfoText("");
    set_pushButtonClearVisible(m_settings->haveAuthData(uiLogic()->selectedServerIndex));
    set_pushButtonClearClientCacheVisible(m_settings->haveAuthData(uiLogic()->selectedServerIndex));
    set_pushButtonShareFullVisible(m_settings->haveAuthData(uiLogic()->selectedServerIndex));
    const QJsonObject &server = m_settings->server(uiLogic()->selectedServerIndex);
    const QString &port = server.value(config_key::port).toString();
    set_labelServerText(QString("%1@%2%3%4")
                                     .arg(server.value(config_key::userName).toString())
                                     .arg(server.value(config_key::hostName).toString())
                                     .arg(port.isEmpty() ? "" : ":")
                                     .arg(port));
    set_lineEditDescriptionText(server.value(config_key::description).toString());

    DockerContainer selectedContainer = m_settings->defaultContainer(uiLogic()->selectedServerIndex);
    QString selectedContainerName = ContainerProps::containerHumanNames().value(selectedContainer);
    set_labelCurrentVpnProtocolText(tr("Service: ") + selectedContainerName);
}

void ServerSettingsLogic::onPushButtonClearServer()
{
    set_pageEnabled(false);
    set_pushButtonClearText(tr("Uninstalling Amnezia software..."));

    if (m_settings->defaultServerIndex() == uiLogic()->selectedServerIndex) {
        uiLogic()->pageLogic<VpnLogic>()->onDisconnect();
    }

    ErrorCode e = m_serverController->removeAllContainers(m_settings->serverCredentials(uiLogic()->selectedServerIndex));
    m_serverController->disconnectFromHost(m_settings->serverCredentials(uiLogic()->selectedServerIndex));
    if (e) {
        uiLogic()->set_dialogConnectErrorText(
                    tr("Error occurred while configuring server.") + "\n" +
                    errorString(e) + "\n" +
                    tr("See logs for details."));
        emit uiLogic()->showConnectErrorDialog();
    }
    else {
        set_labelWaitInfoVisible(true);
        set_labelWaitInfoText(tr("Amnezia server successfully uninstalled"));
    }

    m_settings->setContainers(uiLogic()->selectedServerIndex, {});
    m_settings->setDefaultContainer(uiLogic()->selectedServerIndex, DockerContainer::None);

    set_pageEnabled(true);
    set_pushButtonClearText(tr("Clear server from Amnezia software"));
}

void ServerSettingsLogic::onPushButtonForgetServer()
{
    if (m_settings->defaultServerIndex() == uiLogic()->selectedServerIndex && uiLogic()->m_vpnConnection->isConnected()) {
        uiLogic()->pageLogic<VpnLogic>()->onDisconnect();
    }
    m_settings->removeServer(uiLogic()->selectedServerIndex);

    if (m_settings->defaultServerIndex() == uiLogic()->selectedServerIndex) {
        m_settings->setDefaultServer(0);
    }
    else if (m_settings->defaultServerIndex() > uiLogic()->selectedServerIndex) {
        m_settings->setDefaultServer(m_settings->defaultServerIndex() - 1);
    }

    if (m_settings->serversCount() == 0) {
        m_settings->setDefaultServer(-1);
    }


    uiLogic()->selectedServerIndex = -1;
    uiLogic()->onUpdateAllPages();

    if (m_settings->serversCount() == 0) {
        uiLogic()->setStartPage(Page::Start);
    }
    else {
        uiLogic()->closePage();
    }
}

void ServerSettingsLogic::onPushButtonClearClientCacheClicked()
{
    set_pushButtonClearClientCacheText(tr("Cache cleared"));

    const auto &containers = m_settings->containers(uiLogic()->selectedServerIndex);
    for (DockerContainer container: containers.keys()) {
        m_settings->clearLastConnectionConfig(uiLogic()->selectedServerIndex, container);
    }

    QTimer::singleShot(3000, this, [this]() {
        set_pushButtonClearClientCacheText(tr("Clear client cached profile"));
    });
}

void ServerSettingsLogic::onLineEditDescriptionEditingFinished()
{
    const QString &newText = lineEditDescriptionText();
    QJsonObject server = m_settings->server(uiLogic()->selectedServerIndex);
    server.insert(config_key::description, newText);
    m_settings->editServer(uiLogic()->selectedServerIndex, server);
    uiLogic()->onUpdateAllPages();
}

#if defined(Q_OS_ANDROID)
/* Auth result handler for Android */
void authResultReceiver::handleActivityResult(int receiverRequestCode, int resultCode, const QAndroidJniObject &data)
{
    qDebug() << "receiverRequestCode" << receiverRequestCode << "resultCode" << resultCode;

    if (resultCode == -1) { //ResultOK
        uiLogic()->pageLogic<ShareConnectionLogic>()->updateSharingPage(m_serverIndex, DockerContainer::None);
        emit uiLogic()->goToShareProtocolPage(Proto::Any);
    }
}
#endif

void ServerSettingsLogic::onPushButtonShareFullClicked()
{
#if defined(Q_OS_ANDROID)
/* We use builtin keyguard for ssh key export protection on Android */
    auto appContext = QtAndroid::androidActivity().callObjectMethod(
        "getApplicationContext", "()Landroid/content/Context;");
    if (appContext.isValid()) {
        QAndroidActivityResultReceiver *receiver = new authResultReceiver(uiLogic(), uiLogic()->selectedServerIndex);
        auto intent = QAndroidJniObject::callStaticObjectMethod(
               "org/amnezia/vpn/AuthHelper", "getAuthIntent",
               "(Landroid/content/Context;)Landroid/content/Intent;", appContext.object());
        if (intent.isValid()) {
            if (intent.object<jobject>() != nullptr) {
                    QtAndroid::startActivity(intent.object<jobject>(), 1, receiver);
                }
            } else {
            uiLogic()->pageLogic<ShareConnectionLogic>()->updateSharingPage(uiLogic()->selectedServerIndex, DockerContainer::None);
            emit uiLogic()->goToShareProtocolPage(Proto::Any);
        }
    }
#else
    uiLogic()->pageLogic<ShareConnectionLogic>()->updateSharingPage(uiLogic()->selectedServerIndex, DockerContainer::None);
    emit uiLogic()->goToShareProtocolPage(Proto::Any);
#endif
}

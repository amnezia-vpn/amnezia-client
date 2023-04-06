#include "ServerSettingsLogic.h"
#include "vpnconnection.h"

#include "../uilogic.h"
#include "ShareConnectionLogic.h"
#include "VpnLogic.h"

#include "core/errorstrings.h"
#include <core/servercontroller.h>
#include <QTimer>

#if defined(Q_OS_ANDROID)
#include "../../platforms/android/androidutils.h"
#endif

ServerSettingsLogic::ServerSettingsLogic(UiLogic *logic, QObject *parent):
    PageLogicBase(logic, parent),
    m_labelWaitInfoVisible{true},
    m_pushButtonClearClientCacheVisible{true},
    m_pushButtonShareFullVisible{true},
    m_pushButtonClearClientCacheText{tr("Clear client cached profile")}
{ }

void ServerSettingsLogic::onUpdatePage()
{
    set_labelWaitInfoVisible(false);
    set_labelWaitInfoText("");
    set_pushButtonClearClientCacheVisible(m_settings->haveAuthData(uiLogic()->m_selectedServerIndex));
    set_pushButtonShareFullVisible(m_settings->haveAuthData(uiLogic()->m_selectedServerIndex));
    const QJsonObject &server = m_settings->server(uiLogic()->m_selectedServerIndex);
    const QString &port = server.value(config_key::port).toString();

    const QString &userName = server.value(config_key::userName).toString();
    const QString &hostName = server.value(config_key::hostName).toString();
    QString name = QString("%1%2%3%4%5")
        .arg(userName)
        .arg(userName.isEmpty() ? "" : "@")
        .arg(hostName)
        .arg(port.isEmpty() ? "" : ":")
        .arg(port);

    set_labelServerText(name);
    set_lineEditDescriptionText(server.value(config_key::description).toString());

    DockerContainer selectedContainer = m_settings->defaultContainer(uiLogic()->m_selectedServerIndex);
    QString selectedContainerName = ContainerProps::containerHumanNames().value(selectedContainer);
    set_labelCurrentVpnProtocolText(tr("Service: ") + selectedContainerName);
}

void ServerSettingsLogic::onPushButtonForgetServer()
{
    if (m_settings->defaultServerIndex() == uiLogic()->m_selectedServerIndex && uiLogic()->m_vpnConnection->isConnected()) {
        uiLogic()->pageLogic<VpnLogic>()->onDisconnect();
    }
    m_settings->removeServer(uiLogic()->m_selectedServerIndex);

    if (m_settings->defaultServerIndex() == uiLogic()->m_selectedServerIndex) {
        m_settings->setDefaultServer(0);
    }
    else if (m_settings->defaultServerIndex() > uiLogic()->m_selectedServerIndex) {
        m_settings->setDefaultServer(m_settings->defaultServerIndex() - 1);
    }

    if (m_settings->serversCount() == 0) {
        m_settings->setDefaultServer(-1);
    }


    uiLogic()->m_selectedServerIndex = -1;
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

    const auto &containers = m_settings->containers(uiLogic()->m_selectedServerIndex);
    for (DockerContainer container : containers.keys()) {
        m_settings->clearLastConnectionConfig(uiLogic()->m_selectedServerIndex, container);
    }

    QTimer::singleShot(3000, this, [this]() {
        set_pushButtonClearClientCacheText(tr("Clear client cached profile"));
    });
}

void ServerSettingsLogic::onLineEditDescriptionEditingFinished()
{
    const QString &newText = lineEditDescriptionText();
    QJsonObject server = m_settings->server(uiLogic()->m_selectedServerIndex);
    server.insert(config_key::description, newText);
    m_settings->editServer(uiLogic()->m_selectedServerIndex, server);
    uiLogic()->onUpdateAllPages();
}

#if defined(Q_OS_ANDROID)
/* Auth result handler for Android */
void authResultReceiver::handleActivityResult(int receiverRequestCode, int resultCode, const QJniObject &data)
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
    QJniObject activity = AndroidUtils::getActivity();
    auto appContext = activity.callObjectMethod(
        "getApplicationContext", "()Landroid/content/Context;");
    if (appContext.isValid()) {
        QAndroidActivityResultReceiver *receiver = new authResultReceiver(uiLogic(), uiLogic()->m_selectedServerIndex);
        auto intent = QJniObject::callStaticObjectMethod(
               "org/amnezia/vpn/AuthHelper", "getAuthIntent",
               "(Landroid/content/Context;)Landroid/content/Intent;", appContext.object());
        if (intent.isValid()) {
            if (intent.object<jobject>() != nullptr) {
                    QtAndroidPrivate::startActivity(intent.object<jobject>(), 1, receiver);
                }
            } else {
            uiLogic()->pageLogic<ShareConnectionLogic>()->updateSharingPage(uiLogic()->m_selectedServerIndex, DockerContainer::None);
            emit uiLogic()->goToShareProtocolPage(Proto::Any);
        }
    }
#else
    uiLogic()->pageLogic<ShareConnectionLogic>()->updateSharingPage(uiLogic()->m_selectedServerIndex, DockerContainer::None);
    emit uiLogic()->goToShareProtocolPage(Proto::Any);
#endif
}

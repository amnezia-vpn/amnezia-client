#include "ServerContainersLogic.h"
#include "ShareConnectionLogic.h"
#include "protocols/CloakLogic.h"
#include "protocols/OpenVpnLogic.h"
#include "protocols/ShadowSocksLogic.h"

#include "core/servercontroller.h"
#include <functional>

#include "../uilogic.h"

ServerContainersLogic::ServerContainersLogic(UiLogic *logic, QObject *parent):
    PageLogicBase(logic, parent),
    m_progressBarProtocolsContainerReinstallValue{0},
    m_progressBarProtocolsContainerReinstallMaximium{100},
    m_pushButtonOpenvpnContInstallChecked{false},
    m_pushButtonSsOpenvpnContInstallChecked{false},
    m_pushButtonCloakOpenvpnContInstallChecked{false},
    m_pushButtonWireguardContInstallChecked{false},
    m_pushButtonOpenvpnContInstallEnabled{false},
    m_pushButtonSsOpenvpnContInstallEnabled{false},
    m_pushButtonCloakOpenvpnContInstallEnabled{false},
    m_pushButtonWireguardContInstallEnabled{false},
    m_pushButtonOpenvpnContDefaultChecked{false},
    m_pushButtonSsOpenvpnContDefaultChecked{false},
    m_pushButtonCloakOpenvpnContDefaultChecked{false},
    m_pushButtonWireguardContDefaultChecked{false},
    m_pushButtonOpenvpnContDefaultVisible{true},
    m_pushButtonSsOpenvpnContDefaultVisible{false},
    m_pushButtonCloakOpenvpnContDefaultVisible{false},
    m_pushButtonWireguardContDefaultVisible{false},
    m_pushButtonOpenvpnContShareVisible{false},
    m_pushButtonSsOpenvpnContShareVisible{false},
    m_pushButtonCloakOpenvpnContShareVisible{false},
    m_pushButtonWireguardContShareVisible{false},
    m_frameOpenvpnSettingsVisible{true},
    m_frameOpenvpnSsSettingsVisible{true},
    m_frameOpenvpnSsCloakSettingsVisible{true},
    m_progressBarProtocolsContainerReinstallVisible{false},
    m_frameWireguardSettingsVisible{false},
    m_frameWireguardVisible{false}
{
    setupProtocolsPageConnections();

    set_frameWireguardSettingsVisible(false);
    set_frameWireguardVisible(false);
}

void ServerContainersLogic::updateServerContainersPage()
{
    set_progressBarProtocolsContainerReinstallVisible(false);

    auto containers = m_settings.containers(uiLogic()->selectedServerIndex);
    DockerContainer defaultContainer = m_settings.defaultContainer(uiLogic()->selectedServerIndex);
    bool haveAuthData = m_settings.haveAuthData(uiLogic()->selectedServerIndex);

    // all containers
    QList<DockerContainer> allContainers {
        DockerContainer::OpenVpn,
                DockerContainer::OpenVpnOverShadowSocks,
                DockerContainer::OpenVpnOverCloak,
                DockerContainer::WireGuard
    };

    using SetVisibleFunc = std::function<void(bool)>;
    using SetCheckedFunc = std::function<void(bool)>;
    using SetEnabledFunc = std::function<void(bool)>;
    QList<SetCheckedFunc> installButtonsCheckedFunc {
        [this](bool checked) ->void {set_pushButtonOpenvpnContInstallChecked(checked);},
        [this](bool checked) ->void {set_pushButtonSsOpenvpnContInstallChecked(checked);},
        [this](bool checked) ->void {set_pushButtonCloakOpenvpnContInstallChecked(checked);},
        [this](bool checked) ->void {set_pushButtonWireguardContInstallChecked(checked);},
    };
    QList<SetEnabledFunc> installButtonsEnabledFunc {
        [this](bool enabled) ->void {set_pushButtonOpenvpnContInstallEnabled(enabled);},
        [this](bool enabled) ->void {set_pushButtonSsOpenvpnContInstallEnabled(enabled);},
        [this](bool enabled) ->void {set_pushButtonCloakOpenvpnContInstallEnabled(enabled);},
        [this](bool enabled) ->void {set_pushButtonWireguardContInstallEnabled(enabled);},
    };

    QList<SetCheckedFunc> defaultButtonsCheckedFunc {
        [this](bool checked) ->void {set_pushButtonOpenvpnContDefaultChecked(checked);},
        [this](bool checked) ->void {set_pushButtonSsOpenvpnContDefaultChecked(checked);},
        [this](bool checked) ->void {set_pushButtonCloakOpenvpnContDefaultChecked(checked);},
        [this](bool checked) ->void {set_pushButtonWireguardContDefaultChecked(checked);},
    };
    QList<SetVisibleFunc> defaultButtonsVisibleFunc {
        [this](bool visible) ->void {set_pushButtonOpenvpnContDefaultVisible(visible);},
        [this](bool visible) ->void {set_pushButtonSsOpenvpnContDefaultVisible(visible);},
        [this](bool visible) ->void {set_pushButtonCloakOpenvpnContDefaultVisible(visible);},
        [this](bool visible) ->void {set_pushButtonWireguardContDefaultVisible(visible);},
    };

    QList<SetVisibleFunc> shareButtonsVisibleFunc {
        [this](bool visible) ->void {set_pushButtonOpenvpnContShareVisible(visible);},
        [this](bool visible) ->void {set_pushButtonSsOpenvpnContShareVisible(visible);},
        [this](bool visible) ->void {set_pushButtonCloakOpenvpnContShareVisible(visible);},
        [this](bool visible) ->void {set_pushButtonWireguardContShareVisible(visible);},
    };

    QList<SetVisibleFunc> framesVisibleFunc {
        [this](bool visible) ->void {set_frameOpenvpnSettingsVisible(visible);},
        [this](bool visible) ->void {set_frameOpenvpnSsSettingsVisible(visible);},
        [this](bool visible) ->void {set_frameOpenvpnSsCloakSettingsVisible(visible);},
        [this](bool visible) ->void {set_frameWireguardSettingsVisible(visible);},
    };

    for (int i = 0; i < allContainers.size(); ++i) {
        defaultButtonsCheckedFunc.at(i)(defaultContainer == allContainers.at(i));
        defaultButtonsVisibleFunc.at(i)(haveAuthData && containers.contains(allContainers.at(i)));
        shareButtonsVisibleFunc.at(i)(haveAuthData && containers.contains(allContainers.at(i)));
        installButtonsCheckedFunc.at(i)(containers.contains(allContainers.at(i)));
        installButtonsEnabledFunc.at(i)(haveAuthData);
        framesVisibleFunc.at(i)(containers.contains(allContainers.at(i)));
    }
}


void ServerContainersLogic::setupProtocolsPageConnections()
{
    QJsonObject openvpnConfig;

    // all containers
    QList<DockerContainer> containers {
        DockerContainer::OpenVpn,
                DockerContainer::OpenVpnOverShadowSocks,
                DockerContainer::OpenVpnOverCloak,
                DockerContainer::WireGuard
    };
    using ButtonClickedFunc = void (ServerContainersLogic::*)(bool);
    using ButtonSetEnabledFunc = std::function<void(bool)>;

    // default buttons
    QList<ButtonClickedFunc> defaultButtonClickedSig {
        &ServerContainersLogic::pushButtonOpenvpnContDefaultClicked,
                &ServerContainersLogic::pushButtonSsOpenvpnContDefaultClicked,
                &ServerContainersLogic::pushButtonCloakOpenvpnContDefaultClicked,
                &ServerContainersLogic::pushButtonWireguardContDefaultClicked
    };

    for (int i = 0; i < containers.size(); ++i) {
        connect(this, defaultButtonClickedSig.at(i), [this, containers, i](bool){
            qDebug() << "clmm" << i;
            m_settings.setDefaultContainer(uiLogic()->selectedServerIndex, containers.at(i));
            updateServerContainersPage();
        });
    }

    // install buttons
    QList<ButtonClickedFunc> installButtonsClickedSig {
        &ServerContainersLogic::pushButtonOpenvpnContInstallClicked,
                &ServerContainersLogic::pushButtonSsOpenvpnContInstallClicked,
                &ServerContainersLogic::pushButtonCloakOpenvpnContInstallClicked,
                &ServerContainersLogic::pushButtonWireguardContInstallClicked,
    };
    QList<ButtonSetEnabledFunc> installButtonsSetEnabledFunc {
        [this] (bool enabled) -> void {
            set_pushButtonOpenvpnContInstallEnabled(enabled);
        },
        [this] (bool enabled) -> void {
            set_pushButtonSsOpenvpnContInstallEnabled(enabled);
        },
        [this] (bool enabled) -> void {
            set_pushButtonCloakOpenvpnContInstallEnabled(enabled);
        },
        [this] (bool enabled) -> void {
            set_pushButtonWireguardContInstallEnabled(enabled);
        },
    };

    for (int i = 0; i < containers.size(); ++i) {
        ButtonClickedFunc buttonClickedFunc = installButtonsClickedSig.at(i);
        ButtonSetEnabledFunc buttonSetEnabledFunc = installButtonsSetEnabledFunc.at(i);
        DockerContainer container = containers.at(i);

        connect(this, buttonClickedFunc, [this, container, buttonSetEnabledFunc](bool checked){
            if (checked) {
                UiLogic::PageFunc page_server_containers;
                page_server_containers.setEnabledFunc = [this] (bool enabled) -> void {
                    set_pageEnabled(enabled);
                };
                UiLogic::ButtonFunc no_button;
                UiLogic::LabelFunc no_label;
                UiLogic::ProgressFunc progressBar_protocols_container_reinstall;
                progressBar_protocols_container_reinstall.setVisibleFunc = [this] (bool visible) ->void {
                    set_progressBarProtocolsContainerReinstallVisible(visible);
                };
                progressBar_protocols_container_reinstall.setValueFunc = [this] (int value) ->void {
                    set_progressBarProtocolsContainerReinstallValue(value);
                };
                progressBar_protocols_container_reinstall.getValueFunc = [this] (void) -> int {
                    return progressBarProtocolsContainerReinstallValue();
                };
                progressBar_protocols_container_reinstall.getMaximiumFunc = [this] (void) -> int {
                    return progressBarProtocolsContainerReinstallMaximium();
                };

                ErrorCode e = uiLogic()->doInstallAction([this, container](){
                    return ServerController::setupContainer(m_settings.serverCredentials(uiLogic()->selectedServerIndex), container);
                },
                page_server_containers, progressBar_protocols_container_reinstall,
                no_button, no_label);

                if (!e) {
                    m_settings.setContainerConfig(uiLogic()->selectedServerIndex, container, QJsonObject());
                    m_settings.setDefaultContainer(uiLogic()->selectedServerIndex, container);
                }
            }
            else {
                buttonSetEnabledFunc(false);
                ErrorCode e = ServerController::removeContainer(m_settings.serverCredentials(uiLogic()->selectedServerIndex), container);
                m_settings.removeContainerConfig(uiLogic()->selectedServerIndex, container);
                buttonSetEnabledFunc(true);

                if (m_settings.defaultContainer(uiLogic()->selectedServerIndex) == container) {
                    const auto &c = m_settings.containers(uiLogic()->selectedServerIndex);
                    if (c.isEmpty()) m_settings.setDefaultContainer(uiLogic()->selectedServerIndex, DockerContainer::None);
                    else m_settings.setDefaultContainer(uiLogic()->selectedServerIndex, c.keys().first());
                }
            }

            updateServerContainersPage();
        });
    }

    // share buttons
    QList<ButtonClickedFunc> shareButtonsClickedSig {
        &ServerContainersLogic::pushButtonOpenvpnContShareClicked,
                &ServerContainersLogic::pushButtonSsOpenvpnContShareClicked,
                &ServerContainersLogic::pushButtonCloakOpenvpnContShareClicked,
                &ServerContainersLogic::pushButtonWireguardContShareClicked,
    };

    for (int i = 0; i < containers.size(); ++i) {
        ButtonClickedFunc buttonClickedFunc = shareButtonsClickedSig.at(i);
        DockerContainer container = containers.at(i);

        connect(this, buttonClickedFunc, [this, container](bool){
            uiLogic()->shareConnectionLogic()->updateSharingPage(uiLogic()->selectedServerIndex, m_settings.serverCredentials(uiLogic()->selectedServerIndex), container);
            uiLogic()->goToPage(Page::ShareConnection);
        });
    }
}

void ServerContainersLogic::onPushButtonProtoCloakOpenvpnContOpenvpnConfigClicked()
{
    uiLogic()->selectedDockerContainer = DockerContainer::OpenVpnOverCloak;
    uiLogic()->openVpnLogic()->updateOpenVpnPage(m_settings.protocolConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer, Protocol::OpenVpn),
                      uiLogic()->selectedDockerContainer, m_settings.haveAuthData(uiLogic()->selectedServerIndex));
    uiLogic()->goToPage(Page::OpenVpnSettings);
}

void ServerContainersLogic::onPushButtonProtoCloakOpenvpnContSsConfigClicked()
{
    uiLogic()->selectedDockerContainer = DockerContainer::OpenVpnOverCloak;
    uiLogic()->shadowSocksLogic()->updateShadowSocksPage(m_settings.protocolConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer, Protocol::ShadowSocks),
                          uiLogic()->selectedDockerContainer, m_settings.haveAuthData(uiLogic()->selectedServerIndex));
    uiLogic()->goToPage(Page::ShadowSocksSettings);
}

void ServerContainersLogic::onPushButtonProtoCloakOpenvpnContCloakConfigClicked()
{
    uiLogic()->selectedDockerContainer = DockerContainer::OpenVpnOverCloak;
    uiLogic()->cloakLogic()->updateCloakPage(m_settings.protocolConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer, Protocol::Cloak),
                    uiLogic()->selectedDockerContainer, m_settings.haveAuthData(uiLogic()->selectedServerIndex));
    uiLogic()->goToPage(Page::CloakSettings);
}

void ServerContainersLogic::onPushButtonProtoOpenvpnContOpenvpnConfigClicked()
{
    uiLogic()->selectedDockerContainer = DockerContainer::OpenVpn;
    uiLogic()->openVpnLogic()->updateOpenVpnPage(m_settings.protocolConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer, Protocol::OpenVpn),
                      uiLogic()->selectedDockerContainer, m_settings.haveAuthData(uiLogic()->selectedServerIndex));
    uiLogic()->goToPage(Page::OpenVpnSettings);
}

void ServerContainersLogic::onPushButtonProtoSsOpenvpnContOpenvpnConfigClicked()
{
    uiLogic()->selectedDockerContainer = DockerContainer::OpenVpnOverShadowSocks;
    uiLogic()->openVpnLogic()->updateOpenVpnPage(m_settings.protocolConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer, Protocol::OpenVpn),
                      uiLogic()->selectedDockerContainer, m_settings.haveAuthData(uiLogic()->selectedServerIndex));
    uiLogic()->goToPage(Page::OpenVpnSettings);
}

void ServerContainersLogic::onPushButtonProtoSsOpenvpnContSsConfigClicked()
{
    uiLogic()->selectedDockerContainer = DockerContainer::OpenVpnOverShadowSocks;
    uiLogic()->shadowSocksLogic()->updateShadowSocksPage(m_settings.protocolConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer, Protocol::ShadowSocks),
                          uiLogic()->selectedDockerContainer, m_settings.haveAuthData(uiLogic()->selectedServerIndex));
    uiLogic()->goToPage(Page::ShadowSocksSettings);
}

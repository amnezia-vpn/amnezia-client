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
    m_pushButtonOpenVpnContInstallChecked{false},
    m_pushButtonSsOpenVpnContInstallChecked{false},
    m_pushButtonCloakOpenVpnContInstallChecked{false},
    m_pushButtonWireguardContInstallChecked{false},
    m_pushButtonOpenVpnContInstallEnabled{false},
    m_pushButtonSsOpenVpnContInstallEnabled{false},
    m_pushButtonCloakOpenVpnContInstallEnabled{false},
    m_pushButtonWireguardContInstallEnabled{false},
    m_pushButtonOpenVpnContDefaultChecked{false},
    m_pushButtonSsOpenVpnContDefaultChecked{false},
    m_pushButtonCloakOpenVpnContDefaultChecked{false},
    m_pushButtonWireguardContDefaultChecked{false},
    m_pushButtonOpenVpnContDefaultVisible{true},
    m_pushButtonSsOpenVpnContDefaultVisible{false},
    m_pushButtonCloakOpenVpnContDefaultVisible{false},
    m_pushButtonWireguardContDefaultVisible{false},
    m_pushButtonOpenVpnContShareVisible{false},
    m_pushButtonSsOpenVpnContShareVisible{false},
    m_pushButtonCloakOpenVpnContShareVisible{false},
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

    ContainersModel *c_model = qobject_cast<ContainersModel *>(uiLogic()->containersModel());
    c_model->setSelectedServerIndex(uiLogic()->selectedServerIndex);

    ProtocolsModel *p_model = qobject_cast<ProtocolsModel *>(uiLogic()->protocolsModel());
    p_model->setSelectedServerIndex(uiLogic()->selectedServerIndex);

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
        [this](bool checked) ->void {set_pushButtonOpenVpnContInstallChecked(checked);},
        [this](bool checked) ->void {set_pushButtonSsOpenVpnContInstallChecked(checked);},
        [this](bool checked) ->void {set_pushButtonCloakOpenVpnContInstallChecked(checked);},
        [this](bool checked) ->void {set_pushButtonWireguardContInstallChecked(checked);},
    };
    QList<SetEnabledFunc> installButtonsEnabledFunc {
        [this](bool enabled) ->void {set_pushButtonOpenVpnContInstallEnabled(enabled);},
        [this](bool enabled) ->void {set_pushButtonSsOpenVpnContInstallEnabled(enabled);},
        [this](bool enabled) ->void {set_pushButtonCloakOpenVpnContInstallEnabled(enabled);},
        [this](bool enabled) ->void {set_pushButtonWireguardContInstallEnabled(enabled);},
    };

    QList<SetCheckedFunc> defaultButtonsCheckedFunc {
        [this](bool checked) ->void {set_pushButtonOpenVpnContDefaultChecked(checked);},
        [this](bool checked) ->void {set_pushButtonSsOpenVpnContDefaultChecked(checked);},
        [this](bool checked) ->void {set_pushButtonCloakOpenVpnContDefaultChecked(checked);},
        [this](bool checked) ->void {set_pushButtonWireguardContDefaultChecked(checked);},
    };
    QList<SetVisibleFunc> defaultButtonsVisibleFunc {
        [this](bool visible) ->void {set_pushButtonOpenVpnContDefaultVisible(visible);},
        [this](bool visible) ->void {set_pushButtonSsOpenVpnContDefaultVisible(visible);},
        [this](bool visible) ->void {set_pushButtonCloakOpenVpnContDefaultVisible(visible);},
        [this](bool visible) ->void {set_pushButtonWireguardContDefaultVisible(visible);},
    };

    QList<SetVisibleFunc> shareButtonsVisibleFunc {
        [this](bool visible) ->void {set_pushButtonOpenVpnContShareVisible(visible);},
        [this](bool visible) ->void {set_pushButtonSsOpenVpnContShareVisible(visible);},
        [this](bool visible) ->void {set_pushButtonCloakOpenVpnContShareVisible(visible);},
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

void ServerContainersLogic::onPushButtonProtoSettingsClicked(amnezia::DockerContainer c, amnezia::Protocol p)
{
    uiLogic()->selectedDockerContainer = c;
    uiLogic()->protocolLogic(p)->updateProtocolPage(m_settings.protocolConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer, p),
                      uiLogic()->selectedDockerContainer,
                      m_settings.haveAuthData(uiLogic()->selectedServerIndex));

    emit uiLogic()->goToProtocolPage(static_cast<int>(p));
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
        &ServerContainersLogic::pushButtonOpenVpnContDefaultClicked,
                &ServerContainersLogic::pushButtonSsOpenVpnContDefaultClicked,
                &ServerContainersLogic::pushButtonCloakOpenVpnContDefaultClicked,
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
        &ServerContainersLogic::pushButtonOpenVpnContInstallClicked,
                &ServerContainersLogic::pushButtonSsOpenVpnContInstallClicked,
                &ServerContainersLogic::pushButtonCloakOpenVpnContInstallClicked,
                &ServerContainersLogic::pushButtonWireguardContInstallClicked,
    };
    QList<ButtonSetEnabledFunc> installButtonsSetEnabledFunc {
        [this] (bool enabled) -> void {
            set_pushButtonOpenVpnContInstallEnabled(enabled);
        },
        [this] (bool enabled) -> void {
            set_pushButtonSsOpenVpnContInstallEnabled(enabled);
        },
        [this] (bool enabled) -> void {
            set_pushButtonCloakOpenVpnContInstallEnabled(enabled);
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
        &ServerContainersLogic::pushButtonOpenVpnContShareClicked,
                &ServerContainersLogic::pushButtonSsOpenVpnContShareClicked,
                &ServerContainersLogic::pushButtonCloakOpenVpnContShareClicked,
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


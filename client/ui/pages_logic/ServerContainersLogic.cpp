#include "ServerContainersLogic.h"
#include "ShareConnectionLogic.h"
#include "protocols/CloakLogic.h"
#include "protocols/OpenVpnLogic.h"
#include "protocols/ShadowSocksLogic.h"

#include "core/servercontroller.h"
#include <functional>

#include "../uilogic.h"

using namespace amnezia;
using namespace PageEnumNS;

ServerContainersLogic::ServerContainersLogic(UiLogic *uiLogic, QObject *parent):
    QObject(parent),
    m_uiLogic(uiLogic),
    m_pageServerContainersEnabled{true},
    m_progressBarProtocolsContainerReinstallValue{0},
    m_progressBarProtocolsContainerReinstallMaximium{100},
    m_pushButtonProtoOpenvpnContInstallChecked{false},
    m_pushButtonProtoSsOpenvpnContInstallChecked{false},
    m_pushButtonProtoCloakOpenvpnContInstallChecked{false},
    m_pushButtonProtoWireguardContInstallChecked{false},
    m_pushButtonProtoOpenvpnContInstallEnabled{false},
    m_pushButtonProtoSsOpenvpnContInstallEnabled{false},
    m_pushButtonProtoCloakOpenvpnContInstallEnabled{false},
    m_pushButtonProtoWireguardContInstallEnabled{false},
    m_pushButtonProtoOpenvpnContDefaultChecked{false},
    m_pushButtonProtoSsOpenvpnContDefaultChecked{false},
    m_pushButtonProtoCloakOpenvpnContDefaultChecked{false},
    m_pushButtonProtoWireguardContDefaultChecked{false},
    m_pushButtonProtoOpenvpnContDefaultVisible{true},
    m_pushButtonProtoSsOpenvpnContDefaultVisible{false},
    m_pushButtonProtoCloakOpenvpnContDefaultVisible{false},
    m_pushButtonProtoWireguardContDefaultVisible{false},
    m_pushButtonProtoOpenvpnContShareVisible{false},
    m_pushButtonProtoSsOpenvpnContShareVisible{false},
    m_pushButtonProtoCloakOpenvpnContShareVisible{false},
    m_pushButtonProtoWireguardContShareVisible{false},
    m_frameOpenvpnSettingsVisible{true},
    m_frameOpenvpnSsSettingsVisible{true},
    m_frameOpenvpnSsCloakSettingsVisible{true},
    m_progressBarProtocolsContainerReinstallVisible{false},
    m_frameWireguardSettingsVisible{false},
    m_frameWireguardVisible{false}
{
    setupProtocolsPageConnections();

    setFrameWireguardSettingsVisible(false);
    setFrameWireguardVisible(false);
}

void ServerContainersLogic::updateServerContainersPage()
{
    setProgressBarProtocolsContainerReinstallVisible(false);

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
        [this](bool checked) ->void {setPushButtonProtoOpenvpnContInstallChecked(checked);},
        [this](bool checked) ->void {setPushButtonProtoSsOpenvpnContInstallChecked(checked);},
        [this](bool checked) ->void {setPushButtonProtoCloakOpenvpnContInstallChecked(checked);},
        [this](bool checked) ->void {setPushButtonProtoWireguardContInstallChecked(checked);},
    };
    QList<SetEnabledFunc> installButtonsEnabledFunc {
        [this](bool enabled) ->void {setPushButtonProtoOpenvpnContInstallEnabled(enabled);},
        [this](bool enabled) ->void {setPushButtonProtoSsOpenvpnContInstallEnabled(enabled);},
        [this](bool enabled) ->void {setPushButtonProtoCloakOpenvpnContInstallEnabled(enabled);},
        [this](bool enabled) ->void {setPushButtonProtoWireguardContInstallEnabled(enabled);},
    };

    QList<SetCheckedFunc> defaultButtonsCheckedFunc {
        [this](bool checked) ->void {setPushButtonProtoOpenvpnContDefaultChecked(checked);},
        [this](bool checked) ->void {setPushButtonProtoSsOpenvpnContDefaultChecked(checked);},
        [this](bool checked) ->void {setPushButtonProtoCloakOpenvpnContDefaultChecked(checked);},
        [this](bool checked) ->void {setPushButtonProtoWireguardContDefaultChecked(checked);},
    };
    QList<SetVisibleFunc> defaultButtonsVisibleFunc {
        [this](bool visible) ->void {setPushButtonProtoOpenvpnContDefaultVisible(visible);},
        [this](bool visible) ->void {setPushButtonProtoSsOpenvpnContDefaultVisible(visible);},
        [this](bool visible) ->void {setPushButtonProtoCloakOpenvpnContDefaultVisible(visible);},
        [this](bool visible) ->void {setPushButtonProtoWireguardContDefaultVisible(visible);},
    };

    QList<SetVisibleFunc> shareButtonsVisibleFunc {
        [this](bool visible) ->void {setPushButtonProtoOpenvpnContShareVisible(visible);},
        [this](bool visible) ->void {setPushButtonProtoSsOpenvpnContShareVisible(visible);},
        [this](bool visible) ->void {setPushButtonProtoCloakOpenvpnContShareVisible(visible);},
        [this](bool visible) ->void {setPushButtonProtoWireguardContShareVisible(visible);},
    };

    QList<SetVisibleFunc> framesVisibleFunc {
        [this](bool visible) ->void {setFrameOpenvpnSettingsVisible(visible);},
        [this](bool visible) ->void {setFrameOpenvpnSsSettingsVisible(visible);},
        [this](bool visible) ->void {setFrameOpenvpnSsCloakSettingsVisible(visible);},
        [this](bool visible) ->void {setFrameWireguardSettingsVisible(visible);},
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
        &ServerContainersLogic::pushButtonProtoOpenvpnContDefaultClicked,
                &ServerContainersLogic::pushButtonProtoSsOpenvpnContDefaultClicked,
                &ServerContainersLogic::pushButtonProtoCloakOpenvpnContDefaultClicked,
                &ServerContainersLogic::pushButtonProtoWireguardContDefaultClicked
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
        &ServerContainersLogic::pushButtonProtoOpenvpnContInstallClicked,
                &ServerContainersLogic::pushButtonProtoSsOpenvpnContInstallClicked,
                &ServerContainersLogic::pushButtonProtoCloakOpenvpnContInstallClicked,
                &ServerContainersLogic::pushButtonProtoWireguardContInstallClicked,
    };
    QList<ButtonSetEnabledFunc> installButtonsSetEnabledFunc {
        [this] (bool enabled) -> void {
            setPushButtonProtoOpenvpnContInstallEnabled(enabled);
        },
        [this] (bool enabled) -> void {
            setPushButtonProtoSsOpenvpnContInstallEnabled(enabled);
        },
        [this] (bool enabled) -> void {
            setPushButtonProtoCloakOpenvpnContInstallEnabled(enabled);
        },
        [this] (bool enabled) -> void {
            setPushButtonProtoWireguardContInstallEnabled(enabled);
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
                    setPageServerContainersEnabled(enabled);
                };
                UiLogic::ButtonFunc no_button;
                UiLogic::LabelFunc no_label;
                UiLogic::ProgressFunc progressBar_protocols_container_reinstall;
                progressBar_protocols_container_reinstall.setVisibleFunc = [this] (bool visible) ->void {
                    setProgressBarProtocolsContainerReinstallVisible(visible);
                };
                progressBar_protocols_container_reinstall.setValueFunc = [this] (int value) ->void {
                    setProgressBarProtocolsContainerReinstallValue(value);
                };
                progressBar_protocols_container_reinstall.getValueFunc = [this] (void) -> int {
                    return getProgressBarProtocolsContainerReinstallValue();
                };
                progressBar_protocols_container_reinstall.getMaximiumFunc = [this] (void) -> int {
                    return getProgressBarProtocolsContainerReinstallMaximium();
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
        &ServerContainersLogic::pushButtonProtoOpenvpnContShareClicked,
                &ServerContainersLogic::pushButtonProtoSsOpenvpnContShareClicked,
                &ServerContainersLogic::pushButtonProtoCloakOpenvpnContShareClicked,
                &ServerContainersLogic::pushButtonProtoWireguardContShareClicked,
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

bool ServerContainersLogic::getPageServerContainersEnabled() const
{
    return m_pageServerContainersEnabled;
}

void ServerContainersLogic::setPageServerContainersEnabled(bool pageServerContainersEnabled)
{
    if (m_pageServerContainersEnabled != pageServerContainersEnabled) {
        m_pageServerContainersEnabled = pageServerContainersEnabled;
        emit pageServerContainersEnabledChanged();
    }
}

int ServerContainersLogic::getProgressBarProtocolsContainerReinstallValue() const
{
    return m_progressBarProtocolsContainerReinstallValue;
}

void ServerContainersLogic::setProgressBarProtocolsContainerReinstallValue(int progressBarProtocolsContainerReinstallValue)
{
    if (m_progressBarProtocolsContainerReinstallValue != progressBarProtocolsContainerReinstallValue) {
        m_progressBarProtocolsContainerReinstallValue = progressBarProtocolsContainerReinstallValue;
        emit progressBarProtocolsContainerReinstallValueChanged();
    }
}

int ServerContainersLogic::getProgressBarProtocolsContainerReinstallMaximium() const
{
    return m_progressBarProtocolsContainerReinstallMaximium;
}

void ServerContainersLogic::setProgressBarProtocolsContainerReinstallMaximium(int progressBarProtocolsContainerReinstallMaximium)
{
    if (m_progressBarProtocolsContainerReinstallMaximium != progressBarProtocolsContainerReinstallMaximium) {
        m_progressBarProtocolsContainerReinstallMaximium = progressBarProtocolsContainerReinstallMaximium;
        emit progressBarProtocolsContainerReinstallMaximiumChanged();
    }
}

bool ServerContainersLogic::getPushButtonProtoOpenvpnContInstallChecked() const
{
    return m_pushButtonProtoOpenvpnContInstallChecked;
}

void ServerContainersLogic::setPushButtonProtoOpenvpnContInstallChecked(bool pushButtonProtoOpenvpnContInstallChecked)
{
    if (m_pushButtonProtoOpenvpnContInstallChecked != pushButtonProtoOpenvpnContInstallChecked) {
        m_pushButtonProtoOpenvpnContInstallChecked = pushButtonProtoOpenvpnContInstallChecked;
        emit pushButtonProtoOpenvpnContInstallCheckedChanged();
    }
}

bool ServerContainersLogic::getPushButtonProtoSsOpenvpnContInstallChecked() const
{
    return m_pushButtonProtoSsOpenvpnContInstallChecked;
}

void ServerContainersLogic::setPushButtonProtoSsOpenvpnContInstallChecked(bool pushButtonProtoSsOpenvpnContInstallChecked)
{
    if (m_pushButtonProtoSsOpenvpnContInstallChecked != pushButtonProtoSsOpenvpnContInstallChecked) {
        m_pushButtonProtoSsOpenvpnContInstallChecked = pushButtonProtoSsOpenvpnContInstallChecked;
        emit pushButtonProtoSsOpenvpnContInstallCheckedChanged();
    }
}

bool ServerContainersLogic::getPushButtonProtoCloakOpenvpnContInstallChecked() const
{
    return m_pushButtonProtoCloakOpenvpnContInstallChecked;
}

void ServerContainersLogic::setPushButtonProtoCloakOpenvpnContInstallChecked(bool pushButtonProtoCloakOpenvpnContInstallChecked)
{
    if (m_pushButtonProtoCloakOpenvpnContInstallChecked != pushButtonProtoCloakOpenvpnContInstallChecked) {
        m_pushButtonProtoCloakOpenvpnContInstallChecked = pushButtonProtoCloakOpenvpnContInstallChecked;
        emit pushButtonProtoCloakOpenvpnContInstallCheckedChanged();
    }
}

bool ServerContainersLogic::getPushButtonProtoWireguardContInstallChecked() const
{
    return m_pushButtonProtoWireguardContInstallChecked;
}

void ServerContainersLogic::setPushButtonProtoWireguardContInstallChecked(bool pushButtonProtoWireguardContInstallChecked)
{
    if (m_pushButtonProtoWireguardContInstallChecked != pushButtonProtoWireguardContInstallChecked) {
        m_pushButtonProtoWireguardContInstallChecked = pushButtonProtoWireguardContInstallChecked;
        emit pushButtonProtoWireguardContInstallCheckedChanged();
    }
}

bool ServerContainersLogic::getPushButtonProtoOpenvpnContInstallEnabled() const
{
    return m_pushButtonProtoOpenvpnContInstallEnabled;
}

void ServerContainersLogic::setPushButtonProtoOpenvpnContInstallEnabled(bool pushButtonProtoOpenvpnContInstallEnabled)
{
    if (m_pushButtonProtoOpenvpnContInstallEnabled != pushButtonProtoOpenvpnContInstallEnabled) {
        m_pushButtonProtoOpenvpnContInstallEnabled = pushButtonProtoOpenvpnContInstallEnabled;
        emit pushButtonProtoOpenvpnContInstallEnabledChanged();
    }
}

bool ServerContainersLogic::getPushButtonProtoSsOpenvpnContInstallEnabled() const
{
    return m_pushButtonProtoSsOpenvpnContInstallEnabled;
}

void ServerContainersLogic::setPushButtonProtoSsOpenvpnContInstallEnabled(bool pushButtonProtoSsOpenvpnContInstallEnabled)
{
    if (m_pushButtonProtoSsOpenvpnContInstallEnabled != pushButtonProtoSsOpenvpnContInstallEnabled) {
        m_pushButtonProtoSsOpenvpnContInstallEnabled = pushButtonProtoSsOpenvpnContInstallEnabled;
        emit pushButtonProtoSsOpenvpnContInstallEnabledChanged();
    }
}

bool ServerContainersLogic::getPushButtonProtoCloakOpenvpnContInstallEnabled() const
{
    return m_pushButtonProtoCloakOpenvpnContInstallEnabled;
}

void ServerContainersLogic::setPushButtonProtoCloakOpenvpnContInstallEnabled(bool pushButtonProtoCloakOpenvpnContInstallEnabled)
{
    if (m_pushButtonProtoCloakOpenvpnContInstallEnabled != pushButtonProtoCloakOpenvpnContInstallEnabled) {
        m_pushButtonProtoCloakOpenvpnContInstallEnabled = pushButtonProtoCloakOpenvpnContInstallEnabled;
        emit pushButtonProtoCloakOpenvpnContInstallEnabledChanged();
    }
}

bool ServerContainersLogic::getPushButtonProtoWireguardContInstallEnabled() const
{
    return m_pushButtonProtoWireguardContInstallEnabled;
}

void ServerContainersLogic::setPushButtonProtoWireguardContInstallEnabled(bool pushButtonProtoWireguardContInstallEnabled)
{
    if (m_pushButtonProtoWireguardContInstallEnabled != pushButtonProtoWireguardContInstallEnabled) {
        m_pushButtonProtoWireguardContInstallEnabled = pushButtonProtoWireguardContInstallEnabled;
        emit pushButtonProtoWireguardContInstallEnabledChanged();
    }
}

bool ServerContainersLogic::getPushButtonProtoOpenvpnContDefaultChecked() const
{
    return m_pushButtonProtoOpenvpnContDefaultChecked;
}

void ServerContainersLogic::setPushButtonProtoOpenvpnContDefaultChecked(bool pushButtonProtoOpenvpnContDefaultChecked)
{
    if (m_pushButtonProtoOpenvpnContDefaultChecked != pushButtonProtoOpenvpnContDefaultChecked) {
        m_pushButtonProtoOpenvpnContDefaultChecked = pushButtonProtoOpenvpnContDefaultChecked;
        emit pushButtonProtoOpenvpnContDefaultCheckedChanged();
    }
}

bool ServerContainersLogic::getPushButtonProtoSsOpenvpnContDefaultChecked() const
{
    return m_pushButtonProtoSsOpenvpnContDefaultChecked;
}

void ServerContainersLogic::setPushButtonProtoSsOpenvpnContDefaultChecked(bool pushButtonProtoSsOpenvpnContDefaultChecked)
{
    if (m_pushButtonProtoSsOpenvpnContDefaultChecked != pushButtonProtoSsOpenvpnContDefaultChecked) {
        m_pushButtonProtoSsOpenvpnContDefaultChecked = pushButtonProtoSsOpenvpnContDefaultChecked;
        emit pushButtonProtoSsOpenvpnContDefaultCheckedChanged();
    }
}

bool ServerContainersLogic::getPushButtonProtoCloakOpenvpnContDefaultChecked() const
{
    return m_pushButtonProtoCloakOpenvpnContDefaultChecked;
}

void ServerContainersLogic::setPushButtonProtoCloakOpenvpnContDefaultChecked(bool pushButtonProtoCloakOpenvpnContDefaultChecked)
{
    if (m_pushButtonProtoCloakOpenvpnContDefaultChecked != pushButtonProtoCloakOpenvpnContDefaultChecked) {
        m_pushButtonProtoCloakOpenvpnContDefaultChecked = pushButtonProtoCloakOpenvpnContDefaultChecked;
        emit pushButtonProtoCloakOpenvpnContDefaultCheckedChanged();
    }
}

bool ServerContainersLogic::getPushButtonProtoWireguardContDefaultChecked() const
{
    return m_pushButtonProtoWireguardContDefaultChecked;
}

void ServerContainersLogic::setPushButtonProtoWireguardContDefaultChecked(bool pushButtonProtoWireguardContDefaultChecked)
{
    if (m_pushButtonProtoWireguardContDefaultChecked != pushButtonProtoWireguardContDefaultChecked) {
        m_pushButtonProtoWireguardContDefaultChecked = pushButtonProtoWireguardContDefaultChecked;
        emit pushButtonProtoWireguardContDefaultCheckedChanged();
    }
}

bool ServerContainersLogic::getPushButtonProtoOpenvpnContDefaultVisible() const
{
    return m_pushButtonProtoOpenvpnContDefaultVisible;
}

void ServerContainersLogic::setPushButtonProtoOpenvpnContDefaultVisible(bool pushButtonProtoOpenvpnContDefaultVisible)
{
    if (m_pushButtonProtoOpenvpnContDefaultVisible != pushButtonProtoOpenvpnContDefaultVisible) {
        m_pushButtonProtoOpenvpnContDefaultVisible = pushButtonProtoOpenvpnContDefaultVisible;
        emit pushButtonProtoOpenvpnContDefaultVisibleChanged();
    }
}

bool ServerContainersLogic::getPushButtonProtoSsOpenvpnContDefaultVisible() const
{
    return m_pushButtonProtoSsOpenvpnContDefaultVisible;
}

void ServerContainersLogic::setPushButtonProtoSsOpenvpnContDefaultVisible(bool pushButtonProtoSsOpenvpnContDefaultVisible)
{
    if (m_pushButtonProtoSsOpenvpnContDefaultVisible != pushButtonProtoSsOpenvpnContDefaultVisible) {
        m_pushButtonProtoSsOpenvpnContDefaultVisible = pushButtonProtoSsOpenvpnContDefaultVisible;
        emit pushButtonProtoSsOpenvpnContDefaultVisibleChanged();
    }
}

bool ServerContainersLogic::getPushButtonProtoCloakOpenvpnContDefaultVisible() const
{
    return m_pushButtonProtoCloakOpenvpnContDefaultVisible;
}

void ServerContainersLogic::setPushButtonProtoCloakOpenvpnContDefaultVisible(bool pushButtonProtoCloakOpenvpnContDefaultVisible)
{
    if (m_pushButtonProtoCloakOpenvpnContDefaultVisible != pushButtonProtoCloakOpenvpnContDefaultVisible) {
        m_pushButtonProtoCloakOpenvpnContDefaultVisible = pushButtonProtoCloakOpenvpnContDefaultVisible;
        emit pushButtonProtoCloakOpenvpnContDefaultVisibleChanged();
    }
}

bool ServerContainersLogic::getPushButtonProtoWireguardContDefaultVisible() const
{
    return m_pushButtonProtoWireguardContDefaultVisible;
}

void ServerContainersLogic::setPushButtonProtoWireguardContDefaultVisible(bool pushButtonProtoWireguardContDefaultVisible)
{
    if (m_pushButtonProtoWireguardContDefaultVisible != pushButtonProtoWireguardContDefaultVisible) {
        m_pushButtonProtoWireguardContDefaultVisible = pushButtonProtoWireguardContDefaultVisible;
        emit pushButtonProtoWireguardContDefaultVisibleChanged();
    }
}

bool ServerContainersLogic::getPushButtonProtoOpenvpnContShareVisible() const
{
    return m_pushButtonProtoOpenvpnContShareVisible;
}

void ServerContainersLogic::setPushButtonProtoOpenvpnContShareVisible(bool pushButtonProtoOpenvpnContShareVisible)
{
    if (m_pushButtonProtoOpenvpnContShareVisible != pushButtonProtoOpenvpnContShareVisible) {
        m_pushButtonProtoOpenvpnContShareVisible = pushButtonProtoOpenvpnContShareVisible;
        emit pushButtonProtoOpenvpnContShareVisibleChanged();
    }
}

bool ServerContainersLogic::getPushButtonProtoSsOpenvpnContShareVisible() const
{
    return m_pushButtonProtoSsOpenvpnContShareVisible;
}

void ServerContainersLogic::setPushButtonProtoSsOpenvpnContShareVisible(bool pushButtonProtoSsOpenvpnContShareVisible)
{
    if (m_pushButtonProtoSsOpenvpnContShareVisible != pushButtonProtoSsOpenvpnContShareVisible) {
        m_pushButtonProtoSsOpenvpnContShareVisible = pushButtonProtoSsOpenvpnContShareVisible;
        emit pushButtonProtoSsOpenvpnContShareVisibleChanged();
    }
}

bool ServerContainersLogic::getPushButtonProtoCloakOpenvpnContShareVisible() const
{
    return m_pushButtonProtoCloakOpenvpnContShareVisible;
}

void ServerContainersLogic::setPushButtonProtoCloakOpenvpnContShareVisible(bool pushButtonProtoCloakOpenvpnContShareVisible)
{
    if (m_pushButtonProtoCloakOpenvpnContShareVisible != pushButtonProtoCloakOpenvpnContShareVisible) {
        m_pushButtonProtoCloakOpenvpnContShareVisible = pushButtonProtoCloakOpenvpnContShareVisible;
        emit pushButtonProtoCloakOpenvpnContShareVisibleChanged();
    }
}

bool ServerContainersLogic::getPushButtonProtoWireguardContShareVisible() const
{
    return m_pushButtonProtoWireguardContShareVisible;
}

void ServerContainersLogic::setPushButtonProtoWireguardContShareVisible(bool pushButtonProtoWireguardContShareVisible)
{
    if (m_pushButtonProtoWireguardContShareVisible != pushButtonProtoWireguardContShareVisible) {
        m_pushButtonProtoWireguardContShareVisible = pushButtonProtoWireguardContShareVisible;
        emit pushButtonProtoWireguardContShareVisibleChanged();
    }
}

bool ServerContainersLogic::getFrameOpenvpnSettingsVisible() const
{
    return m_frameOpenvpnSettingsVisible;
}

void ServerContainersLogic::setFrameOpenvpnSettingsVisible(bool frameOpenvpnSettingsVisible)
{
    if (m_frameOpenvpnSettingsVisible != frameOpenvpnSettingsVisible) {
        m_frameOpenvpnSettingsVisible = frameOpenvpnSettingsVisible;
        emit frameOpenvpnSettingsVisibleChanged();
    }
}

bool ServerContainersLogic::getFrameOpenvpnSsSettingsVisible() const
{
    return m_frameOpenvpnSsSettingsVisible;
}

void ServerContainersLogic::setFrameOpenvpnSsSettingsVisible(bool frameOpenvpnSsSettingsVisible)
{
    if (m_frameOpenvpnSsSettingsVisible != frameOpenvpnSsSettingsVisible) {
        m_frameOpenvpnSsSettingsVisible = frameOpenvpnSsSettingsVisible;
        emit frameOpenvpnSsSettingsVisibleChanged();
    }
}

bool ServerContainersLogic::getFrameOpenvpnSsCloakSettingsVisible() const
{
    return m_frameOpenvpnSsCloakSettingsVisible;
}

void ServerContainersLogic::setFrameOpenvpnSsCloakSettingsVisible(bool frameOpenvpnSsCloakSettingsVisible)
{
    if (m_frameOpenvpnSsCloakSettingsVisible != frameOpenvpnSsCloakSettingsVisible) {
        m_frameOpenvpnSsCloakSettingsVisible = frameOpenvpnSsCloakSettingsVisible;
        emit frameOpenvpnSsCloakSettingsVisibleChanged();
    }
}

bool ServerContainersLogic::getProgressBarProtocolsContainerReinstallVisible() const
{
    return m_progressBarProtocolsContainerReinstallVisible;
}

void ServerContainersLogic::setProgressBarProtocolsContainerReinstallVisible(bool progressBarProtocolsContainerReinstallVisible)
{
    if (m_progressBarProtocolsContainerReinstallVisible != progressBarProtocolsContainerReinstallVisible) {
        m_progressBarProtocolsContainerReinstallVisible = progressBarProtocolsContainerReinstallVisible;
        emit progressBarProtocolsContainerReinstallVisibleChanged();
    }
}

bool ServerContainersLogic::getFrameWireguardSettingsVisible() const
{
    return m_frameWireguardSettingsVisible;
}

void ServerContainersLogic::setFrameWireguardSettingsVisible(bool frameWireguardSettingsVisible)
{
    if (m_frameWireguardSettingsVisible != frameWireguardSettingsVisible) {
        m_frameWireguardSettingsVisible = frameWireguardSettingsVisible;
        emit frameWireguardSettingsVisibleChanged();
    }
}

bool ServerContainersLogic::getFrameWireguardVisible() const
{
    return m_frameWireguardVisible;
}

void ServerContainersLogic::setFrameWireguardVisible(bool frameWireguardVisible)
{
    if (m_frameWireguardVisible != frameWireguardVisible) {
        m_frameWireguardVisible = frameWireguardVisible;
        emit frameWireguardVisibleChanged();
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

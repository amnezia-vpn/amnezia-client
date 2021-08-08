import QtQuick 2.14
import QtQuick.Window 2.14
import QtQuick.Controls 2.12
import PageEnum 1.0
import Qt.labs.platform 1.1
import QtQuick.Dialogs 1.1
import "./"

Window {
    id: root
    visible: true
    width: GC.screenWidth
    height: GC.isDesktop() ? GC.screenHeight + titleBar.height : GC.screenHeight
    onClosing: {
        UiLogic.onCloseWindow()
    }

    flags: Qt.FramelessWindowHint
    title: "AmneziaVPN"
    function getPageComponent(page) {
        switch (page) {
        case PageEnum.Start:
            return page_start;
        case PageEnum.NewServer:
            return page_new_server
        case PageEnum.NewServerProtocols:
            return page_new_server_protocols
        case PageEnum.Wizard:
            return page_setup_wizard
        case PageEnum.WizardHigh:
            return page_setup_wizard_high_level
        case PageEnum.WizardLow:
            return page_setup_wizard_low_level
        case PageEnum.WizardMedium:
            return page_setup_wizard_medium_level
        case PageEnum.WizardVpnMode:
            return page_setup_wizard_vpn_mode
        case PageEnum.ServerConfiguring:
            return page_new_server_configuring
        case PageEnum.Vpn:
            return page_vpn
        case PageEnum.GeneralSettings:
            return page_general_settings
        case PageEnum.AppSettings:
            return page_app_settings
        case PageEnum.NetworkSettings:
            return page_network_settings
        case PageEnum.ServerSettings:
            return page_server_settings
        case PageEnum.ServerVpnProtocols:
            return page_server_protocols
        case PageEnum.ServersList:
            return page_servers
        case PageEnum.ShareConnection:
            return page_share_connection
        case PageEnum.Sites:
            return page_sites
        case PageEnum.OpenVpnSettings:
            return page_proto_openvpn
        case PageEnum.ShadowSocksSettings:
            return page_proto_shadowsocks
        case PageEnum.CloakSettings:
            return page_proto_cloak
        }
        return undefined;
    }

    function getPageEnum(item) {
        if (item instanceof PageStart) {
            return PageEnum.Start
        }
        if (item instanceof PageNewServer) {
            return PageEnum.NewServer
        }
        if (item instanceof PageNewServerProtocol) {
            return PageEnum.NewServerProtocols
        }
        if (item instanceof PageSetupWizard) {
            return PageEnum.Wizard
        }
        if (item instanceof PageSetupWizardHighLevel) {
            return PageEnum.WizardHigh
        }
        if (item instanceof PageSetupWizardLowLevel) {
            return PageEnum.WizardLow
        }
        if (item instanceof PageSetupWizardMediumLevel) {
            return PageEnum.WizardMedium
        }
        if (item instanceof PageSetupWizardVPNMode) {
            return PageEnum.WizardVpnMode
        }
        if (item instanceof PageNewServerConfiguring) {
            return PageEnum.ServerConfiguring
        }
        if (item instanceof PageVPN) {
            return PageEnum.Vpn
        }
        if (item instanceof PageGeneralSettings) {
            return PageEnum.GeneralSettings
        }
        if (item instanceof PageAppSetting) {
            return PageEnum.AppSettings
        }
        if (item instanceof PageNetworkSetting) {
            return PageEnum.NetworkSettings
        }
        if (item instanceof PageServerSetting) {
            return PageEnum.ServerSettings
        }
        if (item instanceof PageServerProtocols) {
            return PageEnum.ServerVpnProtocols
        }
        if (item instanceof PageServer) {
            return PageEnum.ServersList
        }
        if (item instanceof PageShareConnection) {
            return PageEnum.ShareConnection
        }
        if (item instanceof PageSites) {
            return PageEnum.Sites
        }
        if (item instanceof PageProtoOpenVPN) {
            return PageEnum.OpenVpnSettings
        }
        if (item instanceof PageProtoShadowSock) {
            return PageEnum.ShadowSocksSettings
        }
        if (item instanceof PageProtoCloak) {
            return PageEnum.CloakSettings
        }
        return PageEnum.Start
    }

    function gotoPage(page, reset, slide) {
        let pageComponent = getPageComponent(page)
        console.debug(pageComponent)
        if (reset) {
            if (page === PageEnum.ServerSettings) {
                UiLogic.updateServerPage();
            }
            if (page === PageEnum.ShareConnection) {}
            if (page === PageEnum.Wizard) {
                UiLogic.radioButtonSetupWizardMediumChecked = true
            }
            if (page === PageEnum.WizardHigh) {
                UiLogic.updateWizardHighPage();
            }
            if (page === PageEnum.ServerConfiguring) {
                UiLogic.progressBarNewServerConfiguringValue = 0;
            }
            if (page === PageEnum.GeneralSettings) {
                UiLogic.updateGeneralSettingPage();
            }
            if (page === PageEnum.ServersList) {
                UiLogic.updateServersListPage();
            }
            if (page === PageEnum.Start) {
                UiLogic.pushButtonBackFromStartVisible = !pageLoader.empty
                UiLogic.updateStartPage();
            }
            if (page === PageEnum.NewServerProtocols) {
                UiLogic.updateNewServerProtocolsPage()
            }
            if (page === PageEnum.ServerVpnProtocols) {
                UiLogic.updateProtocolsPage()
            }
            if (page === PageEnum.AppSettings) {
                UiLogic.updateAppSettingsPage()
            }
            if (page === PageEnum.NetworkSettings) {
                UiLogic.updateAppSettingsPage()
            }
            if (page === PageEnum.Sites) {
                UiLogic.updateSitesPage()
            }
            if (page === PageEnum.Vpn) {
                UiLogic.updateVpnPage()
            }
            UiLogic.pushButtonNewServerConnectKeyChecked = false
        }
        if (slide) {
            pageLoader.push(pageComponent, {}, StackView.PushTransition)
        } else {
            pageLoader.push(pageComponent, {}, StackView.Immediate)
        }
    }

    function close_page() {
        if (pageLoader.depth <= 1) {
            return
        }
        pageLoader.pop()
    }

    function set_start_page(page, slide) {
        pageLoader.clear()
        let pageComponent = getPageComponent(page)
        if (slide) {
            pageLoader.push(pageComponent, {}, StackView.PushTransition)
        } else {
            pageLoader.push(pageComponent, {}, StackView.Immediate)
        }
        if (page === PageEnum.Start) {
            UiLogic.pushButtonBackFromStartVisible = !pageLoader.empty
            UiLogic.updateStartPage();
        }
    }

    TitleBar {
        id: titleBar
        anchors.top: root.top
        visible: GC.isDesktop()
        DragHandler {
            grabPermissions: TapHandler.CanTakeOverFromAnything
            onActiveChanged: {
                if (active) {
                    root.startSystemMove();
                }
            }
            target: null
        }
        onCloseButtonClicked: {
            if (UiLogic.currentPageValue === PageEnum.Start ||
                    UiLogic.currentPageValue === PageEnum.NewServer) {
                Qt.quit()
            } else {
                root.hide()
            }
        }
    }

    Rectangle {
        y: GC.isDesktop() ? titleBar.height : 0
        width: GC.screenWidth
        height: GC.screenHeight
        color: "white"
    }

    StackView {
        id: pageLoader
        y: GC.isDesktop() ? titleBar.height : 0
        width: GC.screenWidth
        height: GC.screenHeight
//        initialItem: page_servers
        onCurrentItemChanged: {
            let pageEnum = root.getPageEnum(currentItem)
            UiLogic.currentPageValue = pageEnum
        }
    }
    Component {
        id: page_start
        PageStart {}
    }
    Component {
        id: page_new_server
        PageNewServer {}
    }
    Component {
        id: page_setup_wizard
        PageSetupWizard {}
    }
    Component {
        id: page_setup_wizard_high_level
        PageSetupWizardHighLevel {}
    }
    Component {
        id: page_setup_wizard_vpn_mode
        PageSetupWizardVPNMode {}
    }
    Component {
        id: page_setup_wizard_medium_level
        PageSetupWizardMediumLevel {}
    }
    Component {
        id: page_setup_wizard_low_level
        PageSetupWizardLowLevel {}
    }
    Component {
        id: page_new_server_protocols
        PageNewServerProtocol {}
    }
    Component {
        id: page_vpn
        PageVPN {}
    }
    Component {
        id: page_sites
        PageSites {}
    }
    Component {
        id: page_general_settings
        PageGeneralSettings {}
    }
    Component {
        id: page_servers
        PageServer {}
    }
    Component {
        id: page_app_settings
        PageAppSetting {}
    }
    Component {
        id: page_network_settings
        PageNetworkSetting {}
    }
    Component {
        id: page_server_settings
        PageServerSetting {}
    }
    Component {
        id: page_server_protocols
        PageServerProtocols {}
    }
    Component {
        id: page_share_connection
        PageShareConnection {}
    }
    Component {
        id: page_proto_openvpn
        PageProtoOpenVPN {}
    }
    Component {
        id: page_proto_shadowsocks
        PageProtoShadowSock {}
    }
    Component {
        id: page_proto_cloak
        PageProtoCloak {}
    }
    Component {
        id: page_new_server_configuring
        PageNewServerConfiguring {}
    }

    Component.onCompleted: {
        UiLogic.initalizeUiLogic()
    }


    Connections {
        target: UiLogic
        onGoToPage: {
            root.gotoPage(page, reset, slide)
        }
        onClosePage: {
            root.close_page()
        }
        onSetStartPage: {
            root.set_start_page(page, slide)
        }
        onShowPublicKeyWarning: {
            publicKeyWarning.visible = true
        }
        onShowConnectErrorDialog: {
            connectErrorDialog.visible = true
        }
        onShow: {
            root.show()
        }
        onHide: {
            root.hide()
        }
    }
    MessageDialog {
        id: closePrompt
//        x: (root.width - width) / 2
//        y: (root.height - height) / 2
        title: qsTr("Exit")
        text: qsTr("Do you really want to quit?")
        standardButtons: StandardButton.Yes | StandardButton.No
        onYes: {
            Qt.quit()
        }
        visible: false
    }
    SystemTrayIcon {
        visible: true
        icon.source: UiLogic.trayIconUrl
        onActivated: {
            if (Qt.platform.os == "osx" ||
                    Qt.platform.os == "linux") {
                if (reason === SystemTrayIcon.DoubleClick ||
                        reason === SystemTrayIcon.Trigger) {
                    root.show()
                    root.raise()
                    root.requestActivate()
                }
            }
        }

        menu: Menu {
            MenuItem {
                iconSource: "qrc:/images/tray/application.png"
                text: qsTr("Show") + " " + "AmneziaVPN"
                onTriggered: {
                    root.show()
                    root.raise()
                }
            }
            MenuSeparator { }
            MenuItem {
                text: qsTr("Connect")
                enabled: UiLogic.trayActionConnectEnabled
                onTriggered: {
                    UiLogic.onConnect()
                }
            }
            MenuItem {
                text: qsTr("Disconnect")
                enabled: UiLogic.trayActionDisconnectEnabled
                onTriggered: {
                    UiLogic.onDisconnect()
                }
            }
            MenuSeparator { }
            MenuItem {
                iconSource: "qrc:/images/tray/link.png"
                text: qsTr("Visit Website")
                onTriggered: {
                    Qt.openUrlExternally("https://amnezia.org")
                }
            }
            MenuItem {
                iconSource: "qrc:/images/tray/cancel.png"
                text: qsTr("Quit") + " " + "AmneziaVPN"
                onTriggered: {
                    closePrompt.open()
                }
            }
        }
    }
    MessageDialog {
        id: publicKeyWarning
        title: "AmneziaVPN"
        text: qsTr("It's public key. Private key required")
        visible: false
    }
    MessageDialog {
        id: connectErrorDialog
        title: "AmneziaVPN"
        text: UiLogic.dialogConnectErrorText
        visible: false
    }

}

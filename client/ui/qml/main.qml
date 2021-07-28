import QtQuick 2.14
import QtQuick.Window 2.14
import QtQuick.Controls 2.12
import Page 1.0
import Qt.labs.platform 1.1
import QtQuick.Dialogs 1.1
import "./"

Window {
    id: root
    visible: true
    width: GC.screenWidth
    height: GC.isDesktop() ? GC.screenHeight + titleBar.height : GC.screenHeight
    //    flags: Qt.FramelessWindowHint
    title: "AmneziaVPN"
    function getPageComponent(page) {
        switch (page) {
        case Page.Start:
            return page_start;
        case Page.NewServer:
            return page_new_server
        case Page.NewServerProtocols:
            return page_new_server_protocols
        case Page.Wizard:
            return page_setup_wizard
        case Page.WizardHigh:
            return page_setup_wizard_high_level
        case Page.WizardLow:
            return page_setup_wizard_low_level
        case Page.WizardMedium:
            return page_setup_wizard_medium_level
        case Page.WizardVpnMode:
            return page_setup_wizard_vpn_mode
        case Page.ServerConfiguring:
            return page_new_server_configuring
        case Page.Vpn:
            return page_vpn
        case Page.GeneralSettings:
            return page_general_settings
        case Page.AppSettings:
            return page_app_settings
        case Page.NetworkSettings:
            return page_network_settings
        case Page.ServerSettings:
            return page_server_settings
        case Page.ServerVpnProtocols:
            return page_server_protocols
        case Page.ServersList:
            return page_servers
        case Page.ShareConnection:
            return page_share_connection
        case Page.Sites:
            return page_sites
        case Page.OpenVpnSettings:
            return page_proto_openvpn
        case Page.ShadowSocksSettings:
            return page_proto_shadowsocks
        case Page.CloakSettings:
            return page_proto_cloak
        }
        return undefined;
    }

    function getPageEnum(item) {
        if (item instanceof PageStart) {
            return Page.Start
        }
        if (item instanceof PageNewServer) {
            return Page.NewServer
        }
        if (item instanceof PageNewServerProtocol) {
            return Page.NewServerProtocols
        }
        if (item instanceof PageSetupWizard) {
            return Page.Wizard
        }
        if (item instanceof PageSetupWizardHighLevel) {
            return Page.WizardHigh
        }
        if (item instanceof PageSetupWizardLowLevel) {
            return Page.WizardLow
        }
        if (item instanceof PageSetupWizardMediumLevel) {
            return Page.WizardMedium
        }
        if (item instanceof PageSetupWizardVPNMode) {
            return Page.WizardVpnMode
        }
        if (item instanceof PageNewServerConfiguring) {
            return Page.ServerConfiguring
        }
        if (item instanceof PageVPN) {
            return Page.Vpn
        }
        if (item instanceof PageGeneralSettings) {
            return Page.GeneralSettings
        }
        if (item instanceof PageAppSetting) {
            return Page.AppSettings
        }
        if (item instanceof PageNetworkSetting) {
            return Page.NetworkSettings
        }
        if (item instanceof PageServerSetting) {
            return Page.ServerSettings
        }
        if (item instanceof PageServerProtocols) {
            return Page.ServerVpnProtocols
        }
        if (item instanceof PageServer) {
            return Page.ServersList
        }
        if (item instanceof PageShareConnection) {
            return Page.ShareConnection
        }
        if (item instanceof PageSites) {
            return Page.Sites
        }
        if (item instanceof PageProtoOpenVPN) {
            return Page.OpenVpnSettings
        }
        if (item instanceof PageProtoShadowSock) {
            return Page.ShadowSocksSettings
        }
        if (item instanceof PageProtoCloak) {
            return Page.CloakSettings
        }
        return Page.Start
    }

    function gotoPage(page, reset, slide) {
        let pageComponent = getPageComponent(page)
        if (reset) {
            if (page === Page.ServerSettings) {
                UiLogic.updateServerPage();
            }
            if (page === Page.ShareConnection) {}
            if (page === Page.Wizard) {
                UiLogic.radioButtonSetupWizardMediumChecked = true
            }
            if (page === Page.WizardHigh) {
                UiLogic.updateWizardHighPage();
            }
            if (page === Page.ServerConfiguring) {
                UiLogic.progressBarNewServerConfiguringValue = 0;
            }
            if (page === Page.GeneralSettings) {
                UiLogic.updateGeneralSettingPage();
            }
            if (page === Page.ServersList) {
                UiLogic.updateServersListPage();
            }
            if (page === Page.Start) {
                UiLogic.pushButtonBackFromStartVisible = !pageLoader.empty
                UiLogic.updateStartPage();
            }
            if (page === Page.NewServerProtocols) {
                UiLogic.updateNewServerProtocolsPage()
            }
            if (page === Page.ServerVpnProtocols) {
                UiLogic.updateProtocolsPage()
            }
            if (page === Page.AppSettings) {
                UiLogic.updateAppSettingsPage()
            }
            if (page === Page.NetworkSettings) {
                UiLogic.updateAppSettingsPage()
            }
            if (page === Page.Sites) {
                UiLogic.updateSitesPage()
            }
            if (page === Page.Vpn) {
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
        if (page === Page.Start) {
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
            if (UiLogic.currentPageValue === Page.Start ||
                    UiLogic.currentPageValue === Page.NewServer) {
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
        initialItem: page_vpn
        onCurrentItemChanged: {
            let pageEnum = root.getPageEnum(currentItem)
            console.debug(pageEnum)
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
}

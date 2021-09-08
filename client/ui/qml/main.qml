import QtQuick 2.14
import QtQuick.Window 2.14
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12
import PageEnum 1.0
import Qt.labs.platform 1.1
import QtQuick.Dialogs 1.1
import "./"
import "Pages"
import "Pages/Protocols"
import "Config"

Window {
    Material.theme: Material.Dark
    Material.accent: Material.Purple

    id: root
    visible: true
    width: GC.screenWidth
    height: GC.isDesktop() ? GC.screenHeight + titleBar.height : GC.screenHeight
    onClosing: {
        UiLogic.onCloseWindow()
    }

    //flags: Qt.FramelessWindowHint
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
        case PageEnum.ServerContainers:
            return page_server_containers
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
        if (item instanceof PageNewServerProtocols) {
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
        if (item instanceof PageServerSettings) {
            return PageEnum.ServerSettings
        }
        if (item instanceof PageServerContainers) {
            return PageEnum.ServerContainers
        }
        if (item instanceof PageServerList) {
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
        if (item instanceof PageProtoShadowSocks) {
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
                ServerSettingsLogic.updatePage();
            }
            if (page === PageEnum.ShareConnection) {
            }
            if (page === PageEnum.Wizard) {
                WizardLogic.radioButtonMediumChecked = true
            }
            if (page === PageEnum.WizardHigh) {
                WizardLogic.updatePage();
            }
            if (page === PageEnum.ServerConfiguring) {
                ServerConfiguringLogic.progressBarValue = 0;
            }
            if (page === PageEnum.GeneralSettings) {
                GeneralSettingsLogic.updatePage();
            }
            if (page === PageEnum.ServersList) {
                ServerListLogic.updatePage();
            }
            if (page === PageEnum.Start) {
                StartPageLogic.pushButtonBackFromStartVisible = !pageLoader.empty
                StartPageLogic.updatePage();
            }
            if (page === PageEnum.NewServerProtocols) {
                NewServerProtocolsLogic.updatePage()
            }
            if (page === PageEnum.ServerContainers) {
                ServerContainersLogic.updateServerContainersPage()
            }
            if (page === PageEnum.AppSettings) {
                AppSettingsLogic.updatePage()
            }
            if (page === PageEnum.NetworkSettings) {
                NetworkSettingsLogic.updatePage()
            }
            if (page === PageEnum.Sites) {
                SitesLogic.updateSitesPage()
            }
            if (page === PageEnum.Vpn) {
                VpnLogic.updateVpnPage()
            }
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
            UiLogic.updatePage();
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
//        width: GC.screenWidth
//        height: GC.screenHeight
        anchors.fill: parent
        color: "white"
    }

    StackView {
        id: pageLoader
        y: GC.isDesktop() ? titleBar.height : 0
//        width: GC.screenWidth
//        height: GC.screenHeight
        anchors.fill: parent

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
        PageNewServerProtocols {}
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
        PageServerList {}
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
        PageServerSettings {}
    }
    Component {
        id: page_server_containers
        PageServerContainers {}
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
        PageProtoShadowSocks {}
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

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0

import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

DrawerType2 {
    id: root

    anchors.fill: parent
    expandedHeight: parent.height * 0.7

    expandedContent: ColumnLayout {
        id: content

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 0

        Connections {
            target: root
            enabled: !GC.isMobile()
            function onOpened() {
                if (splitTunneling.visible) {
                    splitTunneling.rightButton.forceActiveFocus()
                } else {
                    splitTunnelingSiteBased.rightButton.forceActiveFocus()
                }
            }
        }

        Header2Type {
            Layout.fillWidth: true
            Layout.topMargin: 24
            Layout.rightMargin: 16
            Layout.leftMargin: 16
            Layout.bottomMargin: 16

            headerText: qsTr("Split tunneling")
            descriptionText:  qsTr("Allows you to connect to some sites or applications through a VPN connection and bypass others")
        }

        LabelWithButtonType {
            id: splitTunneling
            Layout.fillWidth: true
            Layout.topMargin: 16

            visible: ServersModel.isDefaultServerDefaultContainerHasSplitTunneling && ServersModel.getDefaultServerData("isServerFromApi")

            text: qsTr("Split tunneling on the server")
            descriptionText: qsTr("Enabled \nCan't be disabled for current server")
            rightImageSource: "qrc:/images/controls/chevron-right.svg"

            clickedFunction: function() {
//                PageController.goToPage(PageEnum.PageSettingsSplitTunneling)
//                root.close()
            }

            KeyNavigation.tab: splitTunnelingSiteBased.rightButton
        }

        DividerType {
            visible: ServersModel.isDefaultServerDefaultContainerHasSplitTunneling && ServersModel.getDefaultServerData("isServerFromApi")
        }

        LabelWithButtonType {
            id: splitTunnelingSiteBased
            Layout.fillWidth: true
            Layout.topMargin: 16

            enabled: ! ServersModel.isDefaultServerDefaultContainerHasSplitTunneling || !ServersModel.getDefaultServerData("isServerFromApi")

            text: qsTr("Site-based split tunneling")
            descriptionText: enabled && SitesModel.isTunnelingEnabled ? qsTr("Enabled") : qsTr("Disabled")
            rightImageSource: "qrc:/images/controls/chevron-right.svg"

            clickedFunction: function() {
                PageController.goToPage(PageEnum.PageSettingsSplitTunneling)
                root.close()
            }
            KeyNavigation.tab: appBasedSplitTunneling.visible ? appBasedSplitTunneling.rightButton : null
        }

        DividerType {
        }

        LabelWithButtonType {
            id: appBasedSplitTunneling
            Layout.fillWidth: true
            visible: false

            text: qsTr("App-based split tunneling")
            rightImageSource: "qrc:/images/controls/chevron-right.svg"

            clickedFunction: function() {
//                PageController.goToPage(PageEnum.PageSetupWizardConfigSource)
                root.close()
            }

            KeyNavigation.tab: splitTunneling.rightButton
        }

        DividerType {
            visible: false
        }
    }
}

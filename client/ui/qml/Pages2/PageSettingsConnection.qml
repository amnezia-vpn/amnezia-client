import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0

import "./"
import "../Controls2"
import "../Config"

PageType {
    id: root

    defaultActiveFocusItem: focusItem

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20
    }

    FlickableType {
        id: fl
        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        contentHeight: content.height

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            HeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Connection")
            }

            Item {
                id: focusItem
                KeyNavigation.tab: amneziaDnsSwitch
            }

            SwitcherType {
                id: amneziaDnsSwitch
                Layout.fillWidth: true
                Layout.margins: 16

                text: qsTr("Use AmneziaDNS")
                descriptionText: qsTr("If AmneziaDNS is installed on the server")

                checked: SettingsController.isAmneziaDnsEnabled()
                onCheckedChanged: {
                    if (checked !== SettingsController.isAmneziaDnsEnabled()) {
                        SettingsController.toggleAmneziaDns(checked)
                    }
                }

                KeyNavigation.tab: dnsServersButton.rightButton
            }

            DividerType {}

            LabelWithButtonType {
                id: dnsServersButton
                Layout.fillWidth: true

                text: qsTr("DNS servers")
                descriptionText: qsTr("When AmneziaDNS is not used or installed")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"

                clickedFunction: function() {
                    PageController.goToPage(PageEnum.PageSettingsDns)
                }

                KeyNavigation.tab: splitTunnelingButton.rightButton
            }

            DividerType {}

            LabelWithButtonType {
                id: splitTunnelingButton
                visible: true

                Layout.fillWidth: true

                text: qsTr("Site-based split tunneling")
                descriptionText: qsTr("Allows you to select which sites you want to access through the VPN")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"

                clickedFunction: function() {
                    PageController.goToPage(PageEnum.PageSettingsSplitTunneling)
                }

            Keys.onTabPressed: splitTunnelingButton2.visible ?
                                   splitTunnelingButton2.forceActiveFocus() :
                                   lastItemTabClicked()
            }

            DividerType {
                visible: GC.isDesktop()
            }

            LabelWithButtonType {
                id: splitTunnelingButton2
                visible: false

                Layout.fillWidth: true

                text: qsTr("App-based split tunneling")
                descriptionText: qsTr("Allows you to use the VPN only for certain Apps")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"

                clickedFunction: function() {
                }

                Keys.onTabPressed: lastItemTabClicked()
            }

            DividerType {
                visible: false
            }
        }
    }
}

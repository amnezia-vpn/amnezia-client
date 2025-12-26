import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Config"

PageType {
    id: root

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20 + SettingsController.safeAreaTopMargin

        onActiveFocusChanged: {
            if(backButton.enabled && backButton.activeFocus) {
                listView.positionViewAtBeginning()
            }
        }
    }

    ListViewType {
        id: listView

        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        header: ColumnLayout {
            width: listView.width

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Connection")
            }
        }

        model: 1

        delegate: ColumnLayout {
            width: listView.width

            LabelWithButtonType {
                id: vpnProtocolButton

                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("VPN protocol")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"

                clickedFunction: function() {
                    PageController.goToPage(PageEnum.PageSettingsConnectionProtocols)
                }
            }

            DividerType {}

            LabelWithButtonType {
                id: localProxyButton

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Local proxy")
                descriptionText: qsTr("Running: 127.0.0.1:1080")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"

                clickedFunction: function() {
                    // Placeholder - no action
                }
            }

            DividerType {}
        }
    }
}

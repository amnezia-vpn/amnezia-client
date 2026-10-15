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

    FlickableType {
        id: flickable

        anchors.top: parent.top

        contentHeight: content.implicitHeight

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0

            NavigationBarType {
                Layout.fillWidth: true

                title: qsTr("Connection")
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 16 + PageController.safeAreaBottomMargin

                spacing: 0

                ListItemType {
                    id: localProxyButton

                    Layout.fillWidth: true

                    visible: SettingsController.isLocalProxySupported && ServersUiController.processedServerIsPremium

                    showSeparator: false
                    text: qsTr("Local proxy")
                    subtitle: {
                        if (!SettingsController.isLocalProxyHttpEnabled) {
                            return qsTr("Off")
                        }
                        if (SettingsController.localProxyOwnerId !== ServersUiController.processedServerId) {
                            return qsTr("In use by another server")
                        }
                        return qsTr("Running: 127.0.0.1:%1").arg(SettingsController.localProxyActivePort > 0 ? SettingsController.localProxyActivePort : SettingsController.localProxyPort)
                    }
                    trailingIconSource: "qrc:/images/controls/chevron-right.svg"

                    onClicked: {
                        PageController.goToPage(PageEnum.PageSettingsLocalProxy)
                    }
                }
            }
        }
    }
}

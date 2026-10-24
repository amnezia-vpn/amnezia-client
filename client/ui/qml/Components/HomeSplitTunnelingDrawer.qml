import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0

import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

SheetType {
    id: root

    property bool isAppSplitTinnelingEnabled: Qt.platform.os === "windows" || Qt.platform.os === "android"

    expandedHeight: Math.min(content.implicitHeight + 24 + PageController.safeAreaBottomMargin, windowHeight * 0.9)

    ColumnLayout {
        id: content

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 0

        SectionHeaderType {
            Layout.fillWidth: true
            Layout.topMargin: 16
            Layout.rightMargin: 16
            Layout.leftMargin: 16
            Layout.bottomMargin: 32

            headerText: qsTr("Split tunneling")
            descriptionText:  qsTr("Allows you to connect to some sites or applications through a VPN connection and bypass others")
        }

        ListItemType {
            id: splitTunnelingSwitch
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            visible: ServersUiController.isDefaultServerDefaultContainerHasSplitTunneling

            text: qsTr("Split tunneling on the server")
            subtitle: qsTr("Enabled \nCan't be disabled for current server")
            trailingIconSource: "qrc:/images/controls/chevron-right.svg"

            onClicked: {
                root.close()
                PageController.goToPage(PageEnum.PageSettingsSplitTunneling)
            }
        }

        ListItemType {
            id: siteBasedSplitTunnelingSwitch
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            text: qsTr("Site-based split tunneling")
            subtitle: enabled && IpSplitTunnelingController.isSplitTunnelingEnabled ? qsTr("Enabled") : qsTr("Disabled")
            trailingIconSource: "qrc:/images/controls/chevron-right.svg"

            onClicked: {
                root.close()
                PageController.goToPage(PageEnum.PageSettingsSplitTunneling)
            }
        }

        ListItemType {
            id: appSplitTunnelingSwitch
            visible: isAppSplitTinnelingEnabled

            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            text: qsTr("App-based split tunneling")
            subtitle: AppSplitTunnelingController.isSplitTunnelingEnabled ? qsTr("Enabled") : qsTr("Disabled")
            trailingIconSource: "qrc:/images/controls/chevron-right.svg"

            onClicked: {
                root.close()
                PageController.goToPage(PageEnum.PageSettingsAppSplitTunneling)
            }
        }
    }
}

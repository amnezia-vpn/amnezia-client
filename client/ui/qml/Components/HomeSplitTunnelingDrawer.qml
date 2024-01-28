import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0

import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

DrawerType {
    id: root

    width: parent.width
    height: content.implicitHeight + 32

    ColumnLayout {
        id: content

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 0

        Header2Type {
            Layout.fillWidth: true
            Layout.topMargin: 24
            Layout.rightMargin: 16
            Layout.leftMargin: 16
            Layout.bottomMargin: 16

            headerText: qsTr("Split tunneling")
            descriptionText:  qsTr("Allows you to connect to some sites or applications through a secure connection and bypass others")
        }

        LabelWithButtonType {
            Layout.fillWidth: true
            Layout.topMargin: 16

            visible: ContainersModel.isDefaultContainerHasGlobalSiteSplitTunneling && ServersModel.isDefaultServerFromApi()

            text: qsTr("Tunneling on the server")
            descriptionText: qsTr("Enabled. You are connected to a server that already uses split tunneling, it is prioritized")
            rightImageSource: "qrc:/images/controls/chevron-right.svg"

            clickedFunction: function() {
//                PageController.goToPage(PageEnum.PageSettingsSplitTunneling)
//                root.visible = false
            }
        }

        DividerType {
            visible: ContainersModel.isDefaultContainerHasGlobalSiteSplitTunneling && ServersModel.isDefaultServerFromApi()
        }

        LabelWithButtonType {
            Layout.fillWidth: true
            Layout.topMargin: 16

            text: qsTr("Site-based split tunneling")
            descriptionText: SitesModel.isTunnelingEnabled ? qsTr("Enabled") : qsTr("Disabled")
            rightImageSource: "qrc:/images/controls/chevron-right.svg"

            clickedFunction: function() {
                PageController.goToPage(PageEnum.PageSettingsSplitTunneling)
                root.visible = false
            }
        }

        DividerType {}

        LabelWithButtonType {
            Layout.fillWidth: true
            visible: false

            text: qsTr("App-based split tunneling")
            rightImageSource: "qrc:/images/controls/chevron-right.svg"

            clickedFunction: function() {
//                PageController.goToPage(PageEnum.PageSetupWizardConfigSource)
                root.visible = false
            }
        }

        DividerType {
            visible: false
        }
    }
}

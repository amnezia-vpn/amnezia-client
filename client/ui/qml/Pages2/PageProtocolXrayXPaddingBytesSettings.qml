import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import ProtocolEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    property bool editDirty: false

    ListViewType {
        id: listView

        anchors.top: parent.top
        anchors.bottom: saveButton.visible ? saveButton.top : parent.bottom
        anchors.bottomMargin: saveButton.visible ? 16 : 0
        anchors.left: parent.left
        anchors.right: parent.right

        enabled: ServersUiController.isProcessedServerHasWriteAccess()
        model: XrayConfigModel

        header: NavigationBarType {
            width: listView.width

            title: qsTr("xPaddingBytes")
        }

        footer: Item {
            width: listView.width
            height: saveButton.visible ? 0 : 16 + PageController.safeAreaBottomMargin
        }

        delegate: Item {
            width: listView.width
            implicitHeight: content.implicitHeight

            ColumnLayout {
                id: content

                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 16
                anchors.rightMargin: 16

                spacing: 12

                SmallTextType {
                    Layout.fillWidth: true
                    Layout.topMargin: 16
                    text: qsTr("Range")
                }

                MinMaxRowType {
                    Layout.fillWidth: true
                    minValue: xPaddingBytesMin
                    maxValue: xPaddingBytesMax
                    minPlaceholder: XrayConfigModel.xPaddingBytesMinDefault()
                    maxPlaceholder: XrayConfigModel.xPaddingBytesMaxDefault()
                    onMinChanged: function(val) { xPaddingBytesMin = val; root.editDirty = false }
                    onMaxChanged: function(val) { xPaddingBytesMax = val; root.editDirty = false }
                    onEdited: root.editDirty = true
                }
            }
        }
    }

    ButtonType {
        id: saveButton

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.bottomMargin: 8 + PageController.safeAreaBottomMargin

        visible: listView.enabled && (XrayConfigModel.hasUnsavedChanges || root.editDirty)
        enabled: visible
        text: qsTr("Save")
        onClicked: {
            saveButton.forceActiveFocus()
            var headerText = qsTr("Save settings?")
            var descriptionText = XrayConfigModel.saveDescription("")
            var yesButtonText = qsTr("Continue")
            var noButtonText = qsTr("Cancel")
            var yesButtonFunction = function () {
                if (ConnectionController.isConnected && ServersUiController.serverDefaultContainer(ServersUiController.defaultServerId) === ServersUiController.processedContainerIndex) {
                    PageController.showNotificationMessage(qsTr("Unable change settings while there is an active connection"))
                    return
                }
                PageController.goToPage(PageEnum.PageSetupWizardInstalling)
                InstallController.updateServerConfig(ServersUiController.processedServerId, ServersUiController.processedContainerIndex, ProtocolEnum.Xray)
            }
            var noButtonFunction = function () {
                if (typeof GC !== "undefined" && !GC.isMobile()) {
                    saveButton.forceActiveFocus()
                }
            }
            showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
        }
    }
}

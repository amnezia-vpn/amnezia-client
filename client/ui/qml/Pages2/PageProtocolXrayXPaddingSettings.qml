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

    BackButtonType {
        id: backButton
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20 + PageController.safeAreaTopMargin
    }

    ListViewType {
        id: listView
        anchors.top: backButton.bottom
        anchors.bottom: saveButton.visible ? saveButton.top : parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        enabled: ServersUiController.isProcessedServerHasWriteAccess()
        model: XrayConfigModel

        delegate: ColumnLayout {
            width: listView.width
            spacing: 0

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 0
                Layout.bottomMargin: 24
                headerText: qsTr("xPadding")
            }

            // xPaddingBytes — min/max display row
            LabelWithButtonType {
                Layout.fillWidth: true
                text: qsTr("xPaddingBytes")
                descriptionText: (xPaddingBytesMin !== "" ? xPaddingBytesMin : "0") + "—" + (xPaddingBytesMax !== "" ? xPaddingBytesMax : "0")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"
                clickedFunction: function () {
                    PageController.goToPage(PageEnum.PageProtocolXrayXPaddingBytesSettings)
                }
            }

            DividerType {
            }

            SwitcherType {
                Layout.fillWidth: true
                Layout.margins: 16
                text: qsTr("xPaddingObfsMode")
                checked: xPaddingObfsMode
                onToggled: xPaddingObfsMode = checked
            }

            DividerType {
            }

            TextFieldWithHeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 16
                headerText: qsTr("xPaddingKey")
                textField.text: xPaddingKey
                textField.onEditingFinished: {
                    if (textField.text !== xPaddingKey) xPaddingKey = textField.text
                }
            }

            TextFieldWithHeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 8
                headerText: qsTr("xPaddingHeader")
                textField.text: xPaddingHeader
                textField.onEditingFinished: {
                    if (textField.text !== xPaddingHeader) xPaddingHeader = textField.text
                }
            }

            DropDownType {
                id: placementDropDown
                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: xPaddingPlacement
                descriptionText: qsTr("xPaddingPlacement")
                headerText: qsTr("xPaddingPlacement")
                drawerParent: root
                listView: ListViewWithRadioButtonType {
                    rootWidth: root.width
                    model: ListModel {
                        Component.onCompleted: {
                            var opts = XrayConfigModel.xPaddingPlacementOptions()
                            for (var i = 0; i < opts.length; i++) {
                                append({name: opts[i]})
                            }
                        }
                    }
                    clickedFunction: function () {
                        xPaddingPlacement = selectedText
                        placementDropDown.text = selectedText
                        placementDropDown.closeTriggered()
                    }
                    Component.onCompleted: {
                        for (var i = 0; i < model.count; i++) {
                            if (model.get(i).name === xPaddingPlacement) {
                                selectedIndex = i;
                                break
                            }
                        }
                    }
                }
                Connections {
                    target: XrayConfigModel

                    function onDataChanged() {
                        placementDropDown.text = xPaddingPlacement
                    }
                }
            }

            DropDownType {
                id: methodDropDown
                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: xPaddingMethod
                descriptionText: qsTr("xPaddingMethod")
                headerText: qsTr("xPaddingMethod")
                drawerParent: root
                listView: ListViewWithRadioButtonType {
                    rootWidth: root.width
                    model: ListModel {
                        Component.onCompleted: {
                            var opts = XrayConfigModel.xPaddingMethodOptions()
                            for (var i = 0; i < opts.length; i++) {
                                append({name: opts[i]})
                            }
                        }
                    }
                    clickedFunction: function () {
                        xPaddingMethod = selectedText
                        methodDropDown.text = selectedText
                        methodDropDown.closeTriggered()
                    }
                    Component.onCompleted: {
                        for (var i = 0; i < model.count; i++) {
                            if (model.get(i).name === xPaddingMethod) {
                                selectedIndex = i;
                                break
                            }
                        }
                    }
                }
                Connections {
                    target: XrayConfigModel

                    function onDataChanged() {
                        methodDropDown.text = xPaddingMethod
                    }
                }
            }

            Item {
                Layout.preferredHeight: 16
            }
        }
    }

    BasicButtonType {
        id: saveButton

        anchors.left: root.left
        anchors.right: root.right
        anchors.bottom: root.bottom
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.bottomMargin: 16 + PageController.safeAreaBottomMargin

        visible: listView.enabled && XrayConfigModel.hasUnsavedChanges
        enabled: visible
        text: qsTr("Save")
        clickedFunc: function () {
            var headerText = qsTr("Save settings?")
            var descriptionText = qsTr("All users with whom you shared a connection with will no longer be able to connect to it.")
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

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

    FlickableType {
        id: flickable

        anchors.top: parent.top
        anchors.bottom: saveButton.visible ? saveButton.top : parent.bottom
        anchors.bottomMargin: saveButton.visible ? 24 : 0

        enabled: ServersUiController.isProcessedServerHasWriteAccess()

        contentHeight: content.implicitHeight

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0

            NavigationBarType {
                Layout.fillWidth: true

                title: qsTr("xPadding")
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                spacing: 12

                ListItemType {
                    Layout.fillWidth: true
                    enabled: XrayConfigModel.xPaddingObfsMode
                    text: qsTr("xPaddingBytes")
                    subtitle: (XrayConfigModel.xPaddingBytesMin === "" && XrayConfigModel.xPaddingBytesMax === "") ? qsTr("Default") : (XrayConfigModel.xPaddingBytesMin !== "" ? XrayConfigModel.xPaddingBytesMin : XrayConfigModel.xPaddingBytesMinDefault()) + "—" + (XrayConfigModel.xPaddingBytesMax !== "" ? XrayConfigModel.xPaddingBytesMax : XrayConfigModel.xPaddingBytesMaxDefault())
                    trailingIconSource: "qrc:/images/controls/chevron-right.svg"
                    onClicked: {
                        PageController.goToPage(PageEnum.PageProtocolXrayXPaddingBytesSettings)
                    }
                }

                SwitchRowType {
                    Layout.fillWidth: true
                    Layout.topMargin: 12
                    text: qsTr("xPaddingObfsMode")
                    checked: XrayConfigModel.xPaddingObfsMode
                    onToggled: XrayConfigModel.xPaddingObfsMode = checked
                }

                TextFieldType {
                    Layout.fillWidth: true
                    enabled: XrayConfigModel.xPaddingObfsMode
                    Layout.topMargin: 12
                    headerText: qsTr("xPaddingKey")
                    placeholderText: XrayConfigModel.xPaddingKeyDefault()
                    textField.text: XrayConfigModel.xPaddingKey
                    textField.validator: RegularExpressionValidator { regularExpression: /^[A-Za-z0-9_-]*$/ }
                    textField.onTextEdited: XrayConfigModel.xPaddingKey = textField.text
                    textField.onEditingFinished: {
                        var v = textField.text.trim()
                        if (v !== XrayConfigModel.xPaddingKey) XrayConfigModel.xPaddingKey = v
                    }
                }

                TextFieldType {
                    Layout.fillWidth: true
                    enabled: XrayConfigModel.xPaddingObfsMode
                    headerText: qsTr("xPaddingHeader")
                    placeholderText: XrayConfigModel.xPaddingHeaderDefault()
                    textField.text: XrayConfigModel.xPaddingHeader
                    textField.validator: RegularExpressionValidator { regularExpression: /^[A-Za-z0-9_-]*$/ }
                    textField.onTextEdited: XrayConfigModel.xPaddingHeader = textField.text
                    textField.onEditingFinished: {
                        var v = textField.text.trim()
                        if (v !== XrayConfigModel.xPaddingHeader) XrayConfigModel.xPaddingHeader = v
                    }
                }

                DropDownType {
                    id: placementDropDown
                    enabled: XrayConfigModel.xPaddingObfsMode
                    fitContent: true
                    Layout.fillWidth: true
                    text: XrayConfigModel.xPaddingPlacement
                    descriptionText: qsTr("xPaddingPlacement")
                    headerText: qsTr("xPaddingPlacement")
                    listView: RadioListType {
                        currentValue: XrayConfigModel.xPaddingPlacement
                        model: ListModel {
                            Component.onCompleted: {
                                var opts = XrayConfigModel.xPaddingPlacementOptions()
                                for (var i = 0; i < opts.length; i++) {
                                    append({name: opts[i]})
                                }
                            }
                        }
                        onActivated: {
                            XrayConfigModel.xPaddingPlacement = selectedText
                            placementDropDown.close()
                        }
                        Component.onCompleted: {
                            for (var i = 0; i < model.count; i++) {
                                if (model.get(i).name === XrayConfigModel.xPaddingPlacement) {
                                    selectedIndex = i;
                                    break
                                }
                            }
                        }
                    }
                }

                DropDownType {
                    id: methodDropDown
                    enabled: XrayConfigModel.xPaddingObfsMode
                    fitContent: true
                    Layout.fillWidth: true
                    text: XrayConfigModel.xPaddingMethod
                    descriptionText: qsTr("xPaddingMethod")
                    headerText: qsTr("xPaddingMethod")
                    listView: RadioListType {
                        currentValue: XrayConfigModel.xPaddingMethod
                        model: ListModel {
                            Component.onCompleted: {
                                var opts = XrayConfigModel.xPaddingMethodOptions()
                                for (var i = 0; i < opts.length; i++) {
                                    append({name: opts[i]})
                                }
                            }
                        }
                        onActivated: {
                            XrayConfigModel.xPaddingMethod = selectedText
                            methodDropDown.close()
                        }
                        Component.onCompleted: {
                            for (var i = 0; i < model.count; i++) {
                                if (model.get(i).name === XrayConfigModel.xPaddingMethod) {
                                    selectedIndex = i;
                                    break
                                }
                            }
                        }
                    }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: saveButton.visible ? 0 : 16 + PageController.safeAreaBottomMargin
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

        visible: flickable.enabled
        enabled: XrayConfigModel.hasUnsavedChanges
        text: qsTr("Save")
        onClicked: {
            saveButton.forceActiveFocus()
            var errs = XrayConfigModel.validationErrors()
            if (errs.length > 0) {
                PageController.showErrorMessage(errs.join("\n"))
                return
            }
            var headerText = qsTr("Save settings?")
            var descriptionText = XrayConfigModel.saveDescription("")
            var yesButtonText = qsTr("Continue")
            var noButtonText = qsTr("Cancel")
            var yesButtonFunction = function () {
                if (ConnectionController.isConnected && ServersUiController.serverDefaultContainer(ServersUiController.defaultServerId) === ServersUiController.processedContainerIndex) {
                    PageController.showNotificationMessage(qsTr("Unable change settings while there is an active connection"))
                    return
                }
                var touchesServer = XrayConfigModel.pendingChangeTouchesServer("")
                if (touchesServer)
                    PageController.goToPage(PageEnum.PageSetupWizardInstalling)
                InstallController.updateServerConfig(ServersUiController.processedServerId, ServersUiController.processedContainerIndex, ProtocolEnum.Xray, touchesServer)
            }
            var noButtonFunction = function () {}
            Dialogs.askQuestion(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
        }
    }
}

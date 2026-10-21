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

            title: qsTr("xPadding")
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

                ListItemType {
                    Layout.fillWidth: true
                    Layout.topMargin: 16
                    text: qsTr("xPaddingBytes")
                    subtitle: (xPaddingBytesMin !== "" ? xPaddingBytesMin : "0") + "—" + (xPaddingBytesMax !== "" ? xPaddingBytesMax : "0")
                    trailingIconSource: "qrc:/images/controls/chevron-right.svg"
                    onClicked: {
                        PageController.goToPage(PageEnum.PageProtocolXrayXPaddingBytesSettings)
                    }
                }

                SwitchRowType {
                    Layout.fillWidth: true
                    Layout.topMargin: 12
                    text: qsTr("xPaddingObfsMode")
                    checked: xPaddingObfsMode
                    onToggled: xPaddingObfsMode = checked
                }

                TextFieldType {
                    Layout.fillWidth: true
                    Layout.topMargin: 12
                    headerText: qsTr("xPaddingKey")
                    placeholderText: XrayConfigModel.xPaddingKeyDefault()
                    textField.text: xPaddingKey
                    textField.validator: RegularExpressionValidator { regularExpression: /^[A-Za-z0-9_-]*$/ }
                    textField.onTextEdited: root.editDirty = (textField.text !== xPaddingKey)
                    textField.onEditingFinished: {
                        var v = textField.text.trim()
                        if (v !== xPaddingKey) xPaddingKey = v
                        else if (textField.text !== v) textField.text = v
                        root.editDirty = false
                    }
                }

                TextFieldType {
                    Layout.fillWidth: true
                    headerText: qsTr("xPaddingHeader")
                    placeholderText: XrayConfigModel.xPaddingHeaderDefault()
                    textField.text: xPaddingHeader
                    textField.validator: RegularExpressionValidator { regularExpression: /^[A-Za-z0-9_-]*$/ }
                    textField.onTextEdited: root.editDirty = (textField.text !== xPaddingHeader)
                    textField.onEditingFinished: {
                        var v = textField.text.trim()
                        if (v !== xPaddingHeader) xPaddingHeader = v
                        else if (textField.text !== v) textField.text = v
                        root.editDirty = false
                    }
                }

                DropDownType {
                    id: placementDropDown
                    fitContent: true
                    Layout.fillWidth: true
                    text: xPaddingPlacement
                    descriptionText: qsTr("xPaddingPlacement")
                    headerText: qsTr("xPaddingPlacement")
                    listView: RadioListType {
                        currentValue: xPaddingPlacement
                        model: ListModel {
                            Component.onCompleted: {
                                var opts = XrayConfigModel.xPaddingPlacementOptions()
                                for (var i = 0; i < opts.length; i++) {
                                    append({name: opts[i]})
                                }
                            }
                        }
                        onActivated: {
                            xPaddingPlacement = selectedText
                            placementDropDown.text = selectedText
                            placementDropDown.close()
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
                    fitContent: true
                    Layout.fillWidth: true
                    text: xPaddingMethod
                    descriptionText: qsTr("xPaddingMethod")
                    headerText: qsTr("xPaddingMethod")
                    listView: RadioListType {
                        currentValue: xPaddingMethod
                        model: ListModel {
                            Component.onCompleted: {
                                var opts = XrayConfigModel.xPaddingMethodOptions()
                                for (var i = 0; i < opts.length; i++) {
                                    append({name: opts[i]})
                                }
                            }
                        }
                        onActivated: {
                            xPaddingMethod = selectedText
                            methodDropDown.text = selectedText
                            methodDropDown.close()
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

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

    function clampSigned(text) {
        if (text === "" || text === "-")
            return ""
        var n = parseInt(text, 10)
        if (isNaN(n))
            return ""
        if (n > 2147483647)
            n = 2147483647
        if (n < -2147483648)
            n = -2147483648
        return String(n)
    }

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

            title: qsTr("xmux")
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

                SwitchRowType {
                    Layout.fillWidth: true
                    Layout.topMargin: 16
                    text: qsTr("xmux")
                    checked: xmuxEnabled
                    onToggled: xmuxEnabled = checked
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 12

                    enabled: xmuxEnabled
                    spacing: 12

                    SmallTextType {
                        Layout.fillWidth: true
                        text: qsTr("maxConcurrency")
                    }
                    MinMaxRowType {
                        Layout.fillWidth: true
                        minValue: xmuxMaxConcurrencyMin
                        maxValue: xmuxMaxConcurrencyMax
                        minPlaceholder: "0"
                        maxPlaceholder: "0"
                        onMinChanged: function(val) { xmuxMaxConcurrencyMin = val; root.editDirty = false }
                        onMaxChanged: function(val) { xmuxMaxConcurrencyMax = val; root.editDirty = false }
                        onEdited: root.editDirty = true
                    }

                    SmallTextType {
                        Layout.fillWidth: true
                        Layout.topMargin: 12
                        text: qsTr("maxConnections")
                    }
                    MinMaxRowType {
                        Layout.fillWidth: true
                        minValue: xmuxMaxConnectionsMin
                        maxValue: xmuxMaxConnectionsMax
                        minPlaceholder: "0"
                        maxPlaceholder: "0"
                        onMinChanged: function(val) { xmuxMaxConnectionsMin = val; root.editDirty = false }
                        onMaxChanged: function(val) { xmuxMaxConnectionsMax = val; root.editDirty = false }
                        onEdited: root.editDirty = true
                    }

                    SmallTextType {
                        Layout.fillWidth: true
                        Layout.topMargin: 12
                        text: qsTr("cMaxReuseTimes")
                    }
                    MinMaxRowType {
                        Layout.fillWidth: true
                        minValue: xmuxCMaxReuseTimesMin
                        maxValue: xmuxCMaxReuseTimesMax
                        minPlaceholder: "0"
                        maxPlaceholder: "0"
                        onMinChanged: function(val) { xmuxCMaxReuseTimesMin = val; root.editDirty = false }
                        onMaxChanged: function(val) { xmuxCMaxReuseTimesMax = val; root.editDirty = false }
                        onEdited: root.editDirty = true
                    }

                    SmallTextType {
                        Layout.fillWidth: true
                        Layout.topMargin: 12
                        text: qsTr("hMaxRequestTimes")
                    }
                    MinMaxRowType {
                        Layout.fillWidth: true
                        minValue: xmuxHMaxRequestTimesMin
                        maxValue: xmuxHMaxRequestTimesMax
                        minPlaceholder: "0"
                        maxPlaceholder: "0"
                        onMinChanged: function(val) { xmuxHMaxRequestTimesMin = val; root.editDirty = false }
                        onMaxChanged: function(val) { xmuxHMaxRequestTimesMax = val; root.editDirty = false }
                        onEdited: root.editDirty = true
                    }

                    SmallTextType {
                        Layout.fillWidth: true
                        Layout.topMargin: 12
                        text: qsTr("hMaxReusableSecs")
                    }
                    MinMaxRowType {
                        Layout.fillWidth: true
                        minValue: xmuxHMaxReusableSecsMin
                        maxValue: xmuxHMaxReusableSecsMax
                        minPlaceholder: "0"
                        maxPlaceholder: "0"
                        onMinChanged: function(val) { xmuxHMaxReusableSecsMin = val; root.editDirty = false }
                        onMaxChanged: function(val) { xmuxHMaxReusableSecsMax = val; root.editDirty = false }
                        onEdited: root.editDirty = true
                    }

                    TextFieldType {
                        Layout.fillWidth: true
                        Layout.topMargin: 12
                        headerText: qsTr("hKeepAlivePeriod")
                        helperText: qsTr("HTTP keep-alive period. Integer, may be negative.")
                        textField.text: xmuxHKeepAlivePeriod
                        textField.maximumLength: 11
                        textField.validator: RegularExpressionValidator { regularExpression: /^-?\d*$/ }
                        textField.onTextEdited: root.editDirty = (textField.text !== xmuxHKeepAlivePeriod)
                        textField.onEditingFinished: {
                            var v = root.clampSigned(textField.text)
                            if (v !== xmuxHKeepAlivePeriod) xmuxHKeepAlivePeriod = v
                            else if (textField.text !== v) textField.text = v
                            root.editDirty = false
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
            var noButtonFunction = function () {}
            showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
        }
    }
}


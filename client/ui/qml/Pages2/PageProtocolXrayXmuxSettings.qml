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

                title: qsTr("xmux")
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                spacing: 12

                SwitchRowType {
                    Layout.fillWidth: true
                    text: qsTr("xmux")
                    descriptionText: qsTr("Set either maxConcurrency or maxConnections, not both")
                    checked: XrayConfigModel.xmuxEnabled
                    onToggled: XrayConfigModel.xmuxEnabled = checked
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 12

                    enabled: XrayConfigModel.xmuxEnabled
                    spacing: 12

                    SmallTextType {
                        Layout.fillWidth: true
                        text: qsTr("maxConcurrency")
                    }
                    MinMaxRowType {
                        Layout.fillWidth: true
                        minValue: XrayConfigModel.xmuxMaxConcurrencyMin
                        maxValue: XrayConfigModel.xmuxMaxConcurrencyMax
                        minPlaceholder: "0"
                        maxPlaceholder: "0"
                        onMinChanged: function(val) { XrayConfigModel.xmuxMaxConcurrencyMin = val }
                        onMaxChanged: function(val) { XrayConfigModel.xmuxMaxConcurrencyMax = val }
                    }

                    SmallTextType {
                        Layout.fillWidth: true
                        Layout.topMargin: 12
                        text: qsTr("maxConnections")
                    }
                    MinMaxRowType {
                        Layout.fillWidth: true
                        minValue: XrayConfigModel.xmuxMaxConnectionsMin
                        maxValue: XrayConfigModel.xmuxMaxConnectionsMax
                        minPlaceholder: "0"
                        maxPlaceholder: "0"
                        onMinChanged: function(val) { XrayConfigModel.xmuxMaxConnectionsMin = val }
                        onMaxChanged: function(val) { XrayConfigModel.xmuxMaxConnectionsMax = val }
                    }

                    SmallTextType {
                        Layout.fillWidth: true
                        Layout.topMargin: 12
                        text: qsTr("cMaxReuseTimes")
                    }
                    MinMaxRowType {
                        Layout.fillWidth: true
                        minValue: XrayConfigModel.xmuxCMaxReuseTimesMin
                        maxValue: XrayConfigModel.xmuxCMaxReuseTimesMax
                        minPlaceholder: "0"
                        maxPlaceholder: "0"
                        onMinChanged: function(val) { XrayConfigModel.xmuxCMaxReuseTimesMin = val }
                        onMaxChanged: function(val) { XrayConfigModel.xmuxCMaxReuseTimesMax = val }
                    }

                    SmallTextType {
                        Layout.fillWidth: true
                        Layout.topMargin: 12
                        text: qsTr("hMaxRequestTimes")
                    }
                    MinMaxRowType {
                        Layout.fillWidth: true
                        minValue: XrayConfigModel.xmuxHMaxRequestTimesMin
                        maxValue: XrayConfigModel.xmuxHMaxRequestTimesMax
                        minPlaceholder: "0"
                        maxPlaceholder: "0"
                        onMinChanged: function(val) { XrayConfigModel.xmuxHMaxRequestTimesMin = val }
                        onMaxChanged: function(val) { XrayConfigModel.xmuxHMaxRequestTimesMax = val }
                    }

                    SmallTextType {
                        Layout.fillWidth: true
                        Layout.topMargin: 12
                        text: qsTr("hMaxReusableSecs")
                    }
                    MinMaxRowType {
                        Layout.fillWidth: true
                        minValue: XrayConfigModel.xmuxHMaxReusableSecsMin
                        maxValue: XrayConfigModel.xmuxHMaxReusableSecsMax
                        minPlaceholder: "0"
                        maxPlaceholder: "0"
                        onMinChanged: function(val) { XrayConfigModel.xmuxHMaxReusableSecsMin = val }
                        onMaxChanged: function(val) { XrayConfigModel.xmuxHMaxReusableSecsMax = val }
                    }

                    TextFieldType {
                        Layout.fillWidth: true
                        Layout.topMargin: 12
                        headerText: qsTr("hKeepAlivePeriod")
                        helperText: qsTr("HTTP keep-alive period. Integer, may be negative.")
                        textField.text: XrayConfigModel.xmuxHKeepAlivePeriod
                        textField.maximumLength: 11
                        textField.validator: RegularExpressionValidator { regularExpression: /^-?\d*$/ }
                        textField.onTextEdited: XrayConfigModel.xmuxHKeepAlivePeriod = textField.text
                        textField.onEditingFinished: {
                            var v = root.clampSigned(textField.text)
                            if (v !== XrayConfigModel.xmuxHKeepAlivePeriod) XrayConfigModel.xmuxHKeepAlivePeriod = v
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


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
                headerText: qsTr("xmux")
            }

            SwitcherType {
                Layout.fillWidth: true
                Layout.margins: 16
                text: qsTr("xmux")
                checked: xmuxEnabled
                onToggled: xmuxEnabled = checked
            }

            DividerType {
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0
                enabled: xmuxEnabled

                // maxConcurrency
                CaptionTextType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 24
                    Layout.bottomMargin: 8
                    text: qsTr("maxConcurrency")
                    color: AmneziaStyle.color.mutedGray
                }
                MinMaxRowType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    minValue: xmuxMaxConcurrencyMin
                    maxValue: xmuxMaxConcurrencyMax
                    onMinChanged: function(val) { xmuxMaxConcurrencyMin = val; root.editDirty = false }
                    onMaxChanged: function(val) { xmuxMaxConcurrencyMax = val; root.editDirty = false }
                    onEdited: root.editDirty = true
                }

                // maxConnections
                CaptionTextType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 16
                    Layout.bottomMargin: 8
                    text: qsTr("maxConnections")
                    color: AmneziaStyle.color.mutedGray
                }
                MinMaxRowType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    minValue: xmuxMaxConnectionsMin
                    maxValue: xmuxMaxConnectionsMax
                    onMinChanged: function(val) { xmuxMaxConnectionsMin = val; root.editDirty = false }
                    onMaxChanged: function(val) { xmuxMaxConnectionsMax = val; root.editDirty = false }
                    onEdited: root.editDirty = true
                }

                // cMaxReuseTimes
                CaptionTextType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 16
                    Layout.bottomMargin: 8
                    text: qsTr("cMaxReuseTimes")
                    color: AmneziaStyle.color.mutedGray
                }
                MinMaxRowType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    minValue: xmuxCMaxReuseTimesMin
                    maxValue: xmuxCMaxReuseTimesMax
                    onMinChanged: function(val) { xmuxCMaxReuseTimesMin = val; root.editDirty = false }
                    onMaxChanged: function(val) { xmuxCMaxReuseTimesMax = val; root.editDirty = false }
                    onEdited: root.editDirty = true
                }

                // hMaxRequestTimes
                CaptionTextType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 16
                    Layout.bottomMargin: 8
                    text: qsTr("hMaxRequestTimes")
                    color: AmneziaStyle.color.mutedGray
                }
                MinMaxRowType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    minValue: xmuxHMaxRequestTimesMin
                    maxValue: xmuxHMaxRequestTimesMax
                    onMinChanged: function(val) { xmuxHMaxRequestTimesMin = val; root.editDirty = false }
                    onMaxChanged: function(val) { xmuxHMaxRequestTimesMax = val; root.editDirty = false }
                    onEdited: root.editDirty = true
                }

                // hMaxReusableSecs
                CaptionTextType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 16
                    Layout.bottomMargin: 8
                    text: qsTr("hMaxReusableSecs")
                    color: AmneziaStyle.color.mutedGray
                }
                MinMaxRowType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    minValue: xmuxHMaxReusableSecsMin
                    maxValue: xmuxHMaxReusableSecsMax
                    onMinChanged: function(val) { xmuxHMaxReusableSecsMin = val; root.editDirty = false }
                    onMaxChanged: function(val) { xmuxHMaxReusableSecsMax = val; root.editDirty = false }
                    onEdited: root.editDirty = true
                }

                TextFieldWithHeaderType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 16
                    headerText: qsTr("hKeepAlivePeriod")
                    subtitleText: qsTr("Integer, may be negative")
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

        visible: listView.enabled && (XrayConfigModel.hasUnsavedChanges || root.editDirty)
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


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
                    onMinChanged: xmuxMaxConcurrencyMin = val
                    onMaxChanged: xmuxMaxConcurrencyMax = val
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
                    onMinChanged: xmuxMaxConnectionsMin = val
                    onMaxChanged: xmuxMaxConnectionsMax = val
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
                    onMinChanged: xmuxCMaxReuseTimesMin = val
                    onMaxChanged: xmuxCMaxReuseTimesMax = val
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
                    onMinChanged: xmuxHMaxRequestTimesMin = val
                    onMaxChanged: xmuxHMaxRequestTimesMax = val
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
                    onMinChanged: xmuxHMaxReusableSecsMin = val
                    onMaxChanged: xmuxHMaxReusableSecsMax = val
                }

                TextFieldWithHeaderType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 16
                    headerText: qsTr("hKeepAlivePeriod")
                    textField.text: xmuxHKeepAlivePeriod
                    textField.validator: IntValidator {
                        bottom: 0
                    }
                    textField.onEditingFinished: {
                        if (textField.text !== xmuxHKeepAlivePeriod) xmuxHKeepAlivePeriod = textField.text
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


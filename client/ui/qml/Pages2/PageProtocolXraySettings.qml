import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

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

    function formatTransport(value) {
        if (value === "raw") return "RAW (TCP)"
        if (value === "xhttp") return "XHTTP"
        if (value === "mkcp") return "mKCP"
        return value
    }

    function formatSecurity(value) {
        if (value === "none") return "None"
        if (value === "tls") return "TLS"
        if (value === "reality") return "Reality"
        return value
    }

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20 + PageController.safeAreaTopMargin

        onFocusChanged: {
            if (this.activeFocus) {
                listView.positionViewAtBeginning()
            }
        }
    }

    ListViewType {
        id: listView

        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        enabled: ServersUiController.isProcessedServerHasWriteAccess()
        model: XrayConfigModel

        delegate: ColumnLayout {
            width: listView.width

            property alias focusItemId: textFieldWithHeaderType.textField

            spacing: 0

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 0

                BaseHeaderType {
                    Layout.fillWidth: true
                    headerText: qsTr("XRay VLESS settings")
                }

                ImageButtonType {
                    Layout.alignment: Qt.AlignTop | Qt.AlignRight
                    implicitWidth: 40
                    implicitHeight: 40
                    image: "qrc:/images/controls/more-vertical.svg"
                    imageColor: AmneziaStyle.color.paleGray
                    onClicked: PageController.goToPage(PageEnum.PageProtocolXraySnapshots)
                }
            }

            LabelTextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 4
                text: qsTr("More about settings")
                color: AmneziaStyle.color.goldenApricot
                font.pixelSize: 16
                lineHeight: 24 + LanguageUiController.getLineHeightAppend()
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: Qt.openUrlExternally("https://docs.amnezia.org")
                }
            }

            TextFieldWithHeaderType {
                id: textFieldWithHeaderType
                Layout.fillWidth: true
                Layout.topMargin: 32
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                enabled: listView.enabled
                headerText: qsTr("Port")
                textField.text: port
                textField.maximumLength: 5
                textField.validator: IntValidator {
                    bottom: 1; top: 65535
                }
                textField.onEditingFinished: {
                    if (textField.text !== port) port = textField.text
                }
                checkEmptyText: true
            }

            LabelWithButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 16
                text: qsTr("Transport")
                descriptionText: root.formatTransport(transport)
                rightImageSource: "qrc:/images/controls/chevron-right.svg"
                enabled: listView.enabled
                clickedFunction: function() {
                    PageController.goToPage(PageEnum.PageProtocolXrayTransportSettings)
                }
            }

            DividerType {
            }

            LabelWithButtonType {
                Layout.fillWidth: true
                text: qsTr("Security")
                descriptionText: root.formatSecurity(security)
                rightImageSource: "qrc:/images/controls/chevron-right.svg"
                enabled: listView.enabled
                clickedFunction: function() {
                    PageController.goToPage(PageEnum.PageProtocolXraySecuritySettings)
                }
            }

            DividerType {
            }

            LabelWithButtonType {
                Layout.fillWidth: true
                text: qsTr("Flow")
                descriptionText: flow
                rightImageSource: "qrc:/images/controls/chevron-right.svg"
                enabled: listView.enabled
                clickedFunction: function() {
                    PageController.goToPage(PageEnum.PageProtocolXrayFlowSettings)
                }
            }

            DividerType {
            }

            Item {
                Layout.fillWidth: true; Layout.preferredHeight: 24
            }

            BasicButtonType {
                id: saveButton
                Layout.fillWidth: true
                Layout.bottomMargin: 8
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                // Show Save immediately while user edits port, even before focus loss.
                visible: listView.enabled && (XrayConfigModel.hasUnsavedChanges || textFieldWithHeaderType.textField.text !== port)
                enabled: visible && textFieldWithHeaderType.errorText === ""
                text: qsTr("Save")
                onClicked: function() {
                    forceActiveFocus()
                    var headerText = qsTr("Save settings?")
                    var descriptionText = qsTr("All users with whom you shared a connection with will no longer be able to connect to it.")
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")
                    var yesButtonFunction = function() {
                        if (ConnectionController.isConnected && ServersModel.getDefaultServerData("defaultContainer") === ServersUiController.processedContainerIndex) {
                            PageController.showNotificationMessage(qsTr("Unable change settings while there is an active connection"))
                            return
                        }

                        PageController.goToPage(PageEnum.PageSetupWizardInstalling);
                        InstallController.updateContainer(ServersUiController.getServerId(ServersUiController.processedServerIndex), ServersUiController.processedContainerIndex, ProtocolEnum.Xray)
                    }
                    var noButtonFunction = function() {
                        if (!GC.isMobile()) saveButton.forceActiveFocus()
                    }
                    showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }
                Keys.onEnterPressed: saveButton.clicked()
                Keys.onReturnPressed: saveButton.clicked()
            }

            LabelWithButtonType {
                Layout.fillWidth: true
                text: qsTr("Reset settings")
                textColor: AmneziaStyle.color.vibrantRed
                visible: listView.enabled
                clickedFunction: function() {
                    var yesButtonFunction = function() {
                        XrayConfigModel.resetToDefaults()
                    }
                    showQuestionDrawer(qsTr("Reset settings?"), qsTr("All XRay settings will be restored to defaults."),
                        qsTr("Reset"), qsTr("Cancel"), yesButtonFunction, function() {
                        })
                }
            }

            Item {
                Layout.fillWidth: true; Layout.preferredHeight: 32
            }
        }
    }
}

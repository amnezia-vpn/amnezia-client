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

    focusOnActivation: false

    property bool portDirty: false
    property string portText: ""

    function save() {
        var errs = XrayConfigModel.validationErrors()
        if (errs.length > 0) {
            PageController.showErrorMessage(errs.join("\n"))
            return
        }

        var headerText = qsTr("Save settings?")
        var descriptionText = XrayConfigModel.saveDescription(root.portText)
        var yesButtonText = qsTr("Continue")
        var noButtonText = qsTr("Cancel")
        var yesButtonFunction = function() {
            if (ConnectionController.isConnected && ServersUiController.serverDefaultContainer(ServersUiController.defaultServerId) === ServersUiController.processedContainerIndex) {
                PageController.showNotificationMessage(qsTr("Unable change settings while there is an active connection"))
                return
            }

            var content = listView.itemAtIndex(0)
            if (content) {
                content.commitPort()
            }

            PageController.goToPage(PageEnum.PageSetupWizardInstalling);
            InstallController.updateServerConfig(ServersUiController.processedServerId, ServersUiController.processedContainerIndex, ProtocolEnum.Xray)
        }
        var noButtonFunction = function() {}
        showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
    }

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

    function formatFlow(value) {
        return value === "" ? qsTr("Empty") : value
    }

    Connections {
        target: InstallController

        function onXrayKeyMigrationNeedsConfirm(message) {
            PageController.showBusyIndicator(false)
            showQuestionDrawer(
                qsTr("Recreate XRay with new keys?"),
                message,
                qsTr("Continue"),
                qsTr("Cancel"),
                function() {
                    InstallController.updateServerConfig(ServersUiController.processedServerId,
                                                         ServersUiController.processedContainerIndex,
                                                         ProtocolEnum.Xray, true, true)
                },
                function() {})
        }
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

            title: qsTr("XRay VLESS settings")

            IconButtonType {
                size: IconButtonType.Small
                icon.source: "qrc:/images/controls/more-vertical.svg"
                onClicked: PageController.goToPage(PageEnum.PageProtocolXraySnapshots)
            }
        }

        delegate: ColumnLayout {
            width: listView.width

            spacing: 0

            function commitPort() {
                if (portField.textField.text !== port) {
                    port = portField.textField.text
                }
            }

            SmallTextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 16
                visible: !listView.enabled
                color: AmneziaStyle.color.textSecondary
                text: qsTr("You have read-only access to this server. XRay settings cannot be edited.")
            }

            LinkButtonType {
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 16

                size: LinkButtonType.Medium
                horizontalPadding: 14
                verticalPadding: 8

                text: qsTr("More about settings")

                onClicked: Qt.openUrlExternally("https://docs.amnezia.org")
            }

            TextFieldType {
                id: portField

                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                enabled: listView.enabled
                headerText: qsTr("Port")
                placeholderText: XrayConfigModel.portDefault()
                checkEmptyText: true

                Binding {
                    target: portField.textField
                    property: "text"
                    value: port
                    when: !portField.textField.activeFocus
                    restoreMode: Binding.RestoreNone
                }

                textField.maximumLength: 5
                textField.validator: RegularExpressionValidator {
                    regularExpression: /^(|\d{1,4}|[1-5]\d{4}|6[0-4]\d{3}|65[0-4]\d{2}|655[0-2]\d|6553[0-5])$/
                }
                textField.onActiveFocusChanged: {
                    if (textField.activeFocus && textField.text === "" && port !== "") {
                        textField.text = port
                    }
                }
                textField.onTextChanged: {
                    root.portText = textField.text
                    root.portDirty = (textField.text !== port)
                }
                textField.onEditingFinished: {
                    var v = portField.textField.text
                    if (v !== "") {
                        var n = parseInt(v, 10)
                        if (isNaN(n) || n < 1)
                            n = 1
                        if (n > 65535)
                            n = 65535
                        v = String(n)
                        if (portField.textField.text !== v)
                            portField.textField.text = v
                    }
                    if (v !== port)
                        port = v
                    root.portDirty = false
                }
            }

            ListItemType {
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                enabled: listView.enabled
                text: qsTr("Transport")
                subtitle: root.formatTransport(transport)
                trailingIconSource: "qrc:/images/controls/chevron-right.svg"

                onClicked: PageController.goToPage(PageEnum.PageProtocolXrayTransportSettings)
            }

            ListItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                enabled: listView.enabled
                text: qsTr("Security")
                subtitle: root.formatSecurity((security === "reality" && transport === "mkcp") ? "none" : security)
                trailingIconSource: "qrc:/images/controls/chevron-right.svg"

                onClicked: PageController.goToPage(PageEnum.PageProtocolXraySecuritySettings)
            }

            ListItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                enabled: listView.enabled
                text: qsTr("Flow")
                subtitle: root.formatFlow((transport === "" || transport === "raw") ? flow : "")
                trailingIconSource: "qrc:/images/controls/chevron-right.svg"

                onClicked: PageController.goToPage(PageEnum.PageProtocolXrayFlowSettings)
            }

            ListItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: listView.enabled
                text: qsTr("Reset settings")
                destructive: true

                onClicked: {
                    var yesButtonFunction = function() {
                        PageController.showNotificationMessage(
                            qsTr("Settings were reset to defaults. Tap Save to apply them on the server."))
                        XrayConfigModel.resetToDefaults()
                    }
                    showQuestionDrawer(qsTr("Reset settings?"), qsTr("All XRay settings will be restored to defaults."),
                        qsTr("Reset"), qsTr("Cancel"), yesButtonFunction, function() {
                        })
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

        visible: listView.enabled && (XrayConfigModel.hasUnsavedChanges || root.portDirty)
        enabled: visible && root.portText !== ""
        text: qsTr("Save")

        onClicked: {
            saveButton.forceActiveFocus()
            root.save()
        }
    }
}

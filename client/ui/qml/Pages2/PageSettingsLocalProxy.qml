import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

PageType {
    id: root

    readonly property int localProxyPortMin: 1024
    readonly property int localProxyPortMax: 65535
    readonly property int defaultLocalProxyPort: 10808

    property bool portValidationEnabled: false
    property string portValidationError: ""
    property bool suppressToggleHandler: false

    Component.onCompleted: root.syncSwitchState()

    function computePortErrorText() {
        const text = portField.textField.text.trim()
        if (text === "") {
            return qsTr("Enter a port")
        }
        const value = parseInt(text)
        if (isNaN(value) || value < root.localProxyPortMin || value > root.localProxyPortMax) {
            return qsTr("Port must be between %1 and %2")
                .arg(root.localProxyPortMin)
                .arg(root.localProxyPortMax)
        }
        return ""
    }

    function updatePortValidation(force) {
        if (force) {
            root.portValidationEnabled = true
        }
        root.portValidationError = root.portValidationEnabled ? root.computePortErrorText() : ""
    }

    function isPortValid() {
        return root.computePortErrorText() === ""
    }

    function syncSwitchState() {
        setSwitcherChecked(SettingsController.isLocalProxyHttpEnabled)
    }

    function setSwitcherChecked(value) {
        if (localProxyHeader.switcher.checked === value) {
            return
        }
        root.suppressToggleHandler = true
        localProxyHeader.switcher.checked = value
    }

    function handleLocalProxyToggle(checked) {
        if (checked) {
            const serverUuid = ServersModel.processedServerUuid
            if (!serverUuid) {
                root.setSwitcherChecked(false)
                PageController.showNotificationMessage(qsTr("Unable to determine the current server"))
                return
            }

            if (SettingsController.isLocalProxyHttpEnabled
                    && SettingsController.localProxyOwnerUuid
                    && SettingsController.localProxyOwnerUuid !== serverUuid) {
                root.setSwitcherChecked(false)
                PageController.showNotificationMessage(qsTr("Local proxy is already enabled for another server"))
                return
            }

            const requestedPort = portField.portValue()
            if (requestedPort < root.localProxyPortMin || requestedPort > root.localProxyPortMax) {
                root.setSwitcherChecked(false)
                PageController.showNotificationMessage(qsTr("Port must be between %1 and %2")
                    .arg(root.localProxyPortMin)
                    .arg(root.localProxyPortMax))
                return
            }

            if (!SettingsController.enableLocalProxy(serverUuid, requestedPort)) {
                root.setSwitcherChecked(false)
                PageController.showNotificationMessage(qsTr("Failed to enable local proxy. Check the port (%1-%2).")
                    .arg(root.localProxyPortMin)
                    .arg(root.localProxyPortMax))
                root.syncSwitchState()
                return
            }
            root.syncSwitchState()
        } else {
            SettingsController.disableLocalProxy()
            root.syncSwitchState()
        }
    }

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20 + SettingsController.safeAreaTopMargin

        onActiveFocusChanged: {
            if (activeFocus) {
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

        header: ColumnLayout {
            width: listView.width

            HeaderTypeWithSwitcher {
                id: localProxyHeader

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                headerText: qsTr("Local Proxy")
                descriptionText: qsTr("Use a proxy to route selected apps (for example, the CensorTracker extension) through Amnezia Premium.")
                showSwitcher: true
                switcherFunction: function(checked) {
                    if (root.suppressToggleHandler) {
                        root.suppressToggleHandler = false
                        return
                    }
                    root.handleLocalProxyToggle(checked)
                }
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Only one can be on at a time: VPN or local proxy.")
            }

            BasicButtonType {
                Layout.topMargin: 8
                Layout.leftMargin: 8
                implicitHeight: 32

                defaultColor: AmneziaStyle.color.transparent
                hoveredColor: AmneziaStyle.color.translucentWhite
                pressedColor: AmneziaStyle.color.sheerWhite
                disabledColor: AmneziaStyle.color.mutedGray
                textColor: AmneziaStyle.color.goldenApricot

                text: qsTr("Learn more")
                clickedFunc: function() {
                    Qt.openUrlExternally(LanguageModel.getCurrentSiteUrl())
                }
            }
        }

        model: 1 // fake model to force the ListView to be created without a model

        delegate: ColumnLayout {
            width: listView.width
            spacing: 16

            TextFieldWithHeaderType {
                id: portField

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Address and port")
                buttonText: qsTr("Copy")
                errorText: root.portValidationError

                enabled: true
                rightButtonClickedOnEnter: true

                clickedFunc: function() {
                    const portText = portField.effectivePortText()
                    GC.copyToClipBoard("127.0.0.1:" + portText)
                }

                textField.validator: IntValidator {
                    bottom: root.localProxyPortMin
                    top: root.localProxyPortMax
                }
                textField.leftPadding: portPrefix.implicitWidth + 8
                textField.placeholderText: root.defaultLocalProxyPort.toString()
                textField.inputMethodHints: Qt.ImhDigitsOnly | Qt.ImhNoPredictiveText

                function syncPortValue() {
                    const port = SettingsController.localProxyPort
                    textField.text = (port >= root.localProxyPortMin && port <= root.localProxyPortMax) ? port.toString() : ""
                }

                function portValue() {
                    const value = parseInt(textField.text)
                    return isNaN(value) ? -1 : value
                }

                function effectivePortText() {
                    const value = portValue()
                    if (value >= root.localProxyPortMin && value <= root.localProxyPortMax) {
                        return value.toString()
                    }
                    const fallback = SettingsController.localProxyPort
                    if (fallback >= root.localProxyPortMin && fallback <= root.localProxyPortMax) {
                        return fallback.toString()
                    }
                    return root.defaultLocalProxyPort.toString()
                }

                Component.onCompleted: syncPortValue()

                textField.onTextChanged: root.updatePortValidation(false)
                textField.onActiveFocusChanged: {
                    if (!textField.activeFocus) {
                        root.updatePortValidation(true)
                    }
                }
            }

            LabelTextType {
                id: portPrefix

                parent: portField
                text: "127.0.0.1:"
                color: AmneziaStyle.color.paleGray
                z: 1

                anchors.left: portField.textField.left
                anchors.verticalCenter: portField.textField.verticalCenter
            }

            BasicButtonType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Save")
                enabled: root.isPortValid()

                clickedFunc: function() {
                    root.updatePortValidation(true)
                    if (!root.isPortValid()) {
                        return
                    }
                    const value = portField.portValue()
                    if (!SettingsController.setLocalProxyPort(value)) {
                        PageController.showNotificationMessage(qsTr("Failed to save port. Valid range: %1-%2")
                            .arg(root.localProxyPortMin)
                            .arg(root.localProxyPortMax))
                    }
                    portField.syncPortValue()
                }
            }
        }
    }

    Connections {
        target: SettingsController

        function onLocalProxySettingsUpdated() {
            root.syncSwitchState()
            if (!portField.textField.activeFocus) {
                portField.syncPortValue()
            }
        }

        function onLocalProxyStartFailed(message) {
            PageController.showNotificationMessage(message)
            root.syncSwitchState()
        }
    }

    Connections {
        target: ServersModel

        function onProcessedServerChanged() {
            root.syncSwitchState()
            if (!portField.textField.activeFocus) {
                portField.syncPortValue()
            }
        }
    }
}


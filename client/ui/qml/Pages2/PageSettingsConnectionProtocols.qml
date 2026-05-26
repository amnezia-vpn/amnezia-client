import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Config"

PageType {
    id: root

    // Protocol to re-assert after updateServiceFromGateway completes (empty = auto)
    property string pendingProtocol: ""
    property bool waitingForGatewayUpdate: false

    Timer {
        id: updateProtocolTimer
        interval: 100
        repeat: false
        onTriggered: {
            var serverId = ServersUiController.getServerId(ServersUiController.processedServerIndex)
            root.waitingForGatewayUpdate = true
            SubscriptionUiController.updateServiceFromGateway(serverId, "", "", true)
        }
    }

    function getCurrentProtocol() {
        try {
            if (SubscriptionUiController.isVlessProtocol(ServersUiController.getServerId(ServersUiController.processedServerIndex))) {
                return "vless"
            }
            
            if (SubscriptionUiController.isAwgProtocol(ServersUiController.getServerId(ServersUiController.processedServerIndex))) {
                return "awg"
            }
            
            // If neither VLESS nor AWG, it's auto mode
            return "auto"
        } catch (e) {
            console.log("Error getting current protocol:", e)
            return "auto"
        }
    }

    property string currentProtocol: "auto"

    Connections {
        target: ServersModel
        function onDataChanged() {
            if (!root || !root.visible) {
                return
            }
            Qt.callLater(function() {
                try {
                    if (root && root.visible && typeof root.getCurrentProtocol === "function") {
                        root.currentProtocol = root.getCurrentProtocol()
                    }
                } catch (e) {
                    console.log("Error in ServersModel.onDataChanged:", e)
                }
            })
        }
    }

    Connections {
        target: SubscriptionUiController
        function onUpdateServerFromApiFinished() {
            if (!root.waitingForGatewayUpdate) {
                return
            }
            root.waitingForGatewayUpdate = false

            // Re-assert the protocol the user chose (gateway reload may have reset it)
            if (root.pendingProtocol !== "") {
                var protocolToSet = root.pendingProtocol === "auto" ? "" : root.pendingProtocol
                SubscriptionUiController.setCurrentProtocol(
                    ServersUiController.getServerId(ServersUiController.processedServerIndex),
                    protocolToSet)
            }

            root.currentProtocol = root.getCurrentProtocol()
        }
    }

    Component.onCompleted: {
        root.currentProtocol = root.getCurrentProtocol()
    }

    onVisibleChanged: {
        if (visible) {
            try {
                if (typeof root.getCurrentProtocol === "function") {
                    root.currentProtocol = root.getCurrentProtocol()
                }
            } catch (e) {
                console.log("Error in onVisibleChanged:", e)
            }
        }
    }

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20 + SettingsController.safeAreaTopMargin

        onActiveFocusChanged: {
            if(backButton.enabled && backButton.activeFocus) {
                flickable.contentY = 0
            }
        }
    }

    FlickableType {
        id: flickable

        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        contentHeight: content.height

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 16

                headerText: qsTr("VPN Protocol")
            }

            ButtonGroup {
                id: protocolButtonGroup
            }

            VerticalRadioButton {
                id: autoProtocolButton

                Layout.fillWidth: true
                Layout.topMargin: 32
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                ButtonGroup.group: protocolButtonGroup
                checked: root.currentProtocol === "auto"
                enabled: !ConnectionController.isConnected || !ServersModel.isDefaultServerCurrentlyProcessed()

                text: qsTr("Choose automatically")
                descriptionText: qsTr("AmneziaWG is used by default. If the connection is unstable, the app will switch to VLESS. On the next launch, AmneziaWG will be tried again")

                onClicked: function() {
                    if (ServersModel.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected) {
                        PageController.showNotificationMessage(qsTr("Cannot change protocol during active connection"))
                        return
                    }

                    SubscriptionUiController.setCurrentProtocol(ServersUiController.getServerId(ServersUiController.processedServerIndex), "")
                    root.pendingProtocol = "auto"
                    updateProtocolTimer.start()
                }

                Keys.onEnterPressed: this.clicked()
                Keys.onReturnPressed: this.clicked()
            }

            DividerType {}

            VerticalRadioButton {
                id: awgProtocolButton

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                ButtonGroup.group: protocolButtonGroup
                checked: root.currentProtocol === "awg"
                enabled: !ConnectionController.isConnected || !ServersModel.isDefaultServerCurrentlyProcessed()

                text: qsTr("AmneziaWG")

                onClicked: function() {
                    if (ServersModel.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected) {
                        PageController.showNotificationMessage(qsTr("Cannot change protocol during active connection"))
                        return
                    }

                    root.currentProtocol = "awg"
                    root.pendingProtocol = "awg"
                    SubscriptionUiController.setCurrentProtocol(ServersUiController.getServerId(ServersUiController.processedServerIndex), "awg")
                    updateProtocolTimer.start()
                }

                Keys.onEnterPressed: this.clicked()
                Keys.onReturnPressed: this.clicked()
            }

            DividerType {}

            VerticalRadioButton {
                id: vlessProtocolButton

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                ButtonGroup.group: protocolButtonGroup
                checked: root.currentProtocol === "vless"
                enabled: !ConnectionController.isConnected || !ServersModel.isDefaultServerCurrentlyProcessed()

                text: qsTr("XRay VLESS Reality")

                onClicked: function() {
                    if (ServersModel.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected) {
                        PageController.showNotificationMessage(qsTr("Cannot change protocol during active connection"))
                        return
                    }

                    root.currentProtocol = "vless"
                    root.pendingProtocol = "vless"
                    SubscriptionUiController.setCurrentProtocol(ServersUiController.getServerId(ServersUiController.processedServerIndex), "vless")
                    updateProtocolTimer.start()
                }

                Keys.onEnterPressed: this.clicked()
                Keys.onReturnPressed: this.clicked()
            }

            DividerType {}
        }
    }
}

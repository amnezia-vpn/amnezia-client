import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import ConnectionState 1.0

Button {
    id: root

    Connections {
        target: ConnectionController

        function onConnectionErrorOccurred(errorMessage) {
            PageController.showErrorMessage(errorMessage)
        }
    }

    text: qsTr("Connect")

    background: Item {
        clip: true

        implicitHeight: border.implicitHeight
        implicitWidth: border.implicitWidth

        Image {
            id: border

            source: connectionProccess.running ? "/images/connectionProgress.svg" :
                                                 ConnectionController.isConnected ? "/images/connectionOff.svg" : "/images/connectionOn.svg"
            RotationAnimator {
                id: connectionProccess

                target: border
                running: false
                from: 0
                to: 360
                loops: Animation.Infinite
                duration: 1250
            }

            Behavior on source {
                PropertyAnimation { duration: 200 }
            }
        }

        MouseArea {
            anchors.fill: parent

            cursorShape: Qt.PointingHandCursor
            enabled: false
        }
    }

    contentItem: Text {
        height: 24

        font.family: "PT Root UI VF"
        font.weight: 700
        font.pixelSize: 20

        color: "#D7D8DB"
        text: root.text

        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    onClicked: {
        connectionProccess.running ? ConnectionController.closeConnection() : ConnectionController.openConnection()
    }

    Connections {
        target: ConnectionController
        function onConnectionStateChanged(state) {
            switch(state) {
                case ConnectionState.Unknown: {
                    console.log("Unknown")
                    break
                }
                case ConnectionState.Disconnected: {
                    console.log("Disconnected")
                    connectionProccess.running = false
                    root.text = qsTr("Connect")
                    ConnectionController.isConnected = false
                    break
                }
                case ConnectionState.Preparing: {
                    console.log("Preparing")
                    connectionProccess.running = true
                    root.text = qsTr("Connection...")
                    break
                }
                case ConnectionState.Connecting: {
                    console.log("Connecting")
                    connectionProccess.running = true
                    root.text = qsTr("Connection...")
                    break
                }
                case ConnectionState.Connected: {
                    console.log("Connected")
                    connectionProccess.running = false
                    root.text = qsTr("Disconnect")
                    ConnectionController.isConnected = true
                    break
                }
                case ConnectionState.Disconnecting: {
                    console.log("Disconnecting")
                    connectionProccess.running = true
                    root.text = qsTr("Disconnection...")
                    break
                }
                case ConnectionState.Reconnecting: {
                    console.log("Reconnecting")
                    connectionProccess.running = true
                    root.text = qsTr("Reconnection...")
                    break
                }
                case ConnectionState.Error: {
                    console.log("Error")
                    connectionProccess.running = false
                    root.text = qsTr("Connect")
                    PageController.showErrorMessage(ConnectionController.getLastConnectionError())
                    break
                }
            }
        }
    }
}

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

    text: ConnectionController.connectionStateText

    enabled: !ConnectionController.isConnectionInProgress

    background: Item {
        clip: true

        implicitHeight: border.implicitHeight
        implicitWidth: border.implicitWidth

        Image {
            id: border

            source: {
                if (ConnectionController.isConnectionInProgress) {
                    return "/images/connectionProgress.svg"
                } else if (ConnectionController.isConnected) {
                    return "/images/connectionOff.svg"
                } else {
                   return "/images/connectionOn.svg"
                }
            }

            RotationAnimator {
                id: connectionProccess

                target: border
                running: ConnectionController.isConnectionInProgress
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
        if (ConnectionController.isConnectionInProgress) {
            ConnectionController.closeConnection()
        } else if (ConnectionController.isConnected) {
            ConnectionController.closeConnection()
        } else {
            ConnectionController.openConnection()
        }
    }
}

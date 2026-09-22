pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"

DrawerType2 {
    id: root

    property string otpCode: ""
    property int secondsLeft: 0
    property bool confirmed: false

    readonly property bool expired: !confirmed && secondsLeft <= 0

    function showCode(code, expiresInSec) {
        root.otpCode = code
        root.secondsLeft = expiresInSec
        root.confirmed = false
        countdown.restart()
        statusPoll.restart()
        root.openTriggered()
    }

    function formattedCode() {
        var half = Math.ceil(root.otpCode.length / 2)
        return root.otpCode.length > 4
                ? root.otpCode.substring(0, half) + " " + root.otpCode.substring(half)
                : root.otpCode
    }

    function formattedTime() {
        var minutes = Math.floor(root.secondsLeft / 60)
        var seconds = root.secondsLeft % 60
        return (minutes < 10 ? "0" : "") + minutes + ":" + (seconds < 10 ? "0" : "") + seconds
    }

    onClosed: {
        countdown.stop()
        statusPoll.stop()
    }

    Timer {
        id: countdown

        interval: 1000
        repeat: true

        onTriggered: {
            if (root.secondsLeft > 0) {
                root.secondsLeft -= 1
            } else {
                stop()
                statusPoll.stop()
            }
        }
    }

    Timer {
        id: statusPoll

        interval: 3000
        repeat: true

        onTriggered: {
            SubscriptionUiController.checkOtpStatus()
        }
    }

    Connections {
        target: SubscriptionUiController

        function onOtpConfirmed() {
            root.confirmed = true
            countdown.stop()
            statusPoll.stop()
        }

        function onOtpExpired() {
            root.secondsLeft = 0
            countdown.stop()
            statusPoll.stop()
        }
    }

    expandedStateContent: ColumnLayout {
        id: content

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        spacing: 0

        onImplicitHeightChanged: {
            root.expandedHeight = content.implicitHeight + 32 + PageController.safeAreaBottomMargin
        }

        Item {
            Layout.fillWidth: true
            Layout.topMargin: 8
            Layout.leftMargin: 16
            Layout.rightMargin: 8
            implicitHeight: closeButton.implicitHeight

            Header2TextType {
                anchors.left: parent.left
                anchors.right: closeButton.left
                anchors.verticalCenter: parent.verticalCenter

                text: qsTr("Log in to your account")
            }

            ImageButtonType {
                id: closeButton

                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter

                implicitWidth: 40
                implicitHeight: 40

                image: "qrc:/images/controls/close.svg"
                imageColor: AmneziaStyle.color.paleGray

                onClicked: {
                    root.closeTriggered()
                }
            }
        }

        ParagraphTextType {
            Layout.fillWidth: true
            Layout.topMargin: 8
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            text: qsTr("Enter this code in your account to sign in")
        }

        Header1TextType {
            Layout.fillWidth: true
            Layout.topMargin: 24
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            horizontalAlignment: Text.AlignHCenter
            font.letterSpacing: 4
            opacity: root.expired ? 0.4 : 1.0

            text: root.formattedCode()
        }

        ParagraphTextType {
            Layout.fillWidth: true
            Layout.topMargin: 8
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            horizontalAlignment: Text.AlignHCenter
            color: root.confirmed ? AmneziaStyle.color.vibrantGreen
                                  : (root.expired ? AmneziaStyle.color.vibrantRed : AmneziaStyle.color.mutedGray)

            text: root.confirmed ? qsTr("Signed in successfully")
                                 : (root.expired ? qsTr("Code has expired")
                                                 : qsTr("Code expires in %1").arg(root.formattedTime()))
        }

        BasicButtonType {
            Layout.fillWidth: true
            Layout.topMargin: 16
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.bottomMargin: 8

            visible: root.confirmed

            text: qsTr("Done")

            clickedFunc: function() {
                root.closeTriggered()
            }
        }

        BasicButtonType {
            Layout.fillWidth: true
            Layout.topMargin: 16
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.bottomMargin: 8

            visible: root.expired

            text: qsTr("Get new code")

            clickedFunc: function() {
                PageController.showBusyIndicator(true)
                SubscriptionUiController.otpLogin(ServersUiController.processedServerId)
                PageController.showBusyIndicator(false)
            }
        }
    }
}

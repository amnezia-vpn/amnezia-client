import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

Item {
    id: root

    property bool emailMode: false
    property string errorMessage: ""

    anchors.fill: parent
    focus: true

    Component.onCompleted: {
        console.log("PageTvLogin loaded")
        if (FBLinkController.tvLoginStatus === "") {
            FBLinkController.startTvLogin()
        }
        codeTab.forceActiveFocus()
    }

    Connections {
        target: FBLinkController
        function onLoginError(message) { root.errorMessage = message }
    }

    Timer {
        id: pollTimer
        interval: Math.max(3000, FBLinkController.tvLoginPollIntervalMs)
        repeat: true
        running: !root.emailMode && FBLinkController.tvLoginStatus === "pending"
        onTriggered: FBLinkController.pollTvLogin()
    }

    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Left || event.key === Qt.Key_Right) {
            root.emailMode = !root.emailMode
            root.emailMode ? emailTab.forceActiveFocus() : codeTab.forceActiveFocus()
            event.accepted = true
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "#08090B"
    }

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 120, 1040)
        spacing: 28

        Label {
            Layout.fillWidth: true
            text: "FBLink VPN TV"
            color: "#F8FAFC"
            font.pixelSize: 52
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 14

            Button {
                id: codeTab
                text: "Code / QR"
                font.pixelSize: 24
                padding: 18
                highlighted: !root.emailMode
                onClicked: root.emailMode = false
                KeyNavigation.right: emailTab
                KeyNavigation.down: refreshCodeButton
            }

            Button {
                id: emailTab
                text: "Email / password"
                font.pixelSize: 24
                padding: 18
                highlighted: root.emailMode
                onClicked: root.emailMode = true
                KeyNavigation.left: codeTab
                KeyNavigation.down: emailField
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: root.emailMode ? 390 : 430
            radius: 22
            color: "#111216"
            border.width: 2
            border.color: "#2D3038"

            Item {
                anchors.fill: parent
                anchors.margins: 34

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 18
                    visible: !root.emailMode

                    Label {
                        Layout.fillWidth: true
                        text: "Open the link on your phone, or scan the QR code."
                        color: "#D4D4D8"
                        font.pixelSize: 24
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 30

                        Rectangle {
                            Layout.preferredWidth: 250
                            Layout.preferredHeight: 250
                            Layout.alignment: Qt.AlignVCenter
                            radius: 16
                            color: "#FFFFFF"

                            Image {
                                anchors.fill: parent
                                anchors.margins: 14
                                source: FBLinkController.tvLoginQrCodeImage
                                fillMode: Image.PreserveAspectFit
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignVCenter
                            spacing: 12

                            Label {
                                Layout.fillWidth: true
                                text: FBLinkController.tvLoginUserCode === "" ? "Loading code..." : FBLinkController.tvLoginUserCode
                                color: "#FACC15"
                                font.pixelSize: 54
                                font.bold: true
                                horizontalAlignment: Text.AlignHCenter
                            }

                            Label {
                                Layout.fillWidth: true
                                text: FBLinkController.tvLoginVerificationUrl
                                color: "#A1A1AA"
                                font.pixelSize: 20
                                wrapMode: Text.WrapAnywhere
                                horizontalAlignment: Text.AlignHCenter
                            }

                            Label {
                                Layout.fillWidth: true
                                text: FBLinkController.tvLoginError !== "" ? FBLinkController.tvLoginError : "Waiting for confirmation..."
                                color: FBLinkController.tvLoginError !== "" ? "#F87171" : "#A1A1AA"
                                font.pixelSize: 20
                                horizontalAlignment: Text.AlignHCenter
                            }

                            Button {
                                id: refreshCodeButton
                                Layout.alignment: Qt.AlignHCenter
                                text: "New code"
                                font.pixelSize: 22
                                padding: 16
                                onClicked: FBLinkController.startTvLogin()
                                KeyNavigation.up: codeTab
                            }
                        }
                    }
                }

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 16
                    visible: root.emailMode

                    TextField {
                        id: emailField
                        Layout.fillWidth: true
                        placeholderText: "Email"
                        font.pixelSize: 24
                        inputMethodHints: Qt.ImhEmailCharactersOnly
                        KeyNavigation.down: passwordField
                    }

                    TextField {
                        id: passwordField
                        Layout.fillWidth: true
                        placeholderText: "Password"
                        echoMode: TextInput.Password
                        font.pixelSize: 24
                        KeyNavigation.up: emailField
                        KeyNavigation.down: loginButton
                    }

                    Label {
                        Layout.fillWidth: true
                        text: root.errorMessage
                        visible: text !== ""
                        color: "#F87171"
                        font.pixelSize: 18
                        wrapMode: Text.WordWrap
                    }

                    Button {
                        id: loginButton
                        Layout.fillWidth: true
                        Layout.preferredHeight: 70
                        text: FBLinkController.isLoading ? "Signing in..." : "Sign in"
                        font.pixelSize: 26
                        enabled: !FBLinkController.isLoading
                        KeyNavigation.up: passwordField
                        onClicked: {
                            root.errorMessage = ""
                            FBLinkController.login(emailField.text.trim(), passwordField.text)
                        }
                    }
                }
            }
        }
    }
}

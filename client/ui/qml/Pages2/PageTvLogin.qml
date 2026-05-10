import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../Components"

FocusScope {
    id: root

    width: parent ? parent.width : 1920
    height: parent ? parent.height : 1080
    focus: true
    clip: true

    property string errorMessage: ""

    function isOkKey(event) {
        return event.key === Qt.Key_Return
            || event.key === Qt.Key_Enter
            || event.key === Qt.Key_Select
            || event.key === Qt.Key_Space
    }

    function submitLogin() {
        root.errorMessage = ""
        if (emailField.text.trim() === "") {
            root.errorMessage = "Enter email"
            emailField.forceActiveFocus()
            return
        }
        if (passwordField.text === "") {
            root.errorMessage = "Enter password"
            passwordField.forceActiveFocus()
            return
        }
        FBLinkController.login(emailField.text.trim(), passwordField.text)
    }

    Component.onCompleted: {
        console.log("PageTvLogin loaded")
        loginButton.forceActiveFocus()
    }

    Connections {
        target: FBLinkController
        function onLoginError(message) { root.errorMessage = message }
    }

    Rectangle {
        anchors.fill: parent
        color: "#070707"
    }

    Item {
        id: stage
        width: 1920
        height: 1080
        scale: Math.min(1, root.width / width, root.height / height) * 0.92
        anchors.centerIn: parent

        Item {
            id: brandPane
            x: 190
            y: 132
            width: 650
            height: 680

            Image {
                id: logo
                x: 0
                y: 0
                width: 230
                height: 230
                source: "qrc:/images/fblink_logo.png"
                fillMode: Image.PreserveAspectFit
            }

            Label {
                x: 0
                y: 300
                width: parent.width
                text: "FBLink VPN"
                color: "#F8FAFC"
                font.pixelSize: 64
                font.bold: true
            }

            Label {
                x: 0
                y: 410
                width: parent.width
                text: "Android TV"
                color: "#FACC15"
                font.pixelSize: 34
                font.bold: true
            }

            Label {
                x: 0
                y: 486
                width: parent.width
                text: "Sign in with your account to connect from this TV."
                color: "#A1A1AA"
                font.pixelSize: 28
                lineHeight: 1.18
                wrapMode: Text.WordWrap
            }
        }

        Rectangle {
            id: loginCard
            x: 1010
            y: 230
            width: 720
            height: 620
            radius: 28
            color: "#121212"
            border.width: 2
            border.color: "#2F2F2F"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 48
                spacing: 24

                Label {
                    Layout.fillWidth: true
                    text: "Sign in"
                    color: "#F8FAFC"
                    font.pixelSize: 46
                    font.bold: true
                }

                TextField {
                    id: emailField
                    Layout.fillWidth: true
                    Layout.preferredHeight: 82
                    placeholderText: "Email"
                    font.pixelSize: 28
                    inputMethodHints: Qt.ImhEmailCharactersOnly
                    KeyNavigation.down: passwordField
                    background: Rectangle {
                        radius: 16
                        color: "#18181B"
                        border.width: emailField.activeFocus ? 3 : 1
                        border.color: emailField.activeFocus ? "#FACC15" : "#3F3F46"
                    }
                    Keys.onPressed: function(event) {
                        if (root.isOkKey(event)) {
                            passwordField.forceActiveFocus()
                            event.accepted = true
                        }
                    }
                }

                TextField {
                    id: passwordField
                    Layout.fillWidth: true
                    Layout.preferredHeight: 82
                    placeholderText: "Password"
                    echoMode: TextInput.Password
                    font.pixelSize: 28
                    KeyNavigation.up: emailField
                    KeyNavigation.down: loginButton
                    background: Rectangle {
                        radius: 16
                        color: "#18181B"
                        border.width: passwordField.activeFocus ? 3 : 1
                        border.color: passwordField.activeFocus ? "#FACC15" : "#3F3F46"
                    }
                    Keys.onPressed: function(event) {
                        if (root.isOkKey(event)) {
                            loginButton.forceActiveFocus()
                            event.accepted = true
                        }
                    }
                }

                Label {
                    Layout.fillWidth: true
                    Layout.preferredHeight: visible ? implicitHeight : 0
                    text: root.errorMessage
                    visible: text !== ""
                    color: "#F87171"
                    font.pixelSize: 24
                    wrapMode: Text.WordWrap
                }

                TvButton {
                    id: loginButton
                    Layout.fillWidth: true
                    Layout.preferredHeight: 86
                    text: FBLinkController.isLoading ? "Signing in..." : "Sign in"
                    tvFontPixelSize: 30
                    enabled: !FBLinkController.isLoading
                    KeyNavigation.up: passwordField
                    onClicked: root.submitLogin()
                    Keys.onPressed: function(event) {
                        if (root.isOkKey(event)) {
                            clicked()
                            event.accepted = true
                        }
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }
    }
}

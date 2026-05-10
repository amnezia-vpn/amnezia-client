import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

FocusScope {
    id: root

    property string errorMessage: ""

    anchors.fill: parent
    focus: true

    function isOkKey(event) {
        return event.key === Qt.Key_Return
            || event.key === Qt.Key_Enter
            || event.key === Qt.Key_Select
            || event.key === Qt.Key_Space
    }

    function submitLogin() {
        root.errorMessage = ""
        FBLinkController.login(emailField.text.trim(), passwordField.text)
    }

    Component.onCompleted: {
        console.log("PageTvLogin loaded")
        emailField.forceActiveFocus()
    }

    Connections {
        target: FBLinkController
        function onLoginError(message) { root.errorMessage = message }
    }

    Rectangle {
        anchors.fill: parent
        color: "#070707"
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 72
        spacing: 58

        ColumnLayout {
            Layout.preferredWidth: 430
            Layout.fillHeight: true
            spacing: 28

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 250

                Image {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    width: 210
                    height: 210
                    source: "qrc:/images/fblink_logo.png"
                    fillMode: Image.PreserveAspectFit
                }
            }

            Label {
                Layout.fillWidth: true
                text: "FBLink VPN"
                color: "#F8FAFC"
                font.pixelSize: 50
                font.bold: true
            }

            Label {
                Layout.fillWidth: true
                text: "Вход для Android TV"
                color: "#FACC15"
                font.pixelSize: 28
                font.bold: true
            }

            Label {
                Layout.fillWidth: true
                text: "Используйте email и пароль от аккаунта. Вход по коду включим отдельно, когда подтверждение появится в мобильном приложении."
                color: "#A1A1AA"
                font.pixelSize: 24
                lineHeight: 1.18
                wrapMode: Text.WordWrap
            }

            Item { Layout.fillHeight: true }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.maximumWidth: 720
            Layout.alignment: Qt.AlignVCenter
            Layout.preferredHeight: 560
            radius: 28
            color: "#121212"
            border.width: 2
            border.color: "#2F2F2F"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 42
                spacing: 22

                Label {
                    Layout.fillWidth: true
                    text: "Войти"
                    color: "#F8FAFC"
                    font.pixelSize: 38
                    font.bold: true
                }

                TextField {
                    id: emailField
                    Layout.fillWidth: true
                    Layout.preferredHeight: 72
                    placeholderText: "Email"
                    font.pixelSize: 26
                    inputMethodHints: Qt.ImhEmailCharactersOnly
                    KeyNavigation.down: passwordField
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
                    Layout.preferredHeight: 72
                    placeholderText: "Пароль"
                    echoMode: TextInput.Password
                    font.pixelSize: 26
                    KeyNavigation.up: emailField
                    KeyNavigation.down: loginButton
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
                    font.pixelSize: 22
                    wrapMode: Text.WordWrap
                }

                Button {
                    id: loginButton
                    Layout.fillWidth: true
                    Layout.preferredHeight: 78
                    text: FBLinkController.isLoading ? "Входим..." : "Войти"
                    font.pixelSize: 28
                    enabled: !FBLinkController.isLoading
                    highlighted: activeFocus
                    KeyNavigation.up: passwordField
                    KeyNavigation.down: appCodeButton
                    onClicked: root.submitLogin()
                    Keys.onPressed: function(event) {
                        if (root.isOkKey(event)) {
                            clicked()
                            event.accepted = true
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: "#2F2F2F"
                }

                Button {
                    id: appCodeButton
                    Layout.fillWidth: true
                    Layout.preferredHeight: 72
                    text: "Вход через приложение скоро"
                    font.pixelSize: 24
                    enabled: false
                    KeyNavigation.up: loginButton
                }
            }
        }
    }
}

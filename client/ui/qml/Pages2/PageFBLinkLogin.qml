import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Config"
import "../Controls2/TextTypes"
import "../Components"

PageType {
    id: root

    property string errorMessage: ""
    property bool isLoading: false

    function isSubscriptionGateMessage(message) {
        var normalized = (message || "").toLowerCase()
        return normalized.indexOf("active subscription required") !== -1
            || normalized.indexOf("subscription expired") !== -1
    }

    BackButtonType {
        id: backButton
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: 20 + SettingsController.safeAreaTopMargin
        anchors.leftMargin: 16
    }

    ColumnLayout {
        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 24
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        spacing: 16

        // Logo
        Image {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 8
            Layout.preferredWidth: 120
            Layout.preferredHeight: 120
            fillMode: Image.PreserveAspectFit
            sourceSize.width: 120
            sourceSize.height: 120
            source: "qrc:/images/fblink_logo.png"
        }

        // Header
        BaseHeaderType {
            Layout.fillWidth: true
            headerText: qsTr("Вход")
            descriptionText: qsTr("Введите данные аккаунта FBLink VPN")
        }

        // Error message
        Rectangle {
            Layout.fillWidth: true
            height: errorText.implicitHeight + 16
            color: "#3D1515"
            radius: 8
            visible: errorMessage !== ""

            LabelTextType {
                id: errorText
                anchors.centerIn: parent
                anchors.margins: 8
                width: parent.width - 16
                text: root.errorMessage
                color: "#FF6B6B"
                wrapMode: Text.WordWrap
                font.pixelSize: 13
            }
        }

        // Email field
        TextFieldWithHeaderType {
            id: emailField
            Layout.fillWidth: true
            headerText: qsTr("Email")
            textField.placeholderText: "you@example.com"
            textField.inputMethodHints: Qt.ImhEmailCharactersOnly
            textField.KeyNavigation.tab: passwordField.textField
        }

        // Password field
        TextFieldWithHeaderType {
            id: passwordField
            property bool hidePassword: true
            Layout.fillWidth: true
            headerText: qsTr("Пароль")
            textField.placeholderText: "••••••••"
            textField.echoMode: hidePassword ? TextInput.Password : TextInput.Normal
            buttonImageSource: hidePassword ? "qrc:/images/controls/eye.svg" : "qrc:/images/controls/eye-off.svg"
            clickedFunc: function() { hidePassword = !hidePassword }
        }

        // Forgot password link
        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignRight

            ButtonTextType {
                text: qsTr("Забыли пароль?")
                font.pixelSize: 13
                color: FBLinkStyle.color.mutedGray
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: PageController.goToPage(PageEnum.PageFBLinkForgotPassword)
                }
            }
        }

        Item { Layout.fillHeight: true }

        // Register link
        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter

            LabelTextType {
                text: qsTr("Нет аккаунта?")
                color: FBLinkStyle.color.mutedGray
                font.pixelSize: 14
            }
            ButtonTextType {
                text: qsTr("Зарегистрироваться")
                font.pixelSize: 14

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        PageController.goToPage(PageEnum.PageFBLinkRegister)
                    }
                }
            }
        }

        // Login button
        BasicButtonType {
            id: loginButton
            Layout.fillWidth: true
            Layout.bottomMargin: 24 + SettingsController.safeAreaBottomMargin

            defaultColor: "#00C8FF"
            hoveredColor: "#33D4FF"
            pressedColor: "#0099BB"
            disabledColor: FBLinkStyle.color.mutedGray
            textColor: FBLinkStyle.color.paleGray

            enabled: !root.isLoading
            text: root.isLoading ? qsTr("Вход...") : qsTr("Войти")

            clickedFunc: function() {
                root.errorMessage = ""
                var email = emailField.textField.text.trim()
                var password = passwordField.textField.text

                if (email === "" || password === "") {
                    root.errorMessage = qsTr("Пожалуйста, заполните все поля")
                    return
                }

                root.isLoading = true
                PageController.showBusyIndicator(true)
                FBLinkController.login(email, password)
            }
        }
    }

    // Обработка ответов от C++ контроллера
    Connections {
        target: FBLinkController

        function onLoginSuccess() {
            if (!root.visible) return
            root.errorMessage = ""
        }

        function onLoginError(message) {
            if (!root.visible) return
            root.isLoading = false
            PageController.showBusyIndicator(false)
            root.errorMessage = message
        }

        function onSubscriptionFetched() {
            if (!root.visible) return
            if (!FBLinkController.isSubscribed) {
                root.isLoading = false
                PageController.showBusyIndicator(false)
                root.errorMessage = ""
                PageController.goToPageHome()
            }
        }

        function onSubscriptionError(message) {
            if (!root.visible) return
            root.isLoading = false
            PageController.showBusyIndicator(false)
            root.errorMessage = message
        }

        function onConfigFetched() {
            if (!root.visible) return
            root.isLoading = false
            PageController.showBusyIndicator(false)
            root.errorMessage = ""
            PageController.goToPageHome()
        }

        function onConfigError(message) {
            if (!root.visible) return
            root.isLoading = false
            PageController.showBusyIndicator(false)
            if (root.isSubscriptionGateMessage(message)) {
                root.errorMessage = ""
                PageController.goToPageHome()
                return
            }
            root.errorMessage = message
        }
    }
}

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
    property bool codeSent: false
    property string pendingEmail: ""
    property int resendCooldown: 0

    Timer {
        id: resendTimer
        interval: 1000
        repeat: true
        running: root.resendCooldown > 0
        onTriggered: root.resendCooldown--
    }

    Connections {
        target: FBLinkController

        function onRegisterCodeSent() {
            root.isLoading = false
            PageController.showBusyIndicator(false)
            root.codeSent = true
            root.errorMessage = ""
            root.resendCooldown = 60
        }

        function onRegisterError(message) {
            root.isLoading = false
            PageController.showBusyIndicator(false)
            root.errorMessage = message
        }

        function onVerifySuccess() {
            root.isLoading = false
            PageController.showBusyIndicator(false)
        }

        function onVerifyError(message) {
            root.isLoading = false
            PageController.showBusyIndicator(false)
            root.errorMessage = message
        }

        function onConfigFetched() {
            root.isLoading = false
            PageController.showBusyIndicator(false)
            PageController.goToPageHome()
        }

        function onConfigError(message) {
            root.isLoading = false
            PageController.showBusyIndicator(false)
            root.errorMessage = message
        }
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

        BaseHeaderType {
            Layout.fillWidth: true
            headerText: root.codeSent ? qsTr("Подтверждение email") : qsTr("Создание аккаунта")
            descriptionText: root.codeSent
                ? qsTr("Введите код, отправленный на ") + root.pendingEmail
                : qsTr("Зарегистрируйтесь в сервисе FBLink VPN")
        }

        // Error
        Rectangle {
            Layout.fillWidth: true
            height: errorText.implicitHeight + 16
            color: "#3D1515"
            radius: 8
            visible: root.errorMessage !== ""

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

        // Step 1: Email + Password
        TextFieldWithHeaderType {
            id: emailField
            Layout.fillWidth: true
            headerText: qsTr("Email")
            textField.placeholderText: "you@example.com"
            textField.inputMethodHints: Qt.ImhEmailCharactersOnly
            visible: !root.codeSent
        }

        TextFieldWithHeaderType {
            id: passwordField
            property bool hidePassword: true
            Layout.fillWidth: true
            headerText: qsTr("Пароль")
            textField.placeholderText: "••••••••"
            textField.echoMode: hidePassword ? TextInput.Password : TextInput.Normal
            buttonImageSource: hidePassword ? "qrc:/images/controls/eye.svg" : "qrc:/images/controls/eye-off.svg"
            clickedFunc: function() { hidePassword = !hidePassword }
            visible: !root.codeSent
        }

        TextFieldWithHeaderType {
            id: confirmPasswordField
            property bool hidePassword: true
            Layout.fillWidth: true
            headerText: qsTr("Подтвердите пароль")
            textField.placeholderText: "••••••••"
            textField.echoMode: hidePassword ? TextInput.Password : TextInput.Normal
            buttonImageSource: hidePassword ? "qrc:/images/controls/eye.svg" : "qrc:/images/controls/eye-off.svg"
            clickedFunc: function() { hidePassword = !hidePassword }
            visible: !root.codeSent
        }

        // Step 2: Verification code
        TextFieldWithHeaderType {
            id: codeField
            Layout.fillWidth: true
            headerText: qsTr("Код подтверждения")
            textField.placeholderText: "000000"
            textField.inputMethodHints: Qt.ImhDigitsOnly
            textField.maximumLength: 6
            visible: root.codeSent
        }

        Item { Layout.fillHeight: true }

        // Login link
        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            visible: !root.codeSent

            LabelTextType {
                text: qsTr("Уже есть аккаунт?")
                color: FBLinkStyle.color.mutedGray
                font.pixelSize: 14
            }
            ButtonTextType {
                text: qsTr("Войти")
                font.pixelSize: 14
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: PageController.goToPage(PageEnum.PageFBLinkLogin)
                }
            }
        }

        // Resend code link
        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            visible: root.codeSent

            ButtonTextType {
                text: root.resendCooldown > 0
                    ? qsTr("Отправить повторно (%1 сек)").arg(root.resendCooldown)
                    : qsTr("Отправить код повторно")
                font.pixelSize: 14
                opacity: root.resendCooldown > 0 ? 0.5 : 1.0
                MouseArea {
                    anchors.fill: parent
                    cursorShape: root.resendCooldown > 0 ? Qt.ArrowCursor : Qt.PointingHandCursor
                    onClicked: {
                        if (root.resendCooldown > 0) return
                        root.errorMessage = ""
                        root.isLoading = true
                        PageController.showBusyIndicator(true)
                        FBLinkController.registerUser(root.pendingEmail, passwordField.textField.text)
                    }
                }
            }
        }

        BasicButtonType {
            Layout.fillWidth: true
            Layout.bottomMargin: 24 + SettingsController.safeAreaBottomMargin

            defaultColor: "#00C8FF"
            hoveredColor: "#33D4FF"
            pressedColor: "#0099BB"
            disabledColor: FBLinkStyle.color.mutedGray
            textColor: FBLinkStyle.color.paleGray

            enabled: !root.isLoading
            text: root.isLoading
                ? (root.codeSent ? qsTr("Проверка...") : qsTr("Отправка..."))
                : (root.codeSent ? qsTr("Подтвердить") : qsTr("Создать аккаунт"))

            clickedFunc: function() {
                root.errorMessage = ""

                if (root.codeSent) {
                    // Step 2: verify code
                    var code = codeField.textField.text.trim()
                    if (code.length !== 6) {
                        root.errorMessage = qsTr("Введите 6-значный код")
                        return
                    }
                    root.isLoading = true
                    PageController.showBusyIndicator(true)
                    FBLinkController.verifyEmail(root.pendingEmail, code)
                } else {
                    // Step 1: send code
                    var email = emailField.textField.text.trim()
                    var password = passwordField.textField.text
                    var confirmPassword = confirmPasswordField.textField.text

                    if (email === "" || password === "" || confirmPassword === "") {
                        root.errorMessage = qsTr("Пожалуйста, заполните все поля")
                        return
                    }
                    if (password !== confirmPassword) {
                        root.errorMessage = qsTr("Пароли не совпадают")
                        return
                    }
                    if (password.length < 8) {
                        root.errorMessage = qsTr("Пароль должен быть не менее 8 символов")
                        return
                    }

                    root.pendingEmail = email
                    root.isLoading = true
                    PageController.showBusyIndicator(true)
                    FBLinkController.registerUser(email, password)
                }
            }
        }
    }
}

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
    property string successMessage: ""
    property bool isLoading: false
    property int step: 1 // 1=email, 2=code, 3=new password
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

        function onForgotPasswordSent() {
            root.isLoading = false
            PageController.showBusyIndicator(false)
            root.errorMessage = ""
            root.step = 2
            root.resendCooldown = 60
        }

        function onForgotPasswordError(message) {
            root.isLoading = false
            PageController.showBusyIndicator(false)
            root.errorMessage = message
        }

        function onResetPasswordSuccess() {
            root.isLoading = false
            PageController.showBusyIndicator(false)
            root.errorMessage = ""
            root.successMessage = qsTr("Пароль успешно изменён! Войдите с новым паролем.")
            root.step = 0
        }

        function onResetPasswordError(message) {
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
            headerText: {
                if (root.step === 1) return qsTr("Восстановление пароля")
                if (root.step === 2) return qsTr("Введите код")
                if (root.step === 3) return qsTr("Новый пароль")
                return qsTr("Готово")
            }
            descriptionText: {
                if (root.step === 1) return qsTr("Введите email вашего аккаунта")
                if (root.step === 2) return qsTr("Код отправлен на ") + root.pendingEmail
                if (root.step === 3) return qsTr("Введите новый пароль")
                return ""
            }
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

        // Success
        Rectangle {
            Layout.fillWidth: true
            height: successText.implicitHeight + 16
            color: "#153D1F"
            radius: 8
            visible: root.successMessage !== ""

            LabelTextType {
                id: successText
                anchors.centerIn: parent
                anchors.margins: 8
                width: parent.width - 16
                text: root.successMessage
                color: "#6BFF8A"
                wrapMode: Text.WordWrap
                font.pixelSize: 13
            }
        }

        // Step 1: Email
        TextFieldWithHeaderType {
            id: emailField
            Layout.fillWidth: true
            headerText: qsTr("Email")
            textField.placeholderText: "you@example.com"
            textField.inputMethodHints: Qt.ImhEmailCharactersOnly
            visible: root.step === 1
        }

        // Step 2: Code
        TextFieldWithHeaderType {
            id: codeField
            Layout.fillWidth: true
            headerText: qsTr("Код подтверждения")
            textField.placeholderText: "000000"
            textField.inputMethodHints: Qt.ImhDigitsOnly
            textField.maximumLength: 6
            visible: root.step === 2
        }

        // Step 3: New password
        TextFieldWithHeaderType {
            id: newPasswordField
            property bool hidePassword: true
            Layout.fillWidth: true
            headerText: qsTr("Новый пароль")
            textField.placeholderText: "••••••••"
            textField.echoMode: hidePassword ? TextInput.Password : TextInput.Normal
            buttonImageSource: hidePassword ? "qrc:/images/controls/eye.svg" : "qrc:/images/controls/eye-off.svg"
            clickedFunc: function() { hidePassword = !hidePassword }
            visible: root.step === 3
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
            visible: root.step === 3
        }

        Item { Layout.fillHeight: true }

        // Resend code link
        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            visible: root.step === 2

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
                        FBLinkController.forgotPassword(root.pendingEmail)
                    }
                }
            }
        }

        // Back to login link (shown on success)
        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            visible: root.step === 0

            ButtonTextType {
                text: qsTr("Вернуться к входу")
                font.pixelSize: 14
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: PageController.goToPage(PageEnum.PageFBLinkLogin)
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
            visible: root.step > 0

            text: {
                if (root.isLoading) {
                    return qsTr("Отправка...")
                }
                if (root.step === 1) return qsTr("Отправить код")
                if (root.step === 2) return qsTr("Далее")
                return qsTr("Сменить пароль")
            }

            clickedFunc: function() {
                root.errorMessage = ""

                if (root.step === 1) {
                    var email = emailField.textField.text.trim()
                    if (email === "") {
                        root.errorMessage = qsTr("Введите email")
                        return
                    }
                    root.pendingEmail = email
                    root.isLoading = true
                    PageController.showBusyIndicator(true)
                    FBLinkController.forgotPassword(email)
                } else if (root.step === 2) {
                    var code = codeField.textField.text.trim()
                    if (code.length !== 6) {
                        root.errorMessage = qsTr("Введите 6-значный код")
                        return
                    }
                    // Move to step 3 (enter new password)
                    root.step = 3
                } else if (root.step === 3) {
                    var password = newPasswordField.textField.text
                    var confirmPassword = confirmPasswordField.textField.text

                    if (password === "" || confirmPassword === "") {
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

                    root.isLoading = true
                    PageController.showBusyIndicator(true)
                    FBLinkController.resetPassword(root.pendingEmail, codeField.textField.text.trim(), password)
                }
            }
        }
    }
}

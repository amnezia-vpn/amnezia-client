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
    property int step: 1
    property string pendingEmail: ""
    property int resendCooldown: 0
    readonly property bool wideLayout: GC.isWideWidth(width)
    readonly property real sideMargin: GC.pageHorizontalMargin(width)
    readonly property real maxContentWidth: GC.pageMaxWidth(width)

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
            root.successMessage = qsTr("Пароль успешно изменён. Теперь можно войти с новым паролем.")
            root.step = 0
        }

        function onResetPasswordError(message) {
            root.isLoading = false
            PageController.showBusyIndicator(false)
            root.errorMessage = message
        }
    }

    Flickable {
        anchors.fill: parent
        contentHeight: content.implicitHeight + 28
        clip: true

        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        Item {
            width: parent.width
            height: content.implicitHeight + 28

            ColumnLayout {
                id: content
                width: Math.min(root.maxContentWidth, parent.width - root.sideMargin * 2)
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                spacing: 18

                BackButtonType {
                    Layout.topMargin: 20 + SettingsController.safeAreaTopMargin
                    Layout.leftMargin: 4
                }

                Rectangle {
                    id: authCard
                    Layout.fillWidth: true
                    radius: 26
                    color: Qt.rgba(12/255, 12/255, 12/255, 0.98)
                    border.width: 1
                    border.color: Qt.rgba(63/255, 63/255, 70/255, 0.9)
                    clip: true
                    implicitHeight: authContent.implicitHeight + (root.wideLayout ? 68 : 44)

                    ColumnLayout {
                        id: authContent
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: root.wideLayout ? 34 : 22
                        spacing: 14

                        LabelTextType {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignHCenter
                            text: qsTr("FBLink VPN")
                            font.pixelSize: root.wideLayout ? 44 : 34
                            font.weight: 700
                            color: "#F5F5F5"
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.WordWrap
                        }

                        CaptionTextType {
                            Layout.fillWidth: true
                            text: root.step === 1 ? qsTr("Введите email для сброса пароля.")
                                 : root.step === 2 ? qsTr("Введите код, отправленный на %1.").arg(root.pendingEmail)
                                 : root.step === 3 ? qsTr("Введите и подтвердите новый пароль.")
                                 : qsTr("Пароль успешно обновлён.")
                            color: FBLinkStyle.color.mutedGray
                            font.pixelSize: 13
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.WordWrap
                        }

                        WarningType {
                            Layout.fillWidth: true
                            visible: root.errorMessage !== ""
                            textString: root.errorMessage
                            iconPath: "qrc:/images/controls/alert-circle.svg"
                            backGroundColor: Qt.rgba(239/255, 68/255, 68/255, 0.12)
                            imageColor: "#EF4444"
                            textColor: "#FFB4B4"
                        }

                        WarningType {
                            Layout.fillWidth: true
                            visible: root.successMessage !== ""
                            textString: root.successMessage
                            iconPath: "qrc:/images/controls/check.svg"
                            backGroundColor: Qt.rgba(16/255, 185/255, 129/255, 0.12)
                            imageColor: "#10B981"
                            textColor: "#B6F2D2"
                        }

                        TextFieldWithHeaderType {
                            id: emailField
                            Layout.fillWidth: true
                            headerText: qsTr("EMAIL")
                            textField.placeholderText: "user@company.com"
                            textField.inputMethodHints: Qt.ImhEmailCharactersOnly
                            visible: root.step === 1
                        }

                        TextFieldWithHeaderType {
                            id: codeField
                            Layout.fillWidth: true
                            headerText: qsTr("КОД ПОДТВЕРЖДЕНИЯ")
                            textField.placeholderText: "000000"
                            textField.inputMethodHints: Qt.ImhDigitsOnly
                            textField.maximumLength: 6
                            visible: root.step === 2
                        }

                        TextFieldWithHeaderType {
                            id: newPasswordField
                            property bool hidePassword: true
                            Layout.fillWidth: true
                            headerText: qsTr("НОВЫЙ ПАРОЛЬ")
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
                            headerText: qsTr("ПОДТВЕРДИТЕ ПАРОЛЬ")
                            textField.placeholderText: "••••••••"
                            textField.echoMode: hidePassword ? TextInput.Password : TextInput.Normal
                            buttonImageSource: hidePassword ? "qrc:/images/controls/eye.svg" : "qrc:/images/controls/eye-off.svg"
                            clickedFunc: function() { hidePassword = !hidePassword }
                            visible: root.step === 3
                        }

                        BasicButtonType {
                            Layout.fillWidth: true
                            visible: root.step > 0
                            implicitHeight: 56
                            defaultColor: "#EAB308"
                            hoveredColor: "#FACC15"
                            pressedColor: "#CA8A04"
                            disabledColor: FBLinkStyle.color.mutedGray
                            textColor: "#111111"
                            enabled: !root.isLoading
                            text: root.isLoading
                                ? qsTr("Отправка...")
                                : root.step === 1 ? qsTr("Отправить код")
                                : root.step === 2 ? qsTr("Далее")
                                : qsTr("Сменить пароль")
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
                                    root.step = 3
                                } else if (root.step === 3) {
                                    var password = newPasswordField.textField.text
                                    var confirmPassword = confirmPasswordField.textField.text
                                    var resetCode = codeField.textField.text.trim()

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
                                    FBLinkController.resetPassword(root.pendingEmail, resetCode, password)
                                }
                            }
                        }

                        LabelTextType {
                            Layout.fillWidth: true
                            visible: root.step === 2
                            text: root.resendCooldown > 0
                                ? qsTr("Отправить повторно (%1 сек)").arg(root.resendCooldown)
                                : qsTr("Отправить код повторно")
                            color: root.resendCooldown > 0 ? FBLinkStyle.color.charcoalGray : FBLinkStyle.color.mutedGray
                            font.pixelSize: 13
                            horizontalAlignment: Text.AlignHCenter

                            MouseArea {
                                anchors.fill: parent
                                enabled: root.resendCooldown <= 0
                                cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                                onClicked: {
                                    root.errorMessage = ""
                                    root.isLoading = true
                                    PageController.showBusyIndicator(true)
                                    FBLinkController.forgotPassword(root.pendingEmail)
                                }
                            }
                        }

                        LabelTextType {
                            Layout.fillWidth: true
                            visible: root.step === 0
                            text: qsTr("Вернуться к входу")
                            color: "#EAB308"
                            font.pixelSize: 14
                            horizontalAlignment: Text.AlignHCenter

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: PageController.goToPage(PageEnum.PageFBLinkLogin)
                            }
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 24 + SettingsController.safeAreaBottomMargin
                }
            }
        }
    }
}

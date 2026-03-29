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

                GridLayout {
                    Layout.fillWidth: true
                    columns: root.wideLayout ? 2 : 1
                    columnSpacing: 18
                    rowSpacing: 18

                    PremiumPanel {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        accentVisible: true
                        accentColor: "#F59E0B"
                        visible: root.wideLayout

                        PremiumBadge {
                            text: root.step === 0 ? qsTr("Готово") : qsTr("Восстановление")
                            tone: "warning"
                            iconSource: "qrc:/images/controls/mail.svg"
                        }

                        LabelTextType {
                            Layout.fillWidth: true
                            text: root.step === 0
                                ? qsTr("Возврат к аккаунту без лишних шагов")
                                : qsTr("Сброс пароля через понятный пошаговый поток")
                            font.pixelSize: 28
                            font.weight: 700
                            color: FBLinkStyle.color.paleGray
                            wrapMode: Text.WordWrap
                        }

                        LabelTextType {
                            Layout.fillWidth: true
                            text: root.step === 1
                                ? qsTr("Сначала указываете email, затем код и новый пароль. На mobile и desktop поток одинаковый и предсказуемый.")
                                : qsTr("Мы держим только текущий шаг на экране, чтобы форма не превращалась в длинную перегруженную простыню.")
                            wrapMode: Text.WordWrap
                            font.pixelSize: 14
                            color: FBLinkStyle.color.mutedGray
                        }
                    }

                    PremiumPanel {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        accentVisible: true
                        accentColor: "#00C8FF"

                        Image {
                            visible: !root.wideLayout
                            Layout.alignment: Qt.AlignHCenter
                            Layout.preferredWidth: 88
                            Layout.preferredHeight: 88
                            fillMode: Image.PreserveAspectFit
                            source: "qrc:/images/fblink_logo.png"
                        }

                        LabelTextType {
                            Layout.fillWidth: true
                            text: root.step === 1 ? qsTr("Восстановление пароля")
                                 : root.step === 2 ? qsTr("Введите код")
                                 : root.step === 3 ? qsTr("Новый пароль")
                                 : qsTr("Готово")
                            font.pixelSize: 26
                            font.weight: 700
                            color: FBLinkStyle.color.paleGray
                        }

                        LabelTextType {
                            Layout.fillWidth: true
                            text: root.step === 1 ? qsTr("Введите email вашего аккаунта.")
                                 : root.step === 2 ? qsTr("Код уже отправлен на %1.").arg(root.pendingEmail)
                                 : root.step === 3 ? qsTr("Введите и подтвердите новый пароль.")
                                 : qsTr("Пароль обновлён. Можно возвращаться к входу.")
                            wrapMode: Text.WordWrap
                            font.pixelSize: 14
                            color: FBLinkStyle.color.mutedGray
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
                            headerText: qsTr("Email")
                            textField.placeholderText: "you@example.com"
                            textField.inputMethodHints: Qt.ImhEmailCharactersOnly
                            visible: root.step === 1
                        }

                        TextFieldWithHeaderType {
                            id: codeField
                            Layout.fillWidth: true
                            headerText: qsTr("Код подтверждения")
                            textField.placeholderText: "000000"
                            textField.inputMethodHints: Qt.ImhDigitsOnly
                            textField.maximumLength: 6
                            visible: root.step === 2
                        }

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

                        RowLayout {
                            Layout.fillWidth: true
                            visible: root.step === 2
                            spacing: 6

                            Item { Layout.fillWidth: true }

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

                            Item { Layout.fillWidth: true }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            visible: root.step === 0

                            Item { Layout.fillWidth: true }

                            ButtonTextType {
                                text: qsTr("Вернуться к входу")
                                font.pixelSize: 14
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: PageController.goToPage(PageEnum.PageFBLinkLogin)
                                }
                            }

                            Item { Layout.fillWidth: true }
                        }

                        BasicButtonType {
                            Layout.fillWidth: true
                            visible: root.step > 0
                            defaultColor: "#00C8FF"
                            hoveredColor: "#33D4FF"
                            pressedColor: "#0099BB"
                            disabledColor: FBLinkStyle.color.mutedGray
                            textColor: "#FFFFFF"
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

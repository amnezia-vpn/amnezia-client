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
    readonly property bool wideLayout: GC.isWideWidth(width)
    readonly property real sideMargin: GC.pageHorizontalMargin(width)
    readonly property real maxContentWidth: GC.pageMaxWidth(width)

    function isSubscriptionGateMessage(message) {
        var normalized = (message || "").toLowerCase()
        return normalized.indexOf("active subscription required") !== -1
            || normalized.indexOf("subscription expired") !== -1
    }

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
            if (!root.visible) return
            root.isLoading = false
            PageController.showBusyIndicator(false)
            root.codeSent = true
            root.errorMessage = ""
            root.resendCooldown = 60
        }

        function onRegisterError(message) {
            if (!root.visible) return
            root.isLoading = false
            PageController.showBusyIndicator(false)
            root.errorMessage = message
        }

        function onVerifySuccess() {
            if (!root.visible) return
            root.isLoading = false
            PageController.showBusyIndicator(false)
        }

        function onVerifyError(message) {
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
                        accentColor: "#10B981"
                        visible: root.wideLayout

                        PremiumBadge {
                            text: root.codeSent ? qsTr("Шаг 2 из 2") : qsTr("Шаг 1 из 2")
                            tone: "success"
                            iconSource: "qrc:/images/controls/mail.svg"
                        }

                        LabelTextType {
                            Layout.fillWidth: true
                            text: root.codeSent
                                ? qsTr("Подтвердите email и завершите вход")
                                : qsTr("Создайте FBLink ID и сразу синхронизируйте приложение")
                            font.pixelSize: 28
                            font.weight: 700
                            color: FBLinkStyle.color.paleGray
                            wrapMode: Text.WordWrap
                        }

                        LabelTextType {
                            Layout.fillWidth: true
                            text: root.codeSent
                                ? qsTr("Код уже отправлен на %1. После подтверждения аккаунт сразу готов к использованию.").arg(root.pendingEmail)
                                : qsTr("Новый поток регистрации не перегружает формами: сначала аккаунт, потом подтверждение и синхронизация конфигов.")
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
                            text: root.codeSent ? qsTr("Подтверждение email") : qsTr("Создание аккаунта")
                            font.pixelSize: 26
                            font.weight: 700
                            color: FBLinkStyle.color.paleGray
                        }

                        LabelTextType {
                            Layout.fillWidth: true
                            text: root.codeSent
                                ? qsTr("Введите код, отправленный на %1.").arg(root.pendingEmail)
                                : qsTr("Создайте аккаунт FBLink VPN и получите доступ к premium и VIP-потоку без перегруженных экранов.")
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

                        TextFieldWithHeaderType {
                            id: codeField
                            Layout.fillWidth: true
                            headerText: qsTr("Код подтверждения")
                            textField.placeholderText: "000000"
                            textField.inputMethodHints: Qt.ImhDigitsOnly
                            textField.maximumLength: 6
                            visible: root.codeSent
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            visible: !root.codeSent
                            spacing: 6

                            Item { Layout.fillWidth: true }

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

                            Item { Layout.fillWidth: true }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            visible: root.codeSent
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
                                        FBLinkController.registerUser(root.pendingEmail, passwordField.textField.text)
                                    }
                                }
                            }

                            Item { Layout.fillWidth: true }
                        }

                        BasicButtonType {
                            Layout.fillWidth: true
                            defaultColor: "#00C8FF"
                            hoveredColor: "#33D4FF"
                            pressedColor: "#0099BB"
                            disabledColor: FBLinkStyle.color.mutedGray
                            textColor: "#FFFFFF"
                            enabled: !root.isLoading
                            text: root.isLoading
                                ? (root.codeSent ? qsTr("Проверка...") : qsTr("Отправка..."))
                                : (root.codeSent ? qsTr("Подтвердить") : qsTr("Создать аккаунт"))
                            clickedFunc: function() {
                                root.errorMessage = ""

                                if (root.codeSent) {
                                    var code = codeField.textField.text.trim()
                                    if (code.length !== 6) {
                                        root.errorMessage = qsTr("Введите 6-значный код")
                                        return
                                    }
                                    root.isLoading = true
                                    PageController.showBusyIndicator(true)
                                    FBLinkController.verifyEmail(root.pendingEmail, code)
                                } else {
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

                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 24 + SettingsController.safeAreaBottomMargin
                }
            }
        }
    }
}

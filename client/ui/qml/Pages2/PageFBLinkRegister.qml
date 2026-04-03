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
            if (FBLinkController.isLoggedIn) {
                PageController.showNotificationMessage(qsTr("Аккаунт создан, но статус подписки пока не обновился. Можно продолжить."))
                root.errorMessage = ""
                PageController.goToPageHome()
                return
            }
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
            if (FBLinkController.isLoggedIn) {
                PageController.showNotificationMessage(qsTr("Аккаунт создан, но конфиг пока не загрузился. Можно продолжить и обновить позже."))
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
                            text: root.codeSent
                                ? qsTr("Подтвердите email кодом, отправленным на %1.").arg(root.pendingEmail)
                                : qsTr("Создайте аккаунт для доступа к вашим сетям.")
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

                        TextFieldWithHeaderType {
                            id: emailField
                            Layout.fillWidth: true
                            headerText: qsTr("Email")
                            textField.placeholderText: "user@company.com"
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

                        BasicButtonType {
                            Layout.fillWidth: true
                            implicitHeight: 56
                            defaultColor: "#EAB308"
                            hoveredColor: "#FACC15"
                            pressedColor: "#CA8A04"
                            disabledColor: FBLinkStyle.color.mutedGray
                            textColor: "#111111"
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

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            LabelTextType {
                                Layout.fillWidth: true
                                visible: root.codeSent
                                text: root.resendCooldown > 0
                                    ? qsTr("Отправить повторно (%1 сек)").arg(root.resendCooldown)
                                    : qsTr("Отправить код повторно")
                                color: root.resendCooldown > 0 ? FBLinkStyle.color.charcoalGray : FBLinkStyle.color.mutedGray
                                font.pixelSize: 13
                                horizontalAlignment: Text.AlignHCenter

                                MouseArea {
                                    anchors.fill: parent
                                    enabled: root.codeSent && root.resendCooldown <= 0
                                    cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                                    onClicked: {
                                        root.errorMessage = ""
                                        root.isLoading = true
                                        PageController.showBusyIndicator(true)
                                        FBLinkController.registerUser(root.pendingEmail, passwordField.textField.text)
                                    }
                                }
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

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
    readonly property bool wideLayout: GC.isWideWidth(width)
    readonly property real sideMargin: GC.pageHorizontalMargin(width)
    readonly property real maxContentWidth: GC.pageMaxWidth(width)

    function isSubscriptionGateMessage(message) {
        var normalized = (message || "").toLowerCase()
        return normalized.indexOf("active subscription required") !== -1
            || normalized.indexOf("subscription expired") !== -1
    }

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
            if (FBLinkController.isLoggedIn) {
                PageController.showNotificationMessage(qsTr("Вход выполнен, но статус подписки пока не обновился. Откройте раздел подписки позже."))
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
            if (FBLinkController.isLoggedIn) {
                PageController.showNotificationMessage(qsTr("Вход выполнен, но конфиг пока не загрузился. Можно продолжить и обновить позже."))
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
                            font.pixelSize: root.wideLayout ? 44 : 36
                            font.weight: 700
                            color: "#F5F5F5"
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.WordWrap
                        }

                        CaptionTextType {
                            Layout.fillWidth: true
                            text: qsTr("Войдите для доступа к вашим сетям.")
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
                            textField.placeholderText: "you@example.com"
                            textField.inputMethodHints: Qt.ImhEmailCharactersOnly
                            textField.KeyNavigation.tab: passwordField.textField
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
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Item { Layout.fillWidth: true }

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

                        BasicButtonType {
                            id: loginButton
                            Layout.fillWidth: true
                            implicitHeight: 56
                            defaultColor: "#EAB308"
                            hoveredColor: "#FACC15"
                            pressedColor: "#CA8A04"
                            disabledColor: FBLinkStyle.color.mutedGray
                            textColor: "#111111"
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

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            Item { Layout.fillWidth: true }

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
                                    onClicked: PageController.goToPage(PageEnum.PageFBLinkRegister)
                                }
                            }

                            Item { Layout.fillWidth: true }
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

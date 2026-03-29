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
                        accentColor: "#00C8FF"
                        visible: root.wideLayout

                        Image {
                            Layout.alignment: Qt.AlignLeft
                            Layout.preferredWidth: 84
                            Layout.preferredHeight: 84
                            fillMode: Image.PreserveAspectFit
                            source: "qrc:/images/fblink_logo.png"
                        }

                        PremiumBadge {
                            text: qsTr("FBLink ID")
                            tone: "accent"
                            iconSource: "qrc:/images/controls/shield-tick.svg"
                        }

                        LabelTextType {
                            Layout.fillWidth: true
                            text: qsTr("Быстрый вход без лишнего шума")
                            font.pixelSize: 28
                            font.weight: 700
                            color: FBLinkStyle.color.paleGray
                            wrapMode: Text.WordWrap
                        }

                        LabelTextType {
                            Layout.fillWidth: true
                            text: qsTr("Логин, подписка и обновление конфигов теперь собраны в одном чистом потоке: вход, проверка статуса, синхронизация.")
                            wrapMode: Text.WordWrap
                            font.pixelSize: 14
                            color: FBLinkStyle.color.mutedGray
                        }

                        PremiumPanel {
                            Layout.fillWidth: true
                            fillColor: Qt.rgba(1, 1, 1, 0.03)
                            outlineColor: Qt.rgba(1, 1, 1, 0.06)
                            padding: 14

                            PremiumBadge {
                                text: qsTr("Что вы получите")
                                tone: "success"
                            }

                            LabelTextType {
                                Layout.fillWidth: true
                                text: qsTr("Премиум-карточки, актуальные конфиги и синхронизированные VIP-профили сразу после авторизации.")
                                wrapMode: Text.WordWrap
                                color: FBLinkStyle.color.lightGray
                                font.pixelSize: 13
                            }
                        }
                    }

                    PremiumPanel {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        accentVisible: true
                        accentColor: "#10B981"

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
                            text: qsTr("Вход")
                            font.pixelSize: 26
                            font.weight: 700
                            color: FBLinkStyle.color.paleGray
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
                            defaultColor: "#00C8FF"
                            hoveredColor: "#33D4FF"
                            pressedColor: "#0099BB"
                            disabledColor: FBLinkStyle.color.mutedGray
                            textColor: "#FFFFFF"
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

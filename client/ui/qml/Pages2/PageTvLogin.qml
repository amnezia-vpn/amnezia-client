import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../Components"

// FBLink VPN — TV sign-in screen.
//
// The primary flow is the QR code "device authorization" pattern that the
// backend already exposes through `/auth/tv/start` and `/auth/tv/token`:
//
//   1. The TV calls `FBLinkController.startTvLogin()` on load and shows a
//      short user code plus a QR code that points at the verification URL.
//   2. The user scans the code on a phone, signs in there, and approves
//      the TV.
//   3. The TV polls `FBLinkController.pollTvLogin()` on a timer and is
//      transitioned to `PageTvHome` by `PageTvRoot` once the controller
//      emits `tvLoginApproved`.
//
// The email/password form is kept as a fall-back ("Sign in with email")
// for users who cannot use a phone. The form uses `TvTextField`, which
// keeps the D-pad working sensibly together with the Android TV on-screen
// keyboard (arrow keys move the caret, only Back / Escape leave the field).
FocusScope {
    id: root

    width: parent ? parent.width : 1920
    height: parent ? parent.height : 1080
    focus: true
    clip: true

    property string errorMessage: ""
    property bool useEmailFallback: false

    function isOkKey(event) {
        return event.key === Qt.Key_Return
            || event.key === Qt.Key_Enter
            || event.key === Qt.Key_Select
    }

    function startQrLogin() {
        root.errorMessage = ""
        FBLinkController.startTvLogin()
    }

    function submitLogin() {
        root.errorMessage = ""
        if (emailField.text.trim() === "") {
            root.errorMessage = qsTr("Введите email")
            emailField.textField.forceActiveFocus()
            return
        }
        if (passwordField.text === "") {
            root.errorMessage = qsTr("Введите пароль")
            passwordField.textField.forceActiveFocus()
            return
        }
        FBLinkController.login(emailField.text.trim(), passwordField.text)
    }

    Component.onCompleted: {
        console.log("PageTvLogin loaded")
        root.startQrLogin()
        qrCard.forceActiveFocus()
    }

    Component.onDestruction: {
        pollTimer.stop()
        FBLinkController.cancelTvLogin()
    }

    Connections {
        target: FBLinkController
        function onLoginError(message) { root.errorMessage = message }
        function onTvLoginChanged() {
            const status = FBLinkController.tvLoginStatus
            if (status === "error") {
                root.errorMessage = FBLinkController.tvLoginError !== ""
                        ? FBLinkController.tvLoginError
                        : qsTr("Не удалось запустить вход по QR-коду")
            } else if (status === "pending") {
                root.errorMessage = ""
            }
            const interval = Math.max(3000, FBLinkController.tvLoginPollIntervalMs)
            if (pollTimer.interval !== interval) {
                pollTimer.interval = interval
            }
        }
    }

    Timer {
        id: pollTimer
        interval: Math.max(3000, FBLinkController.tvLoginPollIntervalMs)
        repeat: true
        running: !root.useEmailFallback
                 && FBLinkController.tvLoginStatus === "pending"
                 && FBLinkController.tvLoginUserCode !== ""
        onTriggered: FBLinkController.pollTvLogin()
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#08090B" }
            GradientStop { position: 1.0; color: "#0F0A02" }
        }
    }

    Item {
        id: stage
        width: 1920
        height: 1080
        scale: Math.min(1, root.width / width, root.height / height) * 0.94
        anchors.centerIn: parent

        // Brand panel — left side.
        ColumnLayout {
            id: brandPane
            x: 160
            y: 140
            width: 720
            spacing: 24

            Image {
                Layout.preferredWidth: 220
                Layout.preferredHeight: 220
                source: "qrc:/images/fblink_logo.png"
                fillMode: Image.PreserveAspectFit
            }

            Label {
                Layout.fillWidth: true
                text: "FBLink VPN"
                color: "#F8FAFC"
                font.pixelSize: 72
                font.bold: true
            }

            Label {
                Layout.fillWidth: true
                text: qsTr("Android TV")
                color: "#FACC15"
                font.pixelSize: 36
                font.bold: true
            }

            Label {
                Layout.fillWidth: true
                text: root.useEmailFallback
                        ? qsTr("Войдите по email и паролю. Кнопка OK на пульте откроет экранную клавиатуру.")
                        : qsTr("Откройте указанную ссылку на телефоне или отсканируйте QR-код, введите код и подтвердите вход.")
                color: "#A1A1AA"
                font.pixelSize: 30
                lineHeight: 1.2
                wrapMode: Text.WordWrap
            }

            Label {
                Layout.fillWidth: true
                visible: root.errorMessage !== ""
                text: root.errorMessage
                color: "#F87171"
                font.pixelSize: 26
                wrapMode: Text.WordWrap
            }
        }

        // Auth card — right side.
        Rectangle {
            id: authCard
            x: 1000
            y: 180
            width: 760
            height: 770
            radius: 32
            color: "#101013"
            border.width: 2
            border.color: "#2A2A2D"

            // ----- QR pane (default) -----
            Item {
                id: qrPane
                anchors.fill: parent
                visible: !root.useEmailFallback

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 56
                    spacing: 28

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Вход по QR-коду")
                        color: "#F8FAFC"
                        font.pixelSize: 44
                        font.bold: true
                    }

                    Label {
                        Layout.fillWidth: true
                        text: FBLinkController.tvLoginVerificationUrl !== ""
                                ? FBLinkController.tvLoginVerificationUrl
                                : qsTr("Подключение...")
                        color: "#A1A1AA"
                        font.pixelSize: 22
                        elide: Text.ElideRight
                    }

                    // QR / status box.
                    Item {
                        id: qrCard
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: 360
                        Layout.preferredHeight: 360
                        focus: true
                        activeFocusOnTab: true

                        Rectangle {
                            id: qrBg
                            anchors.fill: parent
                            radius: 24
                            color: "#FAFAFA"
                            border.width: qrCard.activeFocus ? 4 : 1
                            border.color: qrCard.activeFocus ? "#FACC15" : "#27272A"

                            Behavior on border.color { ColorAnimation { duration: 120 } }
                        }

                        Image {
                            anchors.fill: parent
                            anchors.margins: 18
                            visible: FBLinkController.tvLoginQrCodeImage !== ""
                            source: FBLinkController.tvLoginQrCodeImage
                            fillMode: Image.PreserveAspectFit
                            smooth: false
                        }

                        BusyIndicator {
                            anchors.centerIn: parent
                            running: FBLinkController.tvLoginQrCodeImage === ""
                                     && FBLinkController.tvLoginStatus !== "error"
                            visible: running
                        }

                        Keys.onPressed: function(event) {
                            if (root.isOkKey(event)) {
                                if (FBLinkController.tvLoginStatus === "error"
                                    || FBLinkController.tvLoginUserCode === "") {
                                    root.startQrLogin()
                                } else {
                                    FBLinkController.pollTvLogin()
                                }
                                event.accepted = true
                            } else if (event.key === Qt.Key_Down) {
                                refreshCodeButton.forceActiveFocus()
                                event.accepted = true
                            }
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        text: FBLinkController.tvLoginUserCode !== ""
                                ? FBLinkController.tvLoginUserCode
                                : "————"
                        color: "#FACC15"
                        font.pixelSize: 56
                        font.bold: true
                        font.letterSpacing: 4
                    }

                    Label {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        text: {
                            switch (FBLinkController.tvLoginStatus) {
                            case "starting": return qsTr("Получаем код...")
                            case "pending":  return qsTr("Ожидаем подтверждение на телефоне...")
                            case "approved": return qsTr("Вход подтверждён, открываем приложение...")
                            case "error":    return qsTr("Ошибка. Нажмите OK для повторной попытки.")
                            default:         return qsTr("Подключение...")
                            }
                        }
                        color: FBLinkController.tvLoginStatus === "error" ? "#F87171" : "#A1A1AA"
                        font.pixelSize: 24
                        wrapMode: Text.WordWrap
                    }

                    Item { Layout.fillHeight: true }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 18

                        TvButton {
                            id: refreshCodeButton
                            Layout.fillWidth: true
                            Layout.preferredHeight: 78
                            text: qsTr("Обновить код")
                            tvFontPixelSize: 26
                            KeyNavigation.up: qrCard
                            KeyNavigation.right: emailModeButton
                            onClicked: root.startQrLogin()
                            Keys.onPressed: function(event) {
                                if (root.isOkKey(event)) {
                                    clicked()
                                    event.accepted = true
                                }
                            }
                        }

                        TvButton {
                            id: emailModeButton
                            Layout.fillWidth: true
                            Layout.preferredHeight: 78
                            text: qsTr("Войти по email")
                            tvFontPixelSize: 26
                            KeyNavigation.up: qrCard
                            KeyNavigation.left: refreshCodeButton
                            onClicked: {
                                pollTimer.stop()
                                FBLinkController.cancelTvLogin()
                                root.useEmailFallback = true
                                Qt.callLater(function() {
                                    emailField.textField.forceActiveFocus()
                                })
                            }
                            Keys.onPressed: function(event) {
                                if (root.isOkKey(event)) {
                                    clicked()
                                    event.accepted = true
                                }
                            }
                        }
                    }
                }
            }

            // ----- Email / password fallback -----
            Item {
                id: emailPane
                anchors.fill: parent
                visible: root.useEmailFallback

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 56
                    spacing: 22

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Вход по email")
                        color: "#F8FAFC"
                        font.pixelSize: 44
                        font.bold: true
                    }

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Нажмите OK на поле, чтобы открыть экранную клавиатуру. Стрелки двигают курсор внутри поля.")
                        color: "#A1A1AA"
                        font.pixelSize: 22
                        wrapMode: Text.WordWrap
                    }

                    Item { Layout.preferredHeight: 6 }

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Email")
                        color: "#A1A1AA"
                        font.pixelSize: 22
                    }

                    TvTextField {
                        id: emailField
                        Layout.fillWidth: true
                        Layout.preferredHeight: 86
                        placeholderText: "you@example.com"
                        inputMethodHints: Qt.ImhEmailCharactersOnly | Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
                        KeyNavigation.up: backToQrButton
                        KeyNavigation.down: passwordField
                        onAccepted: passwordField.textField.forceActiveFocus()
                    }

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Пароль")
                        color: "#A1A1AA"
                        font.pixelSize: 22
                    }

                    TvTextField {
                        id: passwordField
                        Layout.fillWidth: true
                        Layout.preferredHeight: 86
                        placeholderText: "••••••••"
                        echoMode: TextInput.Password
                        inputMethodHints: Qt.ImhSensitiveData | Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
                        KeyNavigation.up: emailField
                        KeyNavigation.down: loginButton
                        onAccepted: loginButton.forceActiveFocus()
                    }

                    Label {
                        Layout.fillWidth: true
                        visible: root.errorMessage !== ""
                        text: root.errorMessage
                        color: "#F87171"
                        font.pixelSize: 22
                        wrapMode: Text.WordWrap
                    }

                    Item { Layout.fillHeight: true }

                    TvButton {
                        id: loginButton
                        Layout.fillWidth: true
                        Layout.preferredHeight: 90
                        text: FBLinkController.isLoading ? qsTr("Вход...") : qsTr("Войти")
                        tvFontPixelSize: 30
                        enabled: !FBLinkController.isLoading
                        KeyNavigation.up: passwordField
                        KeyNavigation.down: backToQrButton
                        onClicked: root.submitLogin()
                        Keys.onPressed: function(event) {
                            if (root.isOkKey(event)) {
                                clicked()
                                event.accepted = true
                            }
                        }
                    }

                    TvButton {
                        id: backToQrButton
                        Layout.fillWidth: true
                        Layout.preferredHeight: 70
                        text: qsTr("Вернуться к QR-коду")
                        tvFontPixelSize: 22
                        KeyNavigation.up: loginButton
                        onClicked: {
                            root.errorMessage = ""
                            root.useEmailFallback = false
                            Qt.callLater(function() {
                                root.startQrLogin()
                                qrCard.forceActiveFocus()
                            })
                        }
                        Keys.onPressed: function(event) {
                            if (root.isOkKey(event)) {
                                clicked()
                                event.accepted = true
                            }
                        }
                    }
                }
            }
        }
    }
}

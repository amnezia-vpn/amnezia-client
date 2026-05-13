import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../Components"

// FBLink VPN — Android TV sign-in screen.
//
// Primary path: QR / device-flow login.
//   - Auto-starts `FBLinkController.startTvLogin()` on entry.
//   - Renders the QR code, user code and verification URL.
//   - Polls `pollTvLogin()` on a timer until the controller emits
//     `tvLoginApproved` (which `PageTvRoot` reacts to).
//
// Fallback path: email/password using an in-app TV keyboard.
//   - Android TV remotes can not reliably drive Qt's TextInput together
//     with the system OSK (D-pad ends up navigating between fields
//     instead of between IME keys). We side-step that by rendering our
//     own keyboard via `TvOnScreenKeyboard` and binding it to one of
//     two `TvLoginRow` display widgets.
FocusScope {
    id: root

    width: parent ? parent.width : 1920
    height: parent ? parent.height : 1080
    focus: true
    clip: true

    enum InputTarget {
        None,
        Email,
        Password
    }

    property string errorMessage: ""
    property bool useEmailFallback: false
    property string emailValue: ""
    property string passwordValue: ""
    property int activeInput: PageTvLogin.InputTarget.None
    property string keyboardBuffer: ""

    function isOkKey(event) {
        return event.key === Qt.Key_Return
            || event.key === Qt.Key_Enter
            || event.key === Qt.Key_Select
    }

    function startQrLogin() {
        root.errorMessage = ""
        FBLinkController.startTvLogin()
    }

    function openKeyboard(target) {
        root.activeInput = target
        root.keyboardBuffer = target === PageTvLogin.InputTarget.Email
                ? root.emailValue
                : root.passwordValue
        Qt.callLater(function() {
            if (keyboardLoader.item && keyboardLoader.item.keyboard) {
                keyboardLoader.item.keyboard.forceActiveFocus()
            }
        })
    }

    function commitKeyboard() {
        if (root.activeInput === PageTvLogin.InputTarget.Email) {
            root.emailValue = root.keyboardBuffer
        } else if (root.activeInput === PageTvLogin.InputTarget.Password) {
            root.passwordValue = root.keyboardBuffer
        }
        const previousTarget = root.activeInput
        root.activeInput = PageTvLogin.InputTarget.None
        Qt.callLater(function() {
            if (previousTarget === PageTvLogin.InputTarget.Email) {
                passwordRow.forceActiveFocus()
            } else if (previousTarget === PageTvLogin.InputTarget.Password) {
                loginButton.forceActiveFocus()
            }
        })
    }

    function dismissKeyboard() {
        const previousTarget = root.activeInput
        root.activeInput = PageTvLogin.InputTarget.None
        Qt.callLater(function() {
            if (previousTarget === PageTvLogin.InputTarget.Email) {
                emailRow.forceActiveFocus()
            } else if (previousTarget === PageTvLogin.InputTarget.Password) {
                passwordRow.forceActiveFocus()
            }
        })
    }

    function submitLogin() {
        root.errorMessage = ""
        if (root.emailValue.trim() === "") {
            root.errorMessage = qsTr("Введите email")
            emailRow.forceActiveFocus()
            return
        }
        if (root.passwordValue === "") {
            root.errorMessage = qsTr("Введите пароль")
            passwordRow.forceActiveFocus()
            return
        }
        FBLinkController.login(root.emailValue.trim(), root.passwordValue)
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

        // Left — branding panel.
        ColumnLayout {
            x: 140
            y: 130
            width: 720
            spacing: 22

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
                font.pixelSize: 34
                font.bold: true
            }

            Label {
                Layout.fillWidth: true
                text: root.useEmailFallback
                        ? qsTr("Выделите поле и нажмите OK, чтобы вызвать встроенную клавиатуру. Перемещение по клавишам — стрелками пульта, ввод символа — кнопкой OK.")
                        : qsTr("Отсканируйте QR-код, либо откройте указанную ссылку, введите код и подтвердите вход. ТВ войдёт автоматически.")
                color: "#A1A1AA"
                font.pixelSize: 26
                lineHeight: 1.2
                wrapMode: Text.WordWrap
            }

            Label {
                Layout.fillWidth: true
                visible: root.errorMessage !== ""
                text: root.errorMessage
                color: "#F87171"
                font.pixelSize: 24
                wrapMode: Text.WordWrap
            }
        }

        // Right — auth card.
        Rectangle {
            id: authCard
            x: 1000
            y: 100
            width: 800
            height: 880
            radius: 32
            color: "#101013"
            border.width: 2
            border.color: "#2A2A2D"

            // ---- QR pane ------------------------------------------
            Item {
                id: qrPane
                anchors.fill: parent
                visible: !root.useEmailFallback

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 56
                    spacing: 22

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Вход по QR-коду")
                        color: "#F8FAFC"
                        font.pixelSize: 42
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

                    Item {
                        id: qrCard
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: 360
                        Layout.preferredHeight: 360
                        focus: true
                        activeFocusOnTab: true

                        Rectangle {
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
                        font.pixelSize: 22
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
                            tvFontPixelSize: 24
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
                            tvFontPixelSize: 24
                            KeyNavigation.up: qrCard
                            KeyNavigation.left: refreshCodeButton
                            onClicked: {
                                pollTimer.stop()
                                FBLinkController.cancelTvLogin()
                                root.useEmailFallback = true
                                Qt.callLater(function() {
                                    emailRow.forceActiveFocus()
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

            // ---- Email/password fallback ----------------------------
            Item {
                id: emailPane
                anchors.fill: parent
                visible: root.useEmailFallback

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 48
                    spacing: 18

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Вход по email")
                        color: "#F8FAFC"
                        font.pixelSize: 38
                        font.bold: true
                    }

                    TvLoginRow {
                        id: emailRow
                        Layout.fillWidth: true
                        Layout.preferredHeight: 78
                        title: qsTr("Email")
                        placeholder: "you@example.com"
                        value: root.emailValue
                        passwordMode: false
                        KeyNavigation.up: backToQrButton
                        KeyNavigation.down: passwordRow
                        onActivated: root.openKeyboard(PageTvLogin.InputTarget.Email)
                    }

                    TvLoginRow {
                        id: passwordRow
                        Layout.fillWidth: true
                        Layout.preferredHeight: 78
                        title: qsTr("Пароль")
                        placeholder: "••••••••"
                        value: root.passwordValue
                        passwordMode: true
                        KeyNavigation.up: emailRow
                        KeyNavigation.down: loginButton
                        onActivated: root.openKeyboard(PageTvLogin.InputTarget.Password)
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
                        Layout.preferredHeight: 84
                        text: FBLinkController.isLoading
                                ? qsTr("Вход...")
                                : qsTr("Войти")
                        tvFontPixelSize: 28
                        enabled: !FBLinkController.isLoading
                        KeyNavigation.up: passwordRow
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
                        Layout.preferredHeight: 64
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

        // ---- On-screen keyboard overlay ---------------------------------
        //
        // Rendered only while a field is being edited. While visible it
        // captures D-pad input and the rest of the page is dimmed. The
        // overlay sits at the bottom of the stage and is large enough to
        // be readable on a 1080p TV from couch distance.
        Rectangle {
            id: keyboardScrim
            anchors.fill: parent
            color: "#000000"
            opacity: root.activeInput !== PageTvLogin.InputTarget.None ? 0.55 : 0.0
            visible: opacity > 0
            Behavior on opacity { NumberAnimation { duration: 140 } }
            MouseArea { anchors.fill: parent; onClicked: { /* swallow */ } }
        }

        Loader {
            id: keyboardLoader
            x: (stage.width - 1080) / 2
            y: stage.height - 460
            width: 1080
            height: 420
            active: root.activeInput !== PageTvLogin.InputTarget.None

            sourceComponent: FocusScope {
                id: keyboardScope
                property alias keyboard: kb

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 10

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 56
                        radius: 14
                        color: "#0F0F11"
                        border.width: 1
                        border.color: "#27272A"

                        Label {
                            anchors.fill: parent
                            anchors.margins: 14
                            verticalAlignment: Text.AlignVCenter
                            text: {
                                const value = root.keyboardBuffer
                                if (root.activeInput === PageTvLogin.InputTarget.Password) {
                                    return value.length > 0
                                            ? "•".repeat(value.length)
                                            : qsTr("Введите пароль")
                                }
                                return value.length > 0
                                        ? value
                                        : qsTr("Введите email")
                            }
                            color: root.keyboardBuffer.length > 0 ? "#F8FAFC" : "#52525B"
                            font.pixelSize: 24
                            elide: Text.ElideRight
                        }
                    }

                    TvOnScreenKeyboard {
                        id: kb
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        value: root.keyboardBuffer
                        passwordMode: root.activeInput === PageTvLogin.InputTarget.Password
                        focus: true
                        onValueChanged: root.keyboardBuffer = value
                        onAccepted: root.commitKeyboard()
                        onDismissed: root.dismissKeyboard()
                    }
                }
            }
        }
    }
}

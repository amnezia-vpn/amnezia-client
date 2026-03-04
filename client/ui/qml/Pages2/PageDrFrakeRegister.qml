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

    property string apiBase: "http://31.135.65.188:8081"
    property string errorMessage: ""
    property bool isLoading: false

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

        BaseHeaderType {
            Layout.fillWidth: true
            headerText: qsTr("Создание аккаунта")
            descriptionText: qsTr("Зарегистрируйтесь в сервисе Dr.Frake VPN")
        }

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
            textField.KeyNavigation.tab: confirmPasswordField.textField
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
        }

        Item { Layout.fillHeight: true }

        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter

            LabelTextType {
                text: qsTr("Уже есть аккаунт?")
                color: AmneziaStyle.color.mutedGray
                font.pixelSize: 14
            }
            ButtonTextType {
                text: qsTr("Войти")
                font.pixelSize: 14

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        PageController.goToPage(PageEnum.PageDrFrakeLogin)
                    }
                }
            }
        }

        BasicButtonType {
            id: registerButton
            Layout.fillWidth: true
            Layout.bottomMargin: 24 + SettingsController.safeAreaBottomMargin

            defaultColor: "#5B4DFF"
            hoveredColor: "#7065FF"
            pressedColor: "#4338CC"
            disabledColor: AmneziaStyle.color.mutedGray
            textColor: AmneziaStyle.color.paleGray

            enabled: !root.isLoading
            text: root.isLoading ? qsTr("Создание...") : qsTr("Создать аккаунт")

            clickedFunc: function() {
                root.errorMessage = ""
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

                root.isLoading = true
                PageController.showBusyIndicator(true)

                var xhr = new XMLHttpRequest()
                xhr.open("POST", root.apiBase + "/api/v1/auth/register")
                xhr.setRequestHeader("Content-Type", "application/json")
                xhr.onreadystatechange = function() {
                    if (xhr.readyState !== XMLHttpRequest.DONE) return
                    root.isLoading = false
                    PageController.showBusyIndicator(false)

                    if (xhr.status === 201 || xhr.status === 200) {
                        var resp = JSON.parse(xhr.responseText)
                        var token = resp.access_token
                        // Auto-fetch config after registration
                        fetchConfig(token)
                    } else if (xhr.status === 0) {
                        root.errorMessage = qsTr("Ошибка подключения (сервер недоступен)")
                    } else {
                        try {
                            var err = JSON.parse(xhr.responseText)
                            root.errorMessage = err.error || qsTr("Ошибка регистрации")
                        } catch(e) {
                            root.errorMessage = qsTr("Ошибка подключения")
                        }
                    }
                }
                
                xhr.onerror = function() {
                    root.isLoading = false
                    PageController.showBusyIndicator(false)
                    root.errorMessage = qsTr("Ошибка сети (сервер недоступен)")
                }
                
                xhr.send(JSON.stringify({ email: email, password: password }))
            }
        }
    }

    function fetchConfig(token) {
        root.isLoading = true
        PageController.showBusyIndicator(true)
        var xhr = new XMLHttpRequest()
        xhr.open("GET", root.apiBase + "/api/v1/me/config")
        xhr.setRequestHeader("Authorization", "Bearer " + token)
        xhr.onreadystatechange = function() {
            if (xhr.readyState !== XMLHttpRequest.DONE) return
            
            root.isLoading = false
            PageController.showBusyIndicator(false)
            if (xhr.status === 200) {
                var resp = JSON.parse(xhr.responseText)
                var configString = resp.config // The JSON string we get from Go backend
                // Import AWG2 config
                if (ImportController.extractConfigFromData(configString)) {
                    ImportController.importConfig()
                    PageController.goToPageHome()
                } else {
                    root.errorMessage = qsTr("Удалось получить конфигурацию")
                }
            } else if (xhr.status === 0) {
                root.errorMessage = qsTr("Ошибка подключения (сервер недоступен)")
            } else {
                root.errorMessage = qsTr("Аккаунт создан! Войдите чтобы получить вашу конфигурацию.")
                PageController.goToPage(PageEnum.PageDrFrakeLogin)
            }
        }
        
        xhr.onerror = function() {
            root.isLoading = false
            PageController.showBusyIndicator(false)
            root.errorMessage = qsTr("Ошибка сети (сервер недоступен)")
        }
        
        xhr.send()
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ProtocolEnum 1.0
import ContainerProps 1.0
import ContainersModelFilters 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    defaultActiveFocusItem: usernameField.textField

    property string username
    property string password

    Connections {
        target: AuthController
    }

    Item {
        id: loginPage
        anchors.fill: parent

        ColumnLayout {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.leftMargin: 16
            anchors.rightMargin: 16

            spacing: 16

            RowLayout {
                id: topStrip
                Layout.alignment: Qt.AlignTop
                BackButtonType {
                    id: backButton
                    Layout.topMargin: 20

                    KeyNavigation.tab: usernameField.textField

                    backButtonFunction: function() {
                        PageController.closePage()
                    }
                }

                HeaderType {
                    Layout.fillWidth: true
                    Layout.topMargin: 24
                    Layout.rightMargin: 16
                    Layout.leftMargin: 16

                    headerText: qsTr("Login")
                }
            }

            ColumnLayout {
                Layout.alignment: Qt.AlignVCenter

                TextFieldWithHeaderType {
                    id: usernameField

                    Layout.fillWidth: true
                    headerText: qsTr("Username")

                    textField.inputMethodHints: Qt.ImhNoPredictiveText
                    textFieldText: root.username

                    KeyNavigation.tab: passwordField.textField
                }
                Binding { root.username: usernameField.textField.text }

                TextFieldWithHeaderType {
                    id: passwordField

                    Layout.fillWidth: true
                    headerText: qsTr("Password")

                    textField.inputMethodHints: Qt.ImhHiddenText | Qt.ImhSensitiveData | Qt.ImhNoPredictiveText
                    textFieldText: root.password
                    echoMode: TextInput.Password

                    KeyNavigation.tab: loginButton
                }
                Binding { root.password: passwordField.textField.text }

                BasicButtonType {
                    id: loginButton

                    Layout.fillWidth: true
                    Layout.topMargin: 10

                    text: qsTr("Login")

                    clickedFunc: function() {
                        PageController.showBusyIndicator(true)
                        AuthController.login(root.username, root.password)
                    }

                    KeyNavigation.tab: forgotPasswordButton
                }

                BasicButtonType {
                    id: forgotPasswordButton

                    Layout.fillWidth: true

                    defaultColor: AmneziaStyle.color.transparent
                    hoveredColor: AmneziaStyle.color.translucentWhite
                    pressedColor: AmneziaStyle.color.sheerWhite
                    disabledColor: AmneziaStyle.color.mutedGray
                    textColor: AmneziaStyle.color.mutedGray
                    leftImageColor: AmneziaStyle.color.transparent

                    text: qsTr("Forgot password?")

                    clickedFunc: function() {
                        PageController.goToPage(PageEnum.PageForgotPassword)
                    }

                    Keys.onTabPressed: lastItemTabClicked()
                }
            }

            Item {
                implicitHeight: topStrip.height
            }
        }
    }

    Connections {
        target: AuthController

        function onErrorOccurred(error) {
            PageController.showBusyIndicator(false)
        }

        function onLoginSuccessfull() {
            PageController.showBusyIndicator(false)
            PageController.goToPageHome()
        }
    }
}

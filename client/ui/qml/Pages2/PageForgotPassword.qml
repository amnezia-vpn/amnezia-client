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

    property string email

    Item {
        id: forgotPasswordPage
        anchors.fill: parent

        Popup {
            id: blockingPopup
            anchors.centerIn: Overlay.overlay
            background: Rectangle {
                radius: 16
                color: Qt.rgba(14/255, 14/255, 17/255, 1.0)
                border.color: "transparent"
            }

            enter: Transition {
                NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 100 }
            }
            exit: Transition {
                NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 100 }
            }

            modal: true
            focus: true
            closePolicy: Popup.NoAutoClose

            padding: 20
            property int margin: 32
            property int maxWidth: 380
            width: Math.min(parent.width - margin, maxWidth)

            ColumnLayout {
                id: popupContent
                width: parent.width
                spacing: 16

                Header2Type {
                    headerText: qsTr("Recovery link sent")
                }

                ParagraphTextType {
                    Layout.fillWidth: true
                    Layout.maximumWidth: parent.width

                    font.pixelSize: 14

                    text: qsTr("A recovery link has been sent to your email. Follow the instructions in the email message to recover your account.")
                    color: AmneziaStyle.color.paleGray
                }

                BasicButtonType {
                    id: goToLoginButton

                    Layout.fillWidth: true

                    text: qsTr("Go back")

                    clickedFunc: function() {
                        blockingPopup.close()
                        PageController.closePage()
                    }
                }
            }
        }

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

                    KeyNavigation.tab: emailField.textField

                    backButtonFunction: function() {
                        PageController.closePage()
                    }
                }

                HeaderType {
                    Layout.fillWidth: true
                    Layout.topMargin: 24
                    Layout.rightMargin: 16
                    Layout.leftMargin: 16

                    headerText: qsTr("Account recovery")
                }
            }

            ColumnLayout {
                Layout.alignment: Qt.AlignVCenter

                TextFieldWithHeaderType {
                    id: emailField

                    Layout.fillWidth: true
                    headerText: qsTr("Email")

                    textFieldText: root.email

                    KeyNavigation.tab: submitButton
                }
                Binding { root.email: emailField.textField.text }

                BasicButtonType {
                    id: submitButton

                    Layout.fillWidth: true
                    Layout.topMargin: 10

                    text: qsTr("Submit")

                    clickedFunc: function() {
                        PageController.showBusyIndicator(true)
                        AuthController.recoverAccount(root.email)
                    }
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

        function onRecoveryEmailSent() {
            PageController.showBusyIndicator(false)
            blockingPopup.open()
        }
    }
}

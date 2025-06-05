pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QtCore

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

DrawerType2 {
    id: root

    expandedHeight: parent.height * 0.9

    Connections {
        target: ApiPremV1MigrationController

        function onErrorOccurred(error, goToPageHome) {
            PageController.showErrorMessage(error)
            root.closeTriggered()
        }
    }

    expandedStateContent: Item {
        implicitHeight: root.expandedHeight

        ListViewType {
            id: listView

            anchors.fill: parent

            model: 1 // fake model to force the ListView to be created without a model
            snapMode: ListView.NoSnap

            header: ColumnLayout {
                width: listView.width

                Header2Type {
                    id: header
                    Layout.fillWidth: true
                    Layout.topMargin: 20
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16

                    headerText: qsTr("Switch to the new Amnezia Premium subscription")
                }
            }

            delegate: ColumnLayout {
                width: listView.width

                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 16
                anchors.rightMargin: 16

                ParagraphTextType {
                    Layout.fillWidth: true
                    Layout.topMargin: 24
                    Layout.bottomMargin: 24

                    horizontalAlignment: Text.AlignLeft
                    textFormat: Text.RichText
                    text: {
                        var str = qsTr("We'll preserve all remaining days of your current subscription and give you an extra month as a thank you. ")
                        str += qsTr("This new subscription type will be actively developed with more locations and features added regularly. Currently available:")
                        str += "<ul style='margin-left: -16px;'>"
                        str += qsTr("<li>13 locations (with more coming soon)</li>")
                        str += qsTr("<li>Easier switching between countries in the app</li>")
                        str += qsTr("<li>Personal dashboard to manage your subscription</li>")
                        str += "</ul>"
                        str += qsTr("Old keys will be deactivated after switching.")
                    }
                }

                TextFieldWithHeaderType {
                    id: emailLabel
                    Layout.fillWidth: true

                    borderColor: AmneziaStyle.color.mutedGray
                    headerTextColor: AmneziaStyle.color.paleGray

                    headerText: qsTr("Email")
                    textField.placeholderText: qsTr("mail@example.com")


                    textField.onFocusChanged: {
                        textField.text = textField.text.replace(/^\s+|\s+$/g, '')
                    }

                    Connections {
                        target: ApiPremV1MigrationController

                        function onNoSubscriptionToMigrate() {
                            emailLabel.errorText = qsTr("No old format subscriptions for a given email")
                        }
                    }
                }

                CaptionTextType {
                    Layout.fillWidth: true
                    Layout.topMargin: 16

                    color: AmneziaStyle.color.mutedGray

                    text: qsTr("Enter the email you used for your current subscription")
                }

                ApiPremV1SubListDrawer {
                    id: apiPremV1SubListDrawer
                    parent: root

                    anchors.fill: parent
                }

                OtpCodeDrawer {
                    id: otpCodeDrawer
                    parent: root

                    anchors.fill: parent
                }

                BasicButtonType {
                    id: yesButton
                    Layout.fillWidth: true
                    Layout.topMargin: 32

                    text: qsTr("Continue")

                    clickedFunc: function() {
                        PageController.showBusyIndicator(true)
                        ApiPremV1MigrationController.getSubscriptionList(emailLabel.textField.text)
                        PageController.showBusyIndicator(false)
                    }
                }

                BasicButtonType {
                    id: noButton
                    Layout.fillWidth: true

                    defaultColor: AmneziaStyle.color.transparent
                    hoveredColor: AmneziaStyle.color.translucentWhite
                    pressedColor: AmneziaStyle.color.sheerWhite
                    disabledColor: AmneziaStyle.color.mutedGray
                    textColor: AmneziaStyle.color.paleGray
                    borderWidth: 1

                    text: qsTr("Remind me later")

                    clickedFunc: function() {
                        root.closeTriggered()
                    }
                }

                BasicButtonType {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.topMargin: 32
                    Layout.bottomMargin: 32
                    implicitHeight: 32

                    defaultColor: "transparent"
                    hoveredColor: AmneziaStyle.color.translucentWhite
                    pressedColor: AmneziaStyle.color.sheerWhite
                    textColor: AmneziaStyle.color.vibrantRed

                    text: qsTr("Don't remind me again")

                    clickedFunc: function() {
                        var headerText = qsTr("No more reminders? You can always switch to the new format in the server settings")
                        var yesButtonText = qsTr("Continue")
                        var noButtonText = qsTr("Cancel")

                        var yesButtonFunction = function() {
                            ApiPremV1MigrationController.disablePremV1MigrationReminder()
                            root.closeTriggered()
                        }
                        var noButtonFunction = function() {
                        }

                        showQuestionDrawer(headerText, "", yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                    }
                }
            }
        }
    }
}

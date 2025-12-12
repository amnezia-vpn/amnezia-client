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

    property bool isChangingPassword: false

    Connections {
        target: SettingsController

        function onFileEncryptionStateChanged() {
            PageController.showBusyIndicator(true)
            PageController.closePage()
            PageController.goToPage(PageEnum.PageSettingsAppEncryption)
            PageController.showBusyIndicator(false)
        }
    }

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20

        onFocusChanged: {
            if (this.activeFocus) {
                listView.positionViewAtBeginning()
            }
        }

        backButtonFunction: function() {
            PageController.closePage()
            if (root.isChangingPassword) {
                root.isChangingPassword = false
            }
        }
    }

    ListViewType {
        id: listView

        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.left: parent.left

        header: ColumnLayout {
            width: listView.width

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 16

                headerText: qsTr("File encryption")
                descriptionText: qsTr("For encrypting backups, configuration files, subscription keys, and logs")
            }

            EncryptionIndicator {
                id: indicator

                textString: SettingsController.isFileEncryptionEnabled() ? qsTr("Password set. Encryption on") : qsTr("Password not set. Encryption off")
                iconPath: SettingsController.isFileEncryptionEnabled() ? "qrc:/images/controls/lock-locked.svg" : "qrc:/images/controls/lock-unlocked.svg"
            }


            BasicButtonType {
                id: switchEncryptionButton

                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: SettingsController.isFileEncryptionEnabled() ? qsTr("Turn off encryption") : qsTr("Turn on encryption")

                clickedFunc: function() {
                    SettingsController.isFileEncryptionEnabled() ? SettingsController.toggleFileEncryption(false)
                                                                 : SettingsController.toggleFileEncryption(true)
                }
            }

            BasicButtonType {
                id: changePasswordButton

                hoveredColor: AmneziaStyle.color.slateGray
                defaultColor: AmneziaStyle.color.midnightBlack
                textColor: AmneziaStyle.color.paleGray
                borderWidth: 1

                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Change password")

                signal changingPassword

                clickedFunc: function() {
                    passwordDrawer.openTriggered()
                }
            }

            PasswordDrawer {
                id: passwordDrawer

                parent: root

                anchors.fill: parent
                expandedHeight: root.height * 0.45

                securedFunc: function() {
                    root.isChangingPassword = true

                    PageController.showBusyIndicator(true)
                    PageController.closePage()
                    PageController.goToPage(PageEnum.PageSettingsAppPassword)
                    PageController.showBusyIndicator(false)

                    SettingsController.changingPassword()
                }
            }
        }

        spacing: 16

        footer: ColumnLayout {
            width: listView.width

            // TODO: add text
        }
    }
}
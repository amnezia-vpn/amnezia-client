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
            SettingsController.isFileEncryptionEnabled() ? PageController.goToPage(PageEnum.PageSettingsAppEncryption) : PageController.goToPage(PageEnum.PageSettingsAppPassword)
            PageController.showBusyIndicator(false)
            PageController.showNotificationMessage(SettingsController.isFileEncryptionEnabled() ? qsTr("Encryption enabled") : qsTr("Encryption disabled"))
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

                headerText: qsTr("Password & Encryption")
                descriptionText: qsTr("Password protection for backups and configuration files.\nRequired to restore or import encrypted files.")
            }

            BasicButtonType {
                Layout.leftMargin: 8
                Layout.bottomMargin: 16
                implicitHeight: 16

                defaultColor: AmneziaStyle.color.transparent
                hoveredColor: AmneziaStyle.color.translucentWhite
                pressedColor: AmneziaStyle.color.sheerWhite
                disabledColor: AmneziaStyle.color.mutedGray
                textColor: AmneziaStyle.color.goldenApricot

                text: qsTr("Learn more")

                clickedFunc: function() {
                    // TODO: add link
                }
            }

            EncryptionIndicator {
                id: indicator

                textString: qsTr("Password set. Encryption enabled")
                iconPath: "qrc:/images/controls/lock-locked.svg"
            }


            BasicButtonType {
                id: disableEncryptionButton

                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Disable encryption")

                clickedFunc: function() {
                    passwordDrawer.securedFunc = function() {
                        PageController.showBusyIndicator(true)
                        SettingsController.toggleFileEncryption(false)
                        SettingsController.setPassword("")
                        SettingsController.setHint("")
                        PageController.showBusyIndicator(false)
                    }
                    passwordDrawer.openTriggered()
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

                clickedFunc: function() {
                    passwordDrawer.securedFunc = function() {
                        root.isChangingPassword = true

                        PageController.showBusyIndicator(true)
                        PageController.closePage()
                        PageController.goToPage(PageEnum.PageSettingsAppPassword)
                        PageController.showBusyIndicator(false)

                        SettingsController.changingPassword()
                    }
                    passwordDrawer.openTriggered()
                }
            }

            PasswordDrawer {
                id: passwordDrawer

                fromOutside: false

                parent: root

                anchors.fill: parent
                expandedHeight: root.height * 0.45
            }
        }

        spacing: 16

        footer: ColumnLayout {
            width: listView.width
            CaptionTextType {
                Layout.fillWidth: true
                Layout.topMargin: 16

                horizontalAlignment: Text.AlignHCenter
                textFormat: Text.RichText

                text: qsTr("If the password is forgotten, it can be recovered. To reset the password, "
                         + "<a href=\"appSettings\" style=\"text-decoration:none; color:%1;\">settings must be reset</a>."
                         + "\nEncrypted files can only be opened with password used to encrypt them").arg(AmneziaStyle.color.goldenApricot)
                color: AmneziaStyle.color.mutedGray

                onLinkActivated: function(link) {
                    if (link === "appSettings") {
                        PageController.closePage()
                    }
                }
            }
        }
    }
}
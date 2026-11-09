import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Config"
import "../Controls2/TextTypes"

PageType {
    id: root

    function trimField(field) {
        field.textField.text = field.textField.text.replace(/^\s+|\s+$/g, '')
    }

    function isCredentialsFilled() {
        var hasEmptyField = false

        if (hostnameField.textField.text === "") {
            hostnameField.errorText = qsTr("Ip address cannot be empty")
            hasEmptyField = true
        } else if (!hostnameField.textField.acceptableInput) {
            hostnameField.errorText = qsTr("Enter the address in the format 255.255.255.255:88")
        }

        if (usernameField.textField.text === "") {
            usernameField.errorText = qsTr("Login cannot be empty")
            hasEmptyField = true
        }

        if (secretDataField.textField.text === "") {
            secretDataField.errorText = qsTr("Password/private key cannot be empty")
            hasEmptyField = true
        }

        return !hasEmptyField
    }

    FlickableType {
        id: flickable

        anchors.top: parent.top
        anchors.bottom: bottomBar.top

        contentHeight: content.implicitHeight + 16

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0

            NavigationBarType {
                Layout.fillWidth: true

                title: qsTr("Configure your server")
                description: qsTr("You'll need server credentials. Enter the server IP and password")
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                spacing: 24

                ColumnLayout {
                    Layout.fillWidth: true

                    spacing: 12

                    TextFieldType {
                        id: hostnameField

                        Layout.fillWidth: true

                        headerText: qsTr("Server IP address [:port]")
                        textField.placeholderText: qsTr("255.255.255.255:22")

                        textField.onEditingFinished: root.trimField(hostnameField)
                    }

                    TextFieldType {
                        id: usernameField

                        Layout.fillWidth: true

                        headerText: qsTr("SSH Username")
                        textField.placeholderText: "root"

                        textField.onEditingFinished: root.trimField(usernameField)
                    }

                    TextFieldType {
                        id: secretDataField

                        Layout.fillWidth: true

                        headerText: qsTr("Password or SSH private key")
                        passwordToggle: true

                        textField.onEditingFinished: root.trimField(secretDataField)
                    }

                    NotificationType {
                        Layout.fillWidth: true

                        text: qsTr("SSH key requirements: supported key types are ED25519 and RSA in PEM format. Paste the private key, including the BEGIN/END lines. If your key doesn’t work, generate a compatible one")
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true

                    spacing: 12

                    BasicCardType {
                        id: hostLink

                        Layout.fillWidth: true

                        visible: GC.isDesktop() && !IsMacOsNeBuild

                        text: qsTr("Don't have a server?")
                        subtitle: qsTr("Amnezia Hosting - VPN servers without complicated settings")

                        icon.source: "qrc:/images/controls/amnezia.svg"

                        onClicked: {
                            Qt.openUrlExternally(LanguageUiController.getCurrentHostUrl())
                        }
                    }

                    BasicCardType {
                        id: siteLink

                        Layout.fillWidth: true

                        text: qsTr("How to run your VPN server")
                        subtitle: qsTr("Where to get connection data, step-by-step instructions for buying a VPS")

                        icon.source: "qrc:/images/controls/help-circle.svg"

                        onClicked: {
                            Qt.openUrlExternally(LanguageUiController.getCurrentSiteUrl("starter-guide"))
                        }
                    }
                }
            }
        }
    }

    BottomBarType {
        id: bottomBar

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        filled: true

        ButtonType {
            id: continueButton

            Layout.fillWidth: true

            text: qsTr("Continue")

            onClicked: {
                if (!root.isCredentialsFilled()) {
                    return
                }

                InstallController.setProcessedServerCredentials(hostnameField.textField.text,
                                                                usernameField.textField.text,
                                                                secretDataField.textField.text)
                ServersUiController.setProcessedServerId("")

                PageController.showBusyIndicator(true)
                var isConnectionOpened = InstallController.checkSshConnection()
                PageController.showBusyIndicator(false)
                if (!isConnectionOpened) {
                    return
                }

                PageController.goToPage(PageEnum.PageSetupWizardEasy)
            }
        }

        CaptionTextType {
            Layout.fillWidth: true
            Layout.topMargin: 16
            Layout.bottomMargin: 12

            horizontalAlignment: Text.AlignHCenter

            text: qsTr("All data you enter will remain strictly confidential and will not be shared or disclosed to the Amnezia or any third parties")
        }
    }
}

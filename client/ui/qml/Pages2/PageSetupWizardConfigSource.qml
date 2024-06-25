import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import PageEnum 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

PageType {
    id: root

    Connections {
        target: ImportController

        function onQrDecodingFinished() {
            PageController.closePage()
            PageController.goToPage(PageEnum.PageSetupWizardViewConfig)
        }
    }

    defaultActiveFocusItem: focusItem

    FlickableType {
        id: fl
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        contentHeight: content.height

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0

            Item {
                id: focusItem
                KeyNavigation.tab: backButton
            }

            BackButtonType {
                id: backButton
                Layout.topMargin: 20
                KeyNavigation.tab: fileButton.rightButton
            }

            HeaderType {
                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                headerText: qsTr("Connection")
            }

            TextFieldWithHeaderType {
                id: textKey

                Layout.fillWidth: true
                Layout.topMargin: 32
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                headerText: qsTr("Key")
                textFieldPlaceholderText: "vpn://"
                buttonText: qsTr("Insert")

                clickedFunc: function() {
                    textField.text = ""
                    textField.paste()
                }

                KeyNavigation.tab: continueButton
            }

            BasicButtonType {
                id: continueButton

                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                text: qsTr("Continue")
                Keys.onTabPressed: lastItemTabClicked(focusItem)

                clickedFunc: function() {
                    if (ImportController.extractConfigFromData(textKey.textFieldText)) {
                        PageController.goToPage(PageEnum.PageSetupWizardViewConfig)
                    }
                }
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.topMargin: 40
                Layout.rightMargin: 16
                Layout.leftMargin: 16
                Layout.bottomMargin: 24

                color: "#494B50"
                text: qsTr("Other connection options")
            }

            CardWithIconsType {
                id: apiInstalling

                Layout.fillWidth: true
                Layout.rightMargin: 16
                Layout.leftMargin: 16
                Layout.bottomMargin: 16

                headerText: qsTr("Get a VPN from Amnezia")
                bodyText: qsTr("Free VPN to bypass blocking and censorship in China, Russia, Iran and more.")

                rightImageSource: "qrc:/images/controls/chevron-right.svg"

                onClicked: {
                    PageController.showBusyIndicator(true)
                    var result = InstallController.fillAvailableServices()
                    PageController.showBusyIndicator(false)
                    if (result) {
                        PageController.goToPage(PageEnum.PageSetupWizardApiServicesList)
                    }
                }
            }

            CardWithIconsType {
                id: manualInstalling

                Layout.fillWidth: true
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                headerText: qsTr("Create a VPN on your server")
                bodyText: qsTr("Configure Amnezia VPN on your own server")

                rightImageSource: "qrc:/images/controls/chevron-right.svg"

                onClicked: {
                    PageController.goToPage(PageEnum.PageSetupWizardCredentials)
                }
            }

//            LabelWithButtonType {
//                id: fileButton
//                Layout.fillWidth: true
//                Layout.topMargin: 16

//                descriptionText: "What do you have?"

//                text: !ServersModel.getServersCount() ? qsTr("File with connection settings or backup") : qsTr("File with connection settings")
//                rightImageSource: "qrc:/images/controls/chevron-right.svg"
//                leftImageSource: "qrc:/images/controls/folder-open.svg"

//                KeyNavigation.tab: qrButton.visible ? qrButton.rightButton : textButton.rightButton

//                clickedFunction: function() {
//                    var nameFilter = !ServersModel.getServersCount() ? "Config or backup files (*.vpn *.ovpn *.conf *.json *.backup)" :
//                                                                       "Config files (*.vpn *.ovpn *.conf *.json)"
//                    var fileName = SystemController.getFileName(qsTr("Open config file"), nameFilter)
//                    if (fileName !== "") {
//                        if (ImportController.extractConfigFromFile(fileName)) {
//                            PageController.goToPage(PageEnum.PageSetupWizardViewConfig)
//                        }
//                    }
//                }
//            }

//            LabelWithButtonType {
//                id: qrButton
//                Layout.fillWidth: true
//                visible: SettingsController.isCameraPresent()

//                text: qsTr("QR-code")
//                rightImageSource: "qrc:/images/controls/chevron-right.svg"
//                leftImageSource: "qrc:/images/controls/qr-code.svg"

//                KeyNavigation.tab: textButton.rightButton

//                clickedFunction: function() {
//                    ImportController.startDecodingQr()
//                    if (Qt.platform.os === "ios") {
//                        PageController.goToPage(PageEnum.PageSetupWizardQrReader)
//                    }
//                }
//            }
        }
    }
}

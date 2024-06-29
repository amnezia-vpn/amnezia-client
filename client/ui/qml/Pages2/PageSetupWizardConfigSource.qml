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
                KeyNavigation.tab: textKey.textArea
            }

            HeaderType {
                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                headerText: qsTr("Connection")
            }

            TextAreaWithFooterType {
                id: textKey

                Layout.fillWidth: true
                Layout.topMargin: 32
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Connection key")
                placeholderText: qsTr("Starts with vpn://")

                firstButtonImage: SettingsController.isCameraPresent() ? "qrc:/images/controls/scan-line.svg" : ""
                secondButtonImage: "qrc:/images/controls/folder-search-2.svg"

                firstButtonClickedFunc: function() {
                    ImportController.startDecodingQr()
                    if (Qt.platform.os === "ios") {
                        PageController.goToPage(PageEnum.PageSetupWizardQrReader)
                    }
                }

                secondButtonClickedFunc: function() {
                    var nameFilter = !ServersModel.getServersCount() ? "Config or backup files (*.vpn *.ovpn *.conf *.json *.backup)" :
                                                                       "Config files (*.vpn *.ovpn *.conf *.json)"
                    var fileName = SystemController.getFileName(qsTr("Open config file"), nameFilter)
                    if (fileName !== "") {
                        if (ImportController.extractConfigFromFile(fileName)) {
                            PageController.goToPage(PageEnum.PageSetupWizardViewConfig)
                        }
                    }
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
                    if (ImportController.extractConfigFromData(textKey.textAreaText)) {
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
                Layout.bottomMargin: 16

                headerText: qsTr("Create a VPN on your server")
                bodyText: qsTr("Configure Amnezia VPN on your own server")

                rightImageSource: "qrc:/images/controls/chevron-right.svg"

                onClicked: {
                    PageController.goToPage(PageEnum.PageSetupWizardCredentials)
                }
            }
        }
    }
}

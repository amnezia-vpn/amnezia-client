import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import PageEnum 1.0
import Style 1.0

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
                KeyNavigation.tab: textKey.textArea
            }


            HeaderType {
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                headerText: qsTr("Connection")
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.topMargin: 32
                Layout.rightMargin: 16
                Layout.leftMargin: 16
                Layout.bottomMargin: 24

                text: qsTr("Insert the key, add a configuration file or scan the QR-code")
            }

            TextAreaWithFooterType {
                id: textKey

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Insert key")

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

                color: AmneziaStyle.color.charcoalGray
                text: qsTr("Other connection options")
            }

            CardWithIconsType {
                id: apiInstalling

                Layout.fillWidth: true
                Layout.rightMargin: 16
                Layout.leftMargin: 16
                Layout.bottomMargin: 16

                headerText: qsTr("VPN from Amnezia")
                bodyText: qsTr("Connect to classic paid and free VPN services from Amnezia")

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

                headerText: qsTr("Self-hosted VPN")
                bodyText: qsTr("Configure Amnezia VPN on your own server")

                rightImageSource: "qrc:/images/controls/chevron-right.svg"

                onClicked: {
                    PageController.goToPage(PageEnum.PageSetupWizardCredentials)
                }
            }

            CardWithIconsType {
                id: backupRestore

                Layout.fillWidth: true
                Layout.rightMargin: 16
                Layout.leftMargin: 16
                Layout.bottomMargin: 16

                headerText: qsTr("Restore from backup")

                rightImageSource: "qrc:/images/controls/chevron-right.svg"

                onClicked: {
                    var filePath = SystemController.getFileName(qsTr("Open backup file"),
                                                                qsTr("Backup files (*.backup)"))
                    if (filePath !== "") {
                        PageController.showBusyIndicator(true)
                        SettingsController.restoreAppConfig(filePath)
                        PageController.showBusyIndicator(false)
                    }
                }
            }
        }
    }
}

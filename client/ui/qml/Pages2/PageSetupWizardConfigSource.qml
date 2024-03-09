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

    defaultActiveFocusItem:fileButton.rightButton

    Connections {
        target: ImportController

        function onQrDecodingFinished() {
            PageController.closePage()
            PageController.goToPage(PageEnum.PageSetupWizardViewConfig)
        }
    }

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

            BackButtonType {
                Layout.topMargin: 20
            }

            HeaderType {
                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                headerText: qsTr("Server connection")
                descriptionText: qsTr("Do not use connection code from public sources. It may have been created to intercept your data.\n
It's okay as long as it's from someone you trust.")
            }

            Header2TextType {
                Layout.fillWidth: true
                Layout.topMargin: 48
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                text: qsTr("What do you have?")
            }

            LabelWithButtonType {
                id: fileButton
                Layout.fillWidth: true
                Layout.topMargin: 16

                text: !ServersModel.getServersCount() ? qsTr("File with connection settings or backup") : qsTr("File with connection settings")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"
                leftImageSource: "qrc:/images/controls/folder-open.svg"

                clickedFunction: function() {
                    var nameFilter = !ServersModel.getServersCount() ? "Config or backup files (*.vpn *.ovpn *.conf *.backup)" :
                                                                       "Config files (*.vpn *.ovpn *.conf)"
                    var fileName = SystemController.getFileName(qsTr("Open config file"), nameFilter)
                    if (fileName !== "") {
                        if (fileName.indexOf(".backup") !== -1 && !ServersModel.getServersCount()) {
                            PageController.showBusyIndicator(true)
                            SettingsController.restoreAppConfig(fileName)
                            PageController.showBusyIndicator(false)
                        } else {
                            ImportController.extractConfigFromFile(fileName)
                            PageController.goToPage(PageEnum.PageSetupWizardViewConfig)
                        }
                    }
                }

                KeyNavigation.tab: qrButton.visible ? qrButton.rightButton : textButton.rightButton
            }

            DividerType {}

            LabelWithButtonType {
                id: qrButton
                Layout.fillWidth: true
                visible: SettingsController.isCameraPresent()

                text: qsTr("QR-code")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"
                leftImageSource: "qrc:/images/controls/qr-code.svg"

                clickedFunction: function() {
                    ImportController.startDecodingQr()
                    if (Qt.platform.os === "ios") {
                        PageController.goToPage(PageEnum.PageSetupWizardQrReader)
                    }
                }

                KeyNavigation.tab: textButton.rightButton
            }

            DividerType {
                visible: SettingsController.isCameraPresent()
            }

            LabelWithButtonType {
                id: textButton
                Layout.fillWidth: true

                text: qsTr("Key as text")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"
                leftImageSource: "qrc:/images/controls/text-cursor.svg"

                clickedFunction: function() {
                    PageController.goToPage(PageEnum.PageSetupWizardTextKey)
                }

                KeyNavigation.tab: defaultActiveFocusItem
            }

            DividerType {}
        }
    }
}

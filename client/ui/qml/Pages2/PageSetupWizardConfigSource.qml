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
                id: backButton
                Layout.topMargin: 20
            }

            HeaderType {
                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                headerText: qsTr("Server connection")
                descriptionText: qsTr("Do not use connection codes from untrusted sources, as they may be created to intercept your data.")
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
                    var nameFilter = !ServersModel.getServersCount() ? "Config or backup files (*.vpn *.ovpn *.conf *.json *.backup)" :
                                                                       "Config files (*.vpn *.ovpn *.conf *.json)"
                    var fileName = SystemController.getFileName(qsTr("Open config file"), nameFilter)
                    if (fileName !== "") {
                        if (ImportController.extractConfigFromFile(fileName)) {
                            PageController.goToPage(PageEnum.PageSetupWizardViewConfig)
                        }
                    }
                }
            }

            DividerType {}

            LabelWithButtonType {
                id: qrButton
                Layout.fillWidth: true
                visible: SettingsController.isCameraPresent()

                text: qsTr("QR code")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"
                leftImageSource: "qrc:/images/controls/qr-code.svg"

                clickedFunction: function() {
                    ImportController.startDecodingQr()
                    if (Qt.platform.os === "ios") {
                        PageController.goToPage(PageEnum.PageSetupWizardQrReader)
                    }
                }
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
            }

            DividerType {}
        }
    }
}

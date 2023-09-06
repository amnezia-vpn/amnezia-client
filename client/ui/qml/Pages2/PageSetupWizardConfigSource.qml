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
                Layout.fillWidth: true
                Layout.topMargin: 16

                text: qsTr("File with connection settings or backup")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"
                leftImageSource: "qrc:/images/controls/folder-open.svg"

                clickedFunction: function() {
                    fileDialog.open()
                }

                FileDialog {
                    id: fileDialog
                    acceptLabel: qsTr("Open config file")
                    nameFilters: [ "Config or backup files (*.vpn *.ovpn *.conf *.backup)" ]
                    onAccepted: {
                        if (fileDialog.selectedFile.toString().indexOf(".backup") != -1) {
                            PageController.showBusyIndicator(true)
                            SettingsController.restoreAppConfig(fileDialog.selectedFile.toString())
                            PageController.showBusyIndicator(false)
                        } else {
                            ImportController.extractConfigFromFile(fileDialog.selectedFile.toString())
                            PageController.goToPage(PageEnum.PageSetupWizardViewConfig)
                        }
                    }
                }
            }

            DividerType {}

            LabelWithButtonType {
                Layout.fillWidth: true
                visible: GC.isMobile()

                text: qsTr("QR-code")
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
                visible: GC.isMobile()
            }

            LabelWithButtonType {
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

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

                headerText: "Подключение к серверу"
                descriptionText: "Не используйте код подключения из публичных источников. Его могли создать, чтобы перехватывать ваши данные.\n
Всё в порядке, если код передал друг."
            }

            Header2TextType {
                Layout.fillWidth: true
                Layout.topMargin: 48
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                text: "Что у вас есть?"
            }

            LabelWithButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 16

                text: "Файл с настройками подключения"
                rightImageSource: "qrc:/images/controls/chevron-right.svg"
                leftImageSource: "qrc:/images/controls/folder-open.svg"

                clickedFunction: function() {
//                    onClicked: fileDialog.open()
                    ImportController.extractConfigFromFile()
                    goToPage(PageEnum.PageSetupWizardViewConfig)
                }

//                FileDialog {
//                    id: fileDialog
//                    onAccepted: {
//                        ImportController.extractConfigFromFile(selectedFile)
//                        goToPage(PageEnum.PageSetupWizardViewConfig)
//                    }
//                }
            }

            DividerType {}

            //todo ifdef mobile platforms
            LabelWithButtonType {
                Layout.fillWidth: true

                text: "QR-код"
                rightImageSource: "qrc:/images/controls/chevron-right.svg"
                leftImageSource: "qrc:/images/controls/qr-code.svg"

                clickedFunction: function() {
                    ImportController.extractConfigFromQr()
//                    goToPage(PageEnum.PageSetupWizardQrReader)
                }
            }

            DividerType {}

            LabelWithButtonType {
                Layout.fillWidth: true

                text: "Ключ в виде текста"
                rightImageSource: "qrc:/images/controls/chevron-right.svg"
                leftImageSource: "qrc:/images/controls/text-cursor.svg"

                clickedFunction: function() {
                    goToPage(PageEnum.PageSetupWizardTextKey)
                }
            }

            DividerType {}
        }
    }
}

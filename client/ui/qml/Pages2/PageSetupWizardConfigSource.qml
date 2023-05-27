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

        function onImportFinished() {

        }
    }

    FlickableType {
        id: fl
        anchors.top: root.top
        anchors.bottom: root.bottom
        contentHeight: content.height

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            HeaderType {
                Layout.fillWidth: true
                Layout.topMargin: 20
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                backButtonImage: "qrc:/images/controls/arrow-left.svg"

                headerText: "Подключение к серверу"
                descriptionText: "Не используйте код подключения из публичных источников. Его могли создать, чтобы перехватывать ваши данные.\n
Всё в порядке, если код передал друг."
            }

            Header2TextType {
                Layout.fillWidth: true
                Layout.topMargin: 32
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                text: "Что у вас есть?"
            }

            LabelWithButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 16

                text: "Файл с настройками подключения"
                buttonImage: "qrc:/images/controls/chevron-right.svg"
                iconImage: "qrc:/images/controls/folder-open.svg"

                clickedFunction: function() {
                    onClicked: fileDialog.open()
                }

                FileDialog {
                    id: fileDialog
                    onAccepted: {
                        ImportController.importFromFile(selectedFile)
                    }
                }
            }

            DividerType {}

            //todo ifdef mobile platforms
            LabelWithButtonType {
                Layout.fillWidth: true

                text: "QR-код"
                buttonImage: "qrc:/images/controls/chevron-right.svg"
                iconImage: "qrc:/images/controls/qr-code.svg"

                clickedFunction: function() {
                }
            }

            DividerType {}

            LabelWithButtonType {
                Layout.fillWidth: true

                text: "Ключ в виде текста"
                buttonImage: "qrc:/images/controls/chevron-right.svg"
                iconImage: "qrc:/images/controls/text-cursor.svg"

                clickedFunction: function() {
                    goToPage(PageEnum.PageSetupWizardTextKey)
                }
            }

            DividerType {}
        }
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import PageEnum 1.0

import "./"
import "../Pages"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

PageBase {
    id: root
    page: PageEnum.PageSetupWizardInstalling

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
            anchors.rightMargin: 16
            anchors.leftMargin: 16

            spacing: 16

            HeaderType {
                Layout.fillWidth: true
                Layout.topMargin: 20

                buttonImage: "qrc:/images/controls/arrow-left.svg"

                headerText: "Подключение к серверу"
                descriptionText: "Не используйте код подключения из публичных источников. Его могли создать, чтобы перехватывать ваши данные.\n
Всё в порядке, если код передал друг."
            }

            Header2TextType {
                Layout.fillWidth: true
                Layout.topMargin: 32

                text: "Что у вас есть?"
            }

            LabelWithButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 16

                text: "Файл с настройками подключения"
                buttonImage: "qrc:/images/controls/chevron-right.svg"
                iconImage: "qrc:/images/controls/folder-open.svg"

                onClickedFunc: function() {
                    onClicked: fileDialog.open()
                }

                FileDialog {
                    id: fileDialog
//                    currentFolder: StandardPaths.standardLocations(StandardPaths.PicturesLocation)[0]
                    onAccepted: {

                    }
                }
            }
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#2C2D30"
            }

            //todo ifdef mobile platforms>
            LabelWithButtonType {
                Layout.fillWidth: true

                text: "QR-код"
                buttonImage: "qrc:/images/controls/chevron-right.svg"
                iconImage: "qrc:/images/controls/qr-code.svg"

                onClickedFunc: function() {
                }
            }
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#2C2D30"
            }

            LabelWithButtonType {
                Layout.fillWidth: true

                text: "Ключ в виде текста"
                buttonImage: "qrc:/images/controls/chevron-right.svg"
                iconImage: "qrc:/images/controls/text-cursor.svg"

                onClickedFunc: function() {
                    UiLogic.goToPage(PageEnum.PageSetupWizardTextKey)
                }
            }
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#2C2D30"
            }
        }
    }
}

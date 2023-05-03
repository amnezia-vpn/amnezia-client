import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0

import "./"
import "../Pages"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

PageBase {
    id: root
    page: PageEnum.PageSetupWizardProtocolSettings

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

                headerText: "Установка " + ContainersModel.getCurrentlyInstalledContainerName()
                descriptionText: "Эти настройки можно будет изменить позже"
            }

            BodyTextType {
                Layout.topMargin: 16

                text: "Network protocol"
            }

            //TODO move to separete control
            Rectangle {
                implicitWidth: buttonGroup.implicitWidth
                implicitHeight: buttonGroup.implicitHeight

                color: "#1C1D21"
                radius: 16

                RowLayout {
                    id: buttonGroup

                    spacing: 0

                    HorizontalRadioButton {
                        implicitWidth: (root.width - 32) / 2
                        text: "UDP"
                    }

                    HorizontalRadioButton {
                        implicitWidth: (root.width - 32) / 2
                        text: "TCP"
                    }
                }
            }

            TextFieldWithHeaderType {
                Layout.fillWidth: true
                headerText: "Port"
            }
        }
    }

    BasicButtonType {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.rightMargin: 16
        anchors.leftMargin: 16
        anchors.bottomMargin: 32

        text: qsTr("Установить")

        onClicked: function() {
            UiLogic.goToPage(PageEnum.PageSetupWizardInstalling)
        }
    }
}

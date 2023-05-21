import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ProtocolEnum 1.0

import "../Controls2"
import "../Controls2/TextTypes"


ListView {
    id: menuContent

    property var rootWidth

    width: rootWidth
    height: menuContent.contentItem.height

    clip: true

    ButtonGroup {
        id: containersRadioButtonGroup
    }

    delegate: Item {
        implicitWidth: rootWidth
        implicitHeight: containerRadioButton.implicitHeight

        RadioButton {
            id: containerRadioButton

            implicitWidth: parent.width
            implicitHeight: containerRadioButtonContent.implicitHeight

            hoverEnabled: true

            ButtonGroup.group: containersRadioButtonGroup

            checked: isDefault

            indicator: Rectangle {
                anchors.fill: parent
                color: containerRadioButton.hovered ? "#2C2D30" : "#1C1D21"

                Behavior on color {
                    PropertyAnimation { duration: 200 }
                }
            }

            checkable: isInstalled

            RowLayout {
                id: containerRadioButtonContent
                anchors.fill: parent

                anchors.rightMargin: 16
                anchors.leftMargin: 16

                z: 1

                Image {
                    source: isInstalled ? "qrc:/images/controls/check.svg" : "qrc:/images/controls/download.svg"
                    visible: isInstalled ? containerRadioButton.checked : true

                    width: 24
                    height: 24

                    Layout.rightMargin: 8
                }

                Text {
                    id: containerRadioButtonText

                    text: name
                    color: "#D7D8DB"
                    font.pixelSize: 16
                    font.weight: 400
                    font.family: "PT Root UI VF"

                    height: 24

                    Layout.fillWidth: true
                    Layout.topMargin: 20
                    Layout.bottomMargin: 20
                }
            }

            onClicked: {
                if (checked) {
                    isDefault = true
                    menuContent.currentIndex = index
                    containersDropDown.menuVisible = false
                } else {
                    ContainersModel.setCurrentlyInstalledContainerIndex(proxyContainersModel.mapToSource(index))
                    InstallController.setShouldCreateServer(false)
                    PageController.goToPage(PageEnum.PageSetupWizardProtocolSettings)
                    containersDropDown.menuVisible = false
                    menu.visible = false
                }
            }

            MouseArea {
                anchors.fill: containerRadioButton
                cursorShape: Qt.PointingHandCursor
                enabled: false
            }
        }

        Component.onCompleted: {
            if (isDefault) {
                root.currentContainerName = name
            }
        }
    }
}


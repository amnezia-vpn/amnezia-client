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
    property var selectedText

    width: rootWidth
    height: contentItem.height

    clip: true
    snapMode: ListView.SnapToItem

    ScrollBar.vertical: ScrollBarType {}

    property bool isFocusable: true

    ButtonGroup {
        id: containersRadioButtonGroup
    }

    delegate: Item {
        implicitWidth: rootWidth
        implicitHeight: content.implicitHeight

        ColumnLayout {
            id: content

            anchors.fill: parent
            anchors.rightMargin: 16
            anchors.leftMargin: 16

            VerticalRadioButton {
                id: containerRadioButton

                Layout.fillWidth: true

                text: name
                descriptionText: description

                ButtonGroup.group: containersRadioButtonGroup

                imageSource: "qrc:/images/controls/download.svg"
                showImage: !isInstalled

                checkable: isInstalled && !ConnectionController.isConnected
                checked: proxyDefaultServerContainersModel.mapToSource(index) === ServersModel.getDefaultServerData("defaultContainer")

                onClicked: {
                    if (ConnectionController.isConnected && isInstalled) {
                        PageController.showNotificationMessage(qsTr("Unable change protocol while there is an active connection"))
                        return
                    }

                    if (checked) {
                        containersDropDown.closeTriggered()
                        ServersModel.setDefaultContainer(ServersModel.defaultIndex, proxyDefaultServerContainersModel.mapToSource(index))
                    } else {
                        ContainersModel.setProcessedContainerIndex(proxyDefaultServerContainersModel.mapToSource(index))
                        InstallController.setShouldCreateServer(false)
                        PageController.goToPage(PageEnum.PageSetupWizardProtocolSettings)
                        containersDropDown.closeTriggered()
                    }
                }

                MouseArea {
                    anchors.fill: containerRadioButton
                    cursorShape: Qt.PointingHandCursor
                    enabled: false
                }

                Keys.onEnterPressed: {
                    if (checkable) {
                        checked = true
                    }
                    containerRadioButton.clicked()
                }
                Keys.onReturnPressed: {
                    if (checkable) {
                        checked = true
                    }
                    containerRadioButton.clicked()
                }
            }

            DividerType {
                Layout.fillWidth: true
            }
        }
    }
}

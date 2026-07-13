import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0

import "../Controls2"
import "../Controls2/TextTypes"

ListViewType {
    id: menuContent

    property var rootWidth
    property var selectedText

    width: rootWidth
    anchors.top: parent.top
    anchors.bottom: parent.bottom

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
                checked: proxyDefaultServerContainersModel.mapToSource(index) === ServersUiController.serverDefaultContainer(ServersUiController.defaultServerId)

                onClicked: {
                    if (ConnectionController.isConnected && isInstalled) {
                        PageController.showNotificationMessage(qsTr("Unable change protocol while there is an active connection"))
                        return
                    }

                    var containerIndex = proxyDefaultServerContainersModel.mapToSource(index)

                    if (!isInstalled) {
                        ServersUiController.processedContainerIndex = containerIndex
                        PageController.goToPage(PageEnum.PageSetupWizardProtocolSettings)
                        containersDropDown.closeTriggered()
                        return
                    }

                    containersDropDown.closeTriggered()
                    ServersUiController.setDefaultContainer(ServersUiController.defaultServerId, containerIndex)
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

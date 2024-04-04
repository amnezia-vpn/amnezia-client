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

    property bool a: true

    width: rootWidth
    height: menuContent.contentItem.height

    clip: true
    interactive: false

    property FlickableType parentFlickable
    activeFocusOnTab: true
    focus: true
    Keys.onTabPressed: {
        if (currentIndex < menuContent.count - 1) {
            menuContent.incrementCurrentIndex()
        } else {
            menuContent.currentIndex = 0
        }
    }

    onVisibleChanged: {
        if (visible) {
            menuContent.currentIndex = 0
        }
    }

    onCurrentIndexChanged: {
        if (parentFlickable) {
            parentFlickable.ensureVisible(menuContent.currentItem)
        }
    }

    ButtonGroup {
        id: containersRadioButtonGroup
    }

    delegate: Item {
        implicitWidth: rootWidth
        implicitHeight: content.implicitHeight

        onActiveFocusChanged: {
            if (activeFocus) {
                containerRadioButton.forceActiveFocus()
            }
        }

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

                checkable: isInstalled && !ConnectionController.isConnected && isSupported
                checked: proxyDefaultServerContainersModel.mapToSource(index) === ServersModel.getDefaultServerData("defaultContainer")

                onClicked: {
                    if (ConnectionController.isConnected && isInstalled) {
                        PageController.showNotificationMessage(qsTr("Unable change protocol while there is an active connection"))
                        return
                    }

                    if (checked) {
                        containersDropDown.close()
                        ServersModel.setDefaultContainer(ServersModel.defaultIndex, proxyDefaultServerContainersModel.mapToSource(index))
                    } else {
                        if (!isSupported && isInstalled) {
                            PageController.showErrorMessage(qsTr("The selected protocol is not supported on the current platform"))
                            return
                        }

                        ContainersModel.setProcessedContainerIndex(proxyDefaultServerContainersModel.mapToSource(index))
                        InstallController.setShouldCreateServer(false)
                        PageController.goToPage(PageEnum.PageSetupWizardProtocolSettings)
                        containersDropDown.close()
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

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ProtocolEnum 1.0
import ContainerProps 1.0
import ContainersModelFilters 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

ListView {
    id: root

    anchors.top: serversMenuHeader.bottom
    anchors.right: parent.right
    anchors.left: parent.left
    anchors.bottom: parent.bottom
    anchors.topMargin: 16

    model: ServersModel
    currentIndex: ServersModel.defaultIndex

    ScrollBar.vertical: ScrollBar {
        id: scrollBar
        objectName: "scrollBar"
        policy: root.height >= root.contentHeight ? ScrollBar.AlwaysOff : ScrollBar.AlwaysOn
    }

    property bool isFocusable: true

    Connections {
        target: ServersModel
        function onDefaultServerIndexChanged(serverIndex) {
            root.currentIndex = serverIndex
        }
    }

    clip: true

    delegate: Item {
        id: menuContentDelegate
        objectName: "menuContentDelegate"

        property variant delegateData: model
        property VerticalRadioButton serverRadioButtonProperty: serverRadioButton

        implicitWidth: root.width
        implicitHeight: serverRadioButtonContent.implicitHeight

        ColumnLayout {
            id: serverRadioButtonContent
            objectName: "serverRadioButtonContent"

            anchors.fill: parent
            anchors.rightMargin: 16
            anchors.leftMargin: 16

            spacing: 0

            RowLayout {
                objectName: "serverRadioButtonRowLayout"

                Layout.fillWidth: true
                VerticalRadioButton {
                    id: serverRadioButton
                    objectName: "serverRadioButton"

                    Layout.fillWidth: true
                    focus: true
                    text: name
                    descriptionText: serverDescription

                    checked: index === root.currentIndex
                    checkable: !ConnectionController.isConnected

                    ButtonGroup.group: serversRadioButtonGroup

                    onClicked: {
                        if (ConnectionController.isConnected) {
                            PageController.showNotificationMessage(qsTr("Unable change server while there is an active connection"))
                            return
                        }

                        root.currentIndex = index

                        ServersModel.defaultIndex = index
                    }

                    MouseArea {
                        anchors.fill: serverRadioButton
                        cursorShape: Qt.PointingHandCursor
                        enabled: false
                    }

                    Keys.onEnterPressed: serverRadioButton.clicked()
                    Keys.onReturnPressed: serverRadioButton.clicked()
                }

                ImageButtonType {
                    id: serverInfoButton
                    objectName: "serverInfoButton"

                    image: "qrc:/images/controls/settings.svg"
                    imageColor: AmneziaStyle.color.paleGray

                    implicitWidth: 56
                    implicitHeight: 56

                    z: 1

                    Keys.onEnterPressed: serverInfoButton.clicked()
                    Keys.onReturnPressed: serverInfoButton.clicked()

                    onClicked: function() {
                        ServersModel.processedIndex = index
                        PageController.goToPage(PageEnum.PageSettingsServerInfo)
                        drawer.closeTriggered()
                    }
                }
            }

            DividerType {
                Layout.fillWidth: true
                Layout.leftMargin: 0
                Layout.rightMargin: 0
            }
        }
    }
}

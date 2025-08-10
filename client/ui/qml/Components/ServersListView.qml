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

ListViewType {
    id: root

    property int selectedIndex: ServersModel.defaultIndex

    anchors.top: serversMenuHeader.bottom
    anchors.right: parent.right
    anchors.left: parent.left
    anchors.bottom: parent.bottom
    anchors.topMargin: 16

    model: ServersModel

    Connections {
        target: ServersModel
        function onDefaultServerIndexChanged(serverIndex) {
            root.selectedIndex = serverIndex
        }
    }

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

                    text: name
                    descriptionText: serverDescription

                    checked: index === root.selectedIndex
                    checkable: !ConnectionController.isConnected

                    ButtonGroup.group: serversRadioButtonGroup

                    onClicked: {
                        if (ConnectionController.isConnected) {
                            PageController.showNotificationMessage(qsTr("Unable change server while there is an active connection"))
                            return
                        }

                        root.selectedIndex = index

                        ServersModel.defaultIndex = index
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

                    onClicked: function() {
                        ServersModel.processedIndex = index

                        if (ServersModel.getProcessedServerData("isServerFromGatewayApi")) {
                            if (ServersModel.getProcessedServerData("isCountrySelectionAvailable")) {
                                PageController.goToPage(PageEnum.PageSettingsApiAvailableCountries)
                            } else {
                                PageController.showBusyIndicator(true)
                                let result = ApiSettingsController.getAccountInfo(false)
                                PageController.showBusyIndicator(false)
                                if (!result) {
                                    return
                                }

                                PageController.goToPage(PageEnum.PageSettingsApiServerInfo)
                            }
                        } else {
                            PageController.goToPage(PageEnum.PageSettingsServerInfo)
                        }

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

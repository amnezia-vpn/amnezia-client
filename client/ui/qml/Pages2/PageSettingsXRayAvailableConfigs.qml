import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import SortFilterProxyModel 0.2

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    property var processedServer

    Connections {
        target: PageController

        function onGoToPageSettingsServerServices() {
            tabBar.setCurrentIndex(root.pageSettingsServerServices)
        }
    }

    Connections {
        target: ServersUiController

        function onProcessedServerIdChanged() {
            root.processedServer = proxyServersModel.get(0)
        }
    }

    Connections {
        target: ServersModel

        function onModelReset() {
            root.processedServer = proxyServersModel.get(0)
        }
    }

    SortFilterProxyModel {
        id: proxyServersModel
        objectName: "proxyServersModel"

        sourceModel: ServersModel
        filters: [
            ValueFilter {
                roleName: "serverId"
                value: ServersUiController.processedServerId
            }
        ]

        Component.onCompleted: {
            root.processedServer = proxyServersModel.get(0)
        }
    }

    ListViewType {
        id: menuContent

        anchors.fill: parent

        model: xrayConfigs

        currentIndex: 0

        ButtonGroup {
            id: containersRadioButtonGroup
        }

        header: ColumnLayout {
            width: menuContent.width

            spacing: 4

            BackButtonType {
                id: backButton
                objectName: "backButton"

                Layout.topMargin: 20 + PageController.safeAreaTopMargin
            }

            HeaderTypeWithButton {
                id: headerContent
                objectName: "headerContent"

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 10

                actionButtonImage: "qrc:/images/controls/settings.svg"

                headerText: root.processedServer.name

                actionButtonFunction: function() {
                    PageController.goToPage(PageEnum.PageSettingsXRayServerInfo)
                }
            }
        }

        delegate: ColumnLayout {
            id: content

            width: menuContent.width
            height: content.implicitHeight

            RowLayout {
                VerticalRadioButton {
                    id: containerRadioButton

                    Layout.fillWidth: true
                    Layout.leftMargin: 16

                    text: model.title

                    ButtonGroup.group: containersRadioButtonGroup

                    imageSource: "qrc:/images/controls/download.svg"

                    checked: index === ServersUiController.getCurrentConfigIndex()
                    checkable: !ConnectionController.isConnected

                    onClicked: {
                        if (ConnectionController.isConnectionInProgress) {
                            PageController.showNotificationMessage(qsTr("Unable change config while trying to make an active connection"))
                            return
                        }
                        if (ConnectionController.isConnected) {
                            PageController.showNotificationMessage(qsTr("Unable change config while there is an active connection"))
                            return
                        }

                        if (index !== ServersUiController.getCurrentConfigIndex()) {
                            PageController.showBusyIndicator(true)
                            ServersUiController.setCurrentConfigIndex(index)
                            ImportController.editServerConfigWithData(ServersUiController.getProcessedServerId(), ServersUiController.getConfigString(index))
                            PageController.showBusyIndicator(false)
                        }
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
            }

            DividerType {
                Layout.fillWidth: true
            }
        }
    }

    ListModel {
        id: xrayConfigs
    }

    Component.onCompleted: {
        xrayConfigs.clear()
        const names = ServersUiController.getConfigNames()

        for (let i = 0; i < names.length; ++i) {
            xrayConfigs.append({ title: names[i] })
        }
    }
}
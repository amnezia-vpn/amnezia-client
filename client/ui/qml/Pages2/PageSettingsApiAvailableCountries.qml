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
        target: ServersModel

        function onProcessedServerChanged() {
            root.processedServer = proxyServersModel.get(0)
        }
    }

    SortFilterProxyModel {
        id: proxyServersModel
        objectName: "proxyServersModel"

        sourceModel: ServersModel
        filters: [
            ValueFilter {
                roleName: "isCurrentlyProcessed"
                value: true
            }
        ]

        Component.onCompleted: {
            root.processedServer = proxyServersModel.get(0)
        }
    }

    ListViewType {
        id: menuContent

        anchors.fill: parent

        model: ApiCountryModel

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

                Layout.topMargin: 20
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
                descriptionText: qsTr("Location for connection")

                actionButtonFunction: function() {
                    PageController.showBusyIndicator(true)
                    let result = ApiSettingsController.getAccountInfo(false)
                    PageController.showBusyIndicator(false)
                    if (!result) {
                        return
                    }

                    PageController.goToPage(PageEnum.PageSettingsApiServerInfo)
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

                    text: countryName

                    ButtonGroup.group: containersRadioButtonGroup

                    imageSource: "qrc:/images/controls/download.svg"

                    checked: index === ApiCountryModel.currentIndex
                    checkable: !ConnectionController.isConnected

                    onClicked: {
                        if (ConnectionController.isConnectionInProgress) {
                            PageController.showNotificationMessage(qsTr("Unable change server location while trying to make an active connection"))
                            return
                        }
                        if (ConnectionController.isConnected) {
                            PageController.showNotificationMessage(qsTr("Unable change server location while there is an active connection"))
                            return
                        }

                        if (index !== ApiCountryModel.currentIndex) {
                            PageController.showBusyIndicator(true)
                            var prevIndex = ApiCountryModel.currentIndex
                            ApiCountryModel.currentIndex = index
                            if (!ApiConfigsController.updateServiceFromGateway(ServersModel.defaultIndex, countryCode, countryName)) {
                                ApiCountryModel.currentIndex = prevIndex
                            }
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

                Image {
                    Layout.rightMargin: 32
                    Layout.alignment: Qt.AlignRight

                    source: "qrc:/countriesFlags/images/flagKit/" + countryImageCode + ".svg"
                }
            }

            DividerType {
                Layout.fillWidth: true
            }
        }
    }
}

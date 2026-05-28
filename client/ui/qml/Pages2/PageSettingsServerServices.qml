import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ContainerProps 1.0
import ContainersModelFilters 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    property var installedServicesCount

    SettingsContainersListView {
        id: settingsContainersListView

        anchors.fill: parent

        Connections {
            target: ServersUiController

            function onProcessedServerIdChanged() {
                settingsContainersListView.updateContainersModelFilters()
            }
        }

        function updateContainersModelFilters() {
            if (ServersUiController.isProcessedServerHasWriteAccess()) {
                proxyContainersModel.filters = ContainersModelFilters.getWriteAccessServicesListFilters()
            } else {
                proxyContainersModel.filters = ContainersModelFilters.getReadAccessServicesListFilters()
            }
            root.installedServicesCount = proxyContainersModel.count
        }

        model: SortFilterProxyModel {
            id: proxyContainersModel
            sourceModel: ContainersModel
            sorters: [
                RoleSorter { roleName: "isInstalled"; sortOrder: Qt.DescendingOrder }
            ]
        }

        footer: ColumnLayout {
            width: settingsContainersListView.width
            visible: ServersUiController.isProcessedServerHasWriteAccess()
            height: visible ? implicitHeight : 0

            LabelWithButtonType {
                Layout.fillWidth: true

                text: qsTr("Server routing rules")
                descriptionText: qsTr("Configure domains and IPs that this server adds to client split tunneling")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"

                clickedFunction: function() {
                    PageController.goToPage(PageEnum.PageSettingsServerManagedSplitTunneling)
                }
            }

            DividerType {}
        }

        Component.onCompleted: {
            settingsContainersListView.isFocusable = true
            settingsContainersListView.interactive = true
            updateContainersModelFilters()
        }
    }
}

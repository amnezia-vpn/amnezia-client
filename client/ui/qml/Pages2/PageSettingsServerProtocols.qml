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

    readonly property int installedProtocolsCount: proxyContainersModel.count

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
                proxyContainersModel.filters = ContainersModelFilters.getWriteAccessProtocolsListFilters()
            } else {
                proxyContainersModel.filters = ContainersModelFilters.getReadAccessProtocolsListFilters()
            }
        }

        model: SortFilterProxyModel {
            id: proxyContainersModel
            sourceModel: ContainersModel
            sorters: [
                RoleSorter { roleName: "isInstalled"; sortOrder: Qt.DescendingOrder },
                RoleSorter { roleName: "installPageOrder"; sortOrder: Qt.AscendingOrder }
            ]
        }

        Component.onCompleted: updateContainersModelFilters()
    }
}

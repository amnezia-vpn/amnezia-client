import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Config"

PageType {
    id: root

    SortFilterProxyModel {
        id: proxyContainersModel
        sourceModel: ContainersModel
        filters: [
            ValueFilter {
                roleName: "isVpnContainer"
                value: true
            },
            ValueFilter {
                roleName: "isSupported"
                value: true
            },
            ValueFilter {
                roleName: "isInstallationAllowed"
                value: true
            },
            ValueFilter {
                roleName: "isUnsupportedContainer"
                value: false
            }
        ]
        sorters: RoleSorter {
            roleName: "installPageOrder"
            sortOrder: Qt.AscendingOrder
        }
    }

    ListViewType {
        id: listView

        anchors.fill: parent

        header: NavigationBarType {
            width: listView.width

            title: qsTr("VPN protocol")
            description: qsTr("Choose the one with the highest priority for you. Later, you can install other protocols and additional services, such as DNS proxy and SFTP.")
        }

        model: proxyContainersModel

        spacing: 0
        snapMode: ListView.SnapToItem

        delegate: ColumnLayout {
            width: listView.width

            ListItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: name
                subtitle: description
                maximumLineCount: 6
                trailingIconSource: "qrc:/images/controls/chevron-right.svg"

                onClicked: {
                    ServersUiController.processedContainerIndex = proxyContainersModel.mapToSource(index)
                    PageController.goToPage(PageEnum.PageSetupWizardProtocolSettings)
                }
            }
        }

        footer: Item {
            width: listView.width
            height: 16 + PageController.safeAreaBottomMargin
        }
    }
}

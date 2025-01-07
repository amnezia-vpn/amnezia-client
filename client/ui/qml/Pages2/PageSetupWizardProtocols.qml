import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ProtocolEnum 1.0
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
                roleName: "serviceType"
                value: ProtocolEnum.Vpn
            },
            ValueFilter {
                roleName: "isSupported"
                value: true
            }
        ]
        sorters: RoleSorter {
            roleName: "installPageOrder"
            sortOrder: Qt.AscendingOrder
        }
    }

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        anchors.topMargin: 20
    }

    ListView {
        id: listView
        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.left: parent.left

        property bool isFocusable: true

        ScrollBar.vertical: ScrollBarType {}

        header: ColumnLayout {
            width: listView.width

            HeaderType {
                id: header

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 16

                headerText: qsTr("VPN protocol")
                descriptionText: qsTr("Choose the one with the highest priority for you. Later, you can install other protocols and additional services, such as DNS proxy and SFTP.")
            }
        }

        model: proxyContainersModel
        clip: true
        spacing: 0
        reuseItems: true
        snapMode: ListView.SnapToItem

        delegate: ColumnLayout {
            width: listView.width

            LabelWithButtonType {
                Layout.fillWidth: true

                text: name
                descriptionText: description
                rightImageSource: "qrc:/images/controls/chevron-right.svg"

                clickedFunction: function() {
                    ContainersModel.setProcessedContainerIndex(proxyContainersModel.mapToSource(index))
                    PageController.goToPage(PageEnum.PageSetupWizardProtocolSettings)
                }
            }

            DividerType {}
        }
    }
}

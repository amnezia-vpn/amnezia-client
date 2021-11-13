import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.1
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.12
import SortFilterProxyModel 0.2
import ContainerProps 1.0
import ProtocolProps 1.0
import PageEnum 1.0
import ProtocolEnum 1.0
import "./"
import "../Controls"
import "../Config"

PageBase {
    id: root
    page: PageEnum.ShareConnection
    logic: ShareConnectionLogic

    BackButton {
        id: back
    }

    Caption {
        id: caption
        text: qsTr("Share protocol config")
        width: undefined
    }


    Flickable {
        clip: true
        width: parent.width
        anchors.top: caption.bottom
        anchors.bottom: root.bottom
        contentHeight: col.height

        Column {
            id: col
            anchors {
                left: parent.left;
                right: parent.right;
            }
            topPadding: 20
            spacing: 10

            SortFilterProxyModel {
                id: proxyProtocolsModel
                sourceModel: UiLogic.protocolsModel
                filters: ValueFilter {
                    roleName: "is_installed_role"
                    value: true
                }
            }


            ShareConnectionContent {
                x: 10
                text: qsTr("Share for Amnezia")
                height: 40
                width: tb_c.width - 10
                onClicked: UiLogic.onGotoShareProtocolPage(ProtocolEnum.Any)
            }

            ListView {
                id: tb_c
                x: 10
                width: parent.width - 10
                height: tb_c.contentItem.height
                currentIndex: -1
                spacing: 10
                clip: true
                interactive: false
                model: proxyProtocolsModel

                delegate: Item {
                    implicitWidth: tb_c.width - 10
                    implicitHeight: c_item.height

                    ShareConnectionContent {
                        id: c_item
                        text: qsTr("Share for ") + name_role
                        height: 40
                        width: tb_c.width - 10
                        onClicked: UiLogic.onGotoShareProtocolPage(proxyProtocolsModel.mapToSource(index))

                    }
                }
            }
        }
    }
}

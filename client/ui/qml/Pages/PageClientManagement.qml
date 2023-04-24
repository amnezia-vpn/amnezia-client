import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes 1.4
import SortFilterProxyModel 0.2
import PageEnum 1.0
import "./"
import "../Controls"
import "../Config"

PageBase {
    id: root
    page: PageEnum.ClientManagement
    logic: ClientManagementLogic
    enabled: !ClientManagementLogic.busyIndicatorIsRunning

    BackButton {
        id: back
    }

    Caption {
        id: caption
        text: qsTr("Clients Management")
    }

    BusyIndicator {
        z: 99
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        visible: ClientManagementLogic.busyIndicatorIsRunning
        running: ClientManagementLogic.busyIndicatorIsRunning
    }

    FlickableType {
        id: fl
        anchors.top: caption.bottom
        contentHeight: content.height

        Column {
            id: content
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            LabelType {
                font.pixelSize: 20
                leftPadding: -20
                anchors.horizontalCenter: parent.horizontalCenter
                horizontalAlignment: Text.AlignHCenter
                text: ClientManagementLogic.labelCurrentVpnProtocolText
            }

            SortFilterProxyModel {
                id: proxyClientManagementModel
                sourceModel: UiLogic.clientManagementModel
                sorters: RoleSorter { roleName: "clientName" }
            }

            ListView {
                id: lv_clients
                width: parent.width
                implicitHeight: contentHeight + 20
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.rightMargin: 20
                topMargin: 10
                spacing: 10
                clip: true
                model: proxyClientManagementModel
                highlightRangeMode: ListView.ApplyRange
                highlightMoveVelocity: -1
                delegate: Item {
                    implicitWidth: lv_clients.width
                    implicitHeight: 60

                    MouseArea {
                        id: ms
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            ClientManagementLogic.onClientItemClicked(proxyClientManagementModel.mapToSource(index))
                        }
                    }

                    Rectangle {
                        anchors.fill: parent
                        gradient: ms.containsMouse ? gradient_containsMouse : gradient_notContainsMouse
                        LinearGradient {
                            id: gradient_notContainsMouse
                            x1: 0 ; y1:0
                            x2: 0 ; y2: height
                            stops: [
                                GradientStop { position: 0.0; color: "#FAFBFE" },
                                GradientStop { position: 1.0; color: "#ECEEFF" }
                            ]
                        }
                        LinearGradient {
                            id: gradient_containsMouse
                            x1: 0 ; y1:0
                            x2: 0 ; y2: height
                            stops: [
                                GradientStop { position: 0.0; color: "#FAFBFE" },
                                GradientStop { position: 1.0; color: "#DCDEDF" }
                            ]
                        }
                    }

                    LabelType {
                        x: 20
                        y: 20
                        font.pixelSize: 20
                        text: clientName
                    }
                }
            }
        }
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

DrawerType2 {
    id: root

    Connections {
        target: ApiPremV1MigrationController

        function onSubscriptionsModelChanged() {
            if (ApiPremV1MigrationController.subscriptionsModel.length > 1) {
                root.openTriggered()
            } else {
                sendMigrationCode(0)
            }
        }
    }

    function sendMigrationCode(index) {
        PageController.showBusyIndicator(true)
        ApiPremV1MigrationController.sendMigrationCode(index)
        root.closeTriggered()
        PageController.showBusyIndicator(false)
    }

    expandedHeight: parent.height * 0.9

    expandedStateContent: Item {
        implicitHeight: root.expandedHeight

        ListViewType {
            id: listView

            anchors.fill: parent

            model: ApiPremV1MigrationController.subscriptionsModel

            header: ColumnLayout {
                width: listView.width

                Header2Type {
                    id: header
                    Layout.fillWidth: true
                    Layout.topMargin: 20
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16

                    headerText: qsTr("Choose Subscription")
                }
            }

            delegate: Item {
                implicitWidth: listView.width
                implicitHeight: delegateContent.implicitHeight

                ColumnLayout {
                    id: delegateContent

                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right

                    LabelWithButtonType {
                        id: server
                        Layout.fillWidth: true

                        text: qsTr("Order ID: ") + modelData.id

                        descriptionText: qsTr("Purchase Date: ") +  Qt.formatDateTime(new Date(modelData.created_at), "dd.MM.yyyy hh:mm")
                        rightImageSource: "qrc:/images/controls/chevron-right.svg"

                        clickedFunction: function() {
                            sendMigrationCode(index)
                        }
                    }

                    DividerType {}
                }
            }
        }
    }
}

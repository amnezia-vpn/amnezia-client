import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ProtocolEnum 1.0
import ContainerEnum 1.0
import ContainerProps 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    ColumnLayout {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        anchors.topMargin: 20

        BackButtonType {
        }

        HeaderType {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            headerText: ContainersModel.getCurrentlyProcessedContainerName() + qsTr(" settings")
        }
    }

    FlickableType {
        id: fl
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        contentHeight: content.height

        Column {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 32

            ListView {
                id: protocols
                width: parent.width
                height: protocols.contentItem.height
                clip: true
                interactive: false
                model: ProtocolsModel

                delegate: Item {
                    implicitWidth: protocols.width
                    implicitHeight: delegateContent.implicitHeight

                    ColumnLayout {
                        id: delegateContent

                        anchors.fill: parent

                        LabelWithButtonType {
                            id: button

                            Layout.fillWidth: true

                            text: protocolName
                            rightImageSource: "qrc:/images/controls/chevron-right.svg"

                            clickedFunction: function() {
                                switch (protocolIndex) {
                                case ProtocolEnum.OpenVpn: OpenVpnConfigModel.updateModel(ProtocolsModel.getConfig()); break;
                                case ProtocolEnum.ShadowSocks: ShadowSocksConfigModel.updateModel(ProtocolsModel.getConfig()); break;
                                case ProtocolEnum.Cloak: CloakConfigModel.updateModel(ProtocolsModel.getConfig()); break;
                                case ProtocolEnum.WireGuard: WireGuardConfigModel.updateModel(ProtocolsModel.getConfig()); break;
                                case ProtocolEnum.Ipsec: Ikev2ConfigModel.updateModel(ProtocolsModel.getConfig()); break;
                                }
                                PageController.goToPage(protocolPage);
                            }

                            MouseArea {
                                anchors.fill: button
                                cursorShape: Qt.PointingHandCursor
                                enabled: false
                            }
                        }

                        DividerType {}
                    }
                }
            }

            LabelWithButtonType {
                id: removeButton

                width: parent.width

                visible: ServersModel.isProcessedServerHasWriteAccess()

                text: qsTr("Remove ") + ContainersModel.getCurrentlyProcessedContainerName()
                textColor: "#EB5757"

                clickedFunction: function() {
                    var headerText = qsTr("Remove %1 from server?").arg(ContainersModel.getCurrentlyProcessedContainerName())
                    var descriptionText = qsTr("All users with whom you shared a connection will no longer be able to connect to it.")
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        PageController.goToPage(PageEnum.PageDeinstalling)
                        InstallController.removeCurrentlyProcessedContainer()
                    }
                    var noButtonFunction = function() {
                    }

                    showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }

                MouseArea {
                    anchors.fill: removeButton
                    cursorShape: Qt.PointingHandCursor
                    enabled: false
                }
            }

            DividerType {}
        }
    }

    QuestionDrawer {
        id: questionDrawer
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ProtocolEnum 1.0

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
    }

    ColumnLayout {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        anchors.topMargin: 20

        BackButtonType {
        }
    }

    FlickableType {
        id: fl
        anchors.top: backButton.top
        anchors.bottom: parent.bottom
        contentHeight: content.implicitHeight + content.anchors.topMargin + content.anchors.bottomMargin

        Column {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottomMargin: 20

            Item {
                width: parent.width
                height: header.implicitHeight

                HeaderType {
                    id: header

                    anchors.fill: parent

                    anchors.leftMargin: 16
                    anchors.rightMargin: 16

                    width: parent.width

                    headerText: "Протокол подключения"
                    descriptionText: "Выберите более приоритетный для вас. Позже можно будет установить остальные протоколы и доп сервисы, вроде DNS-прокси и SFTP."
                }
            }

            ListView {
                id: containers
                width: parent.width
                height: containers.contentItem.height
                currentIndex: -1
                clip: true
                interactive: false
                model: proxyContainersModel

                delegate: Item {
                    implicitWidth: containers.width
                    implicitHeight: delegateContent.implicitHeight

                    ColumnLayout {
                        id: delegateContent

                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right

                        LabelWithButtonType {
                            id: container
                            Layout.fillWidth: true

                            text: name
                            descriptionText: description
                            rightImageSource: "qrc:/images/controls/chevron-right.svg"

                            clickedFunction: function() {
                                ContainersModel.setCurrentlyProcessedContainerIndex(proxyContainersModel.mapToSource(index))
                                goToPage(PageEnum.PageSetupWizardProtocolSettings)
                            }
                        }

                        DividerType {}
                    }
                }
            }
        }
    }
}

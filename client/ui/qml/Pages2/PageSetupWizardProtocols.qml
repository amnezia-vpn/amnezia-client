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

    FlickableType {
        id: fl
        anchors.top: root.top
        anchors.bottom: root.bottom
        contentHeight: content.height

        Column {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.rightMargin: 16
            anchors.leftMargin: 16
            anchors.topMargin: 20

            spacing: 16

            BackButtonType {
                width: parent.width
            }

            HeaderType {
                width: parent.width

                headerText: "Протокол подключения"
                descriptionText: "Выберите более приоритетный для вас. Позже можно будет установить остальные протоколы и доп сервисы, вроде DNS-прокси и SFTP."
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
                        anchors.rightMargin: -16
                        anchors.leftMargin: -16

                        LabelWithButtonType {
                            id: container
                            Layout.fillWidth: true

                            text: name
                            descriptionText: description
                            buttonImage: "qrc:/images/controls/chevron-right.svg"

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

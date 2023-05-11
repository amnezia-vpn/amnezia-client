import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ProtocolEnum 1.0

import "./"
import "../Pages"
import "../Controls2"
import "../Config"

PageBase {
    id: root
    page: PageEnum.PageSetupWizardProtocols

    SortFilterProxyModel {
        id: proxyContainersModel
        sourceModel: ContainersModel
        filters: [
            ValueFilter {
                roleName: "is_installed_role"
                value: false
            },
            ValueFilter {
                roleName: "service_type_role"
                value: ProtocolEnum.Vpn
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

            HeaderType {
                width: parent.width
                backButtonImage: "qrc:/images/controls/arrow-left.svg"

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

                        LabelWithButtonType {
                            id: container
                            Layout.fillWidth: true
                            Layout.topMargin: 16
                            Layout.bottomMargin: 16

                            text: name_role
                            descriptionText: desc_role
                            buttonImage: "qrc:/images/controls/chevron-right.svg"

                            onClickedFunc: function() {
                                ContainersModel.setCurrentlyInstalledContainerIndex(proxyContainersModel.mapToSource(index))
                                UiLogic.goToPage(PageEnum.PageSetupWizardProtocolSettings)
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: "#2C2D30"
                        }
                    }
                }
            }
        }
    }
}

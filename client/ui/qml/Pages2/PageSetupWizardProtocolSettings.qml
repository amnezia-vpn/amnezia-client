import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ContainerProps 1.0
import ProtocolProps 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

PageType {
    id: root

    SortFilterProxyModel {
        id: proxyContainersModel
        sourceModel: ContainersModel
        filters: [
            ValueFilter {
                roleName: "isCurrentlyInstalled"
                value: true
            }
        ]
    }

    FlickableType {
        id: fl
        anchors.fill: parent
        contentHeight: content.height

        Column {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            ListView {
                // todo change id naming
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

                        anchors.fill: parent
                        anchors.rightMargin: 16
                        anchors.leftMargin: 16

                        BackButtonType {
                            id: backButton

                            Layout.topMargin: 20
                        }

                        HeaderType {
                            id: header

                            Layout.fillWidth: true

                            headerText: "Установка " + name
                            descriptionText: "Эти настройки можно будет изменить позже"
                        }

                        ParagraphTextType {
                            id: transportProtoHeader

                            Layout.topMargin: 16

                            text: "Network protocol"
                        }

                        Rectangle {
                            id: transportProtoBackground

                            implicitWidth: transportProtoButtonGroup.implicitWidth
                            implicitHeight: transportProtoButtonGroup.implicitHeight

                            color: "#1C1D21"
                            radius: 16

                            RowLayout {
                                id: transportProtoButtonGroup

                                property int currentIndex
                                spacing: 0

                                HorizontalRadioButton {
                                    checked: transportProtoButtonGroup.currentIndex === 0

                                    implicitWidth: (root.width - 32) / 2
                                    text: "UDP"

                                    hoverEnabled: !transportProtoButtonMouseArea.enabled

                                    onClicked: {
                                        transportProtoButtonGroup.currentIndex = 0
                                    }
                                }

                                HorizontalRadioButton {
                                    checked: transportProtoButtonGroup.currentIndex === 1

                                    implicitWidth: (root.width - 32) / 2
                                    text: "TCP"

                                    hoverEnabled: !transportProtoButtonMouseArea.enabled

                                    onClicked: {
                                        transportProtoButtonGroup.currentIndex = 1
                                    }
                                }    
                            }

                            MouseArea {
                                id: transportProtoButtonMouseArea

                                anchors.fill: parent
                            }
                        }

                        TextFieldWithHeaderType {
                            id: port

                            Layout.fillWidth: true
                            Layout.topMargin: 16
                            headerText: "Port"
                        }

                        Rectangle {
                            // todo make it dynamic
                            implicitHeight: root.height - port.implicitHeight -
                                            transportProtoBackground.implicitHeight - transportProtoHeader.implicitHeight -
                                            header.implicitHeight - backButton.implicitHeight - installButton.implicitHeight - 116

                            color: "transparent"
                        }

                        BasicButtonType {
                            id: installButton

                            Layout.fillWidth: true
                            Layout.bottomMargin: 32

                            text: qsTr("Установить")

                            onClicked: function() {
                                goToPage(PageEnum.PageSetupWizardInstalling);
                                InstallController.install(dockerContainer, port.textFieldText, transportProtoButtonGroup.currentIndex)
                            }
                        }

                        Component.onCompleted: {
                            //todo move to protocols model?
                            var defaultContainerProto =  ContainerProps.defaultProtocol(dockerContainer)

                            if (ProtocolProps.defaultPort(defaultContainerProto) < 0) {
                                port.visible = false
                            } else {
                                port.textFieldText = ProtocolProps.defaultPort(defaultContainerProto)
                            }
                            transportProtoButtonGroup.currentIndex = ProtocolProps.defaultTransportProto(defaultContainerProto)

                            port.enabled = ProtocolProps.defaultPortChangeable(defaultContainerProto)
                            transportProtoButtonMouseArea.enabled = !ProtocolProps.defaultTransportProtoChangeable(defaultContainerProto)
                        }
                    }
                }
            }
        }
    }
}

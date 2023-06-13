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
import "../Components"

PageType {
    id: root

    SortFilterProxyModel {
        id: proxyContainersModel
        sourceModel: ContainersModel
        filters: [
            ValueFilter {
                roleName: "isCurrentlyProcessed"
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

                        TransportProtoSelector {
                            id: transportProtoSelector

                            Layout.fillWidth: true
                            rootWidth: root.width
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
                                            transportProtoSelector.implicitHeight - transportProtoHeader.implicitHeight -
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
                                InstallController.install(dockerContainer, port.textFieldText, transportProtoSelector.currentIndex)
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
                            transportProtoSelector.currentIndex = ProtocolProps.defaultTransportProto(defaultContainerProto)

                            port.enabled = ProtocolProps.defaultPortChangeable(defaultContainerProto)
                            transportProtoSelector.mouseArea.enabled = !ProtocolProps.defaultTransportProtoChangeable(defaultContainerProto)
                        }
                    }
                }
            }
        }
    }
}

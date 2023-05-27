import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ProtocolEnum 1.0
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
        id: proxyServersModel
        sourceModel: ServersModel
        filters: [
            ValueFilter {
                roleName: "isCurrentlyProcessed"
                value: true
            }
        ]
    }

        ColumnLayout {
            id: content

            anchors.fill: parent

            spacing: 16

            Repeater {
                id: header
                model: proxyServersModel

                delegate: HeaderType {
                    Layout.fillWidth: true
                    Layout.topMargin: 20
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16

                    actionButtonImage: "qrc:/images/controls/edit-3.svg"
                    backButtonImage: "qrc:/images/controls/arrow-left.svg"

                    headerText: name
                    descriptionText: hostName

                    actionButtonFunction: function() {
                        connectionTypeSelection.visible = true
                    }

                    backButtonFunction: function() {
                        closePage()
                    }
                }
            }

            TabBar {
                id: tabBar

                Layout.fillWidth: true

                background: Rectangle {
                    color: "transparent"
                }

                TabButtonType {
                    isSelected: tabBar.currentIndex === 0
                    text: qsTr("Protocols")
                }
                TabButtonType {
                    isSelected: tabBar.currentIndex === 1
                    text: qsTr("Services")
                }
                TabButtonType {
                    isSelected: tabBar.currentIndex === 2
                    text: qsTr("Data")
                }
            }

            StackLayout {
                id: stackLayout
                currentIndex: tabBar.currentIndex

                Layout.fillWidth: true
                height: root.height

                StackView {
                    id: protocolsStackView
                    initialItem: PageSettingsContainersListView {
                        model: SortFilterProxyModel {
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
                    }
                }

                StackView {
                    id: servicesStackView
                    initialItem: PageSettingsContainersListView {
                        model: SortFilterProxyModel {
                            sourceModel: ContainersModel
                            filters: [
                                ValueFilter {
                                    roleName: "serviceType"
                                    value: ProtocolEnum.Other
                                },
                                ValueFilter {
                                    roleName: "isSupported"
                                    value: true
                                }
                            ]
                        }
                    }
                }

                StackView {
                    id: dataStackView
                    initialItem: PageSettingsData {

                    }
                }
            }
        }
//    }
}

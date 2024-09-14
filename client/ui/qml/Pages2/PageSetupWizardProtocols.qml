import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ProtocolEnum 1.0
import Style 1.0

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
        sorters: RoleSorter {
            roleName: "installPageOrder"
            sortOrder: Qt.AscendingOrder
        }
    }

    ColumnLayout {
        id: backButtonLayout

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        anchors.topMargin: 20

        BackButtonType {
            id: backButton
        }
    }

    FlickableType {
        id: fl
        anchors.top: backButtonLayout.bottom
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

                    headerText: qsTr("VPN protocol")
                    descriptionText: qsTr("Choose the one with the highest priority for you. Later, you can install other protocols and additional services, such as DNS proxy and SFTP.")
                }
            }

            ListView {
                id: containers
                width: parent.width
                height: containers.contentItem.height
                // currentIndex: -1
                clip: true
                interactive: false
                model: proxyContainersModel

                function ensureCurrentItemVisible() {
                    if (currentIndex >= 0) {
                        if (currentItem.y < fl.contentY) {
                            fl.contentY = currentItem.y
                        } else if (currentItem.y + currentItem.height + header.height > fl.contentY + fl.height) {
                            fl.contentY = currentItem.y + currentItem.height  + header.height - fl.height + 40 // 40 is a bottom margin
                        }
                    }
                }

                activeFocusOnTab: true
                Keys.onTabPressed: {
                    if (currentIndex < this.count - 1) {
                        this.incrementCurrentIndex()
                    } else {
                        this.currentIndex = 0
                        focusItem.forceActiveFocus()
                    }

                    ensureCurrentItemVisible()
                }

                onVisibleChanged: {
                    if (visible) {
                        currentIndex = 0
                    }
                }

                delegate: Item {
                    implicitWidth: containers.width
                    implicitHeight: delegateContent.implicitHeight

                    onActiveFocusChanged: {
                        if (activeFocus) {
                            container.rightButton.forceActiveFocus()
                        }
                    }

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
                                ContainersModel.setProcessedContainerIndex(proxyContainersModel.mapToSource(index))
                                PageController.goToPage(PageEnum.PageSetupWizardProtocolSettings)
                            }
                        }

                        DividerType {}
                    }
                }
            }
        }
    }
}

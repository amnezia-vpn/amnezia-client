import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ProtocolEnum 1.0
import ContainerProps 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    defaultActiveFocusItem: focusItem

    Item {
        id: focusItem
        KeyNavigation.tab: backButton
    }

    ColumnLayout {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        anchors.topMargin: 20

        BackButtonType {
            id: backButton
            KeyNavigation.tab: servers
        }

        HeaderType {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            headerText: qsTr("Servers")
        }
    }

    FlickableType {
        id: fl
        anchors.top: header.bottom
        anchors.topMargin: 16
        contentHeight: col.implicitHeight

        Column {
            id: col
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            ListView {
                id: servers
                width: parent.width
                height: servers.contentItem.height

                model: ServersModel

                clip: true
                interactive: false

                activeFocusOnTab: true
                focus: true
                Keys.onTabPressed: {
                    if (currentIndex < servers.count - 1) {
                        servers.incrementCurrentIndex()
                    } else {
                        servers.currentIndex = 0
                        focusItem.forceActiveFocus()
                        root.lastItemTabClicked()
                    }

                    fl.ensureVisible(this.currentItem)
                }

                onVisibleChanged: {
                    if (visible) {
                        currentIndex = 0
                    }
                }

                delegate: Item {
                    implicitWidth: servers.width
                    implicitHeight: delegateContent.implicitHeight

                    onFocusChanged: {
                        if (focus) {
                            server.rightButton.forceActiveFocus()
                        }
                    }

                    ColumnLayout {
                        id: delegateContent

                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right

                        LabelWithButtonType {
                            id: server
                            Layout.fillWidth: true

                            text: name
                            parentFlickable: fl
                            descriptionText: {
                                var servicesNameString = ""
                                var servicesName = ServersModel.getAllInstalledServicesName(index)
                                for (var i = 0; i < servicesName.length; i++) {
                                    servicesNameString += servicesName[i] + " · "
                                }

                                return servicesNameString + hostName
                            }
                            rightImageSource: "qrc:/images/controls/chevron-right.svg"

                            clickedFunction: function() {
                                ServersModel.processedIndex = index
                                PageController.goToPage(PageEnum.PageSettingsServerInfo)
                            }
                        }

                        DividerType {}
                    }
                }
            }
        }
    }
}

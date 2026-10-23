import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ContainerProps 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    ListViewType {
        id: listView

        anchors.fill: parent

        header: NavigationBarType {
            width: listView.width
            height: implicitHeight + 16

            title: ContainersModel.getProcessedContainerName() + qsTr(" settings")
        }

        model: ProtocolsModel

        delegate: Item {
            implicitWidth: listView.width
            implicitHeight: button.implicitHeight

            ListItemType {
                id: button

                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 16
                anchors.rightMargin: 16

                text: qsTr("Show connection options")

                onClicked: {
                    configContentDrawer.open()
                }
            }

            SheetType {
                id: configContentDrawer

                expandedHeight: windowHeight * 0.9

                parent: root

                Item {
                    anchors.fill: parent

                    IconButtonType {
                        id: drawerBackButton

                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.topMargin: 8
                        anchors.leftMargin: 16

                        size: IconButtonType.Small
                        icon.source: "qrc:/images/controls/arrow-left.svg"

                        onClicked: configContentDrawer.close()
                    }

                    ListViewType {
                        id: drawerListView

                        anchors.top: drawerBackButton.bottom
                        anchors.topMargin: 24
                        anchors.bottom: parent.bottom
                        anchors.right: parent.right
                        anchors.left: parent.left

                        header: Item {
                            width: drawerListView.width
                            height: sheetTitle.implicitHeight + 12

                            SectionHeaderType {
                                id: sheetTitle

                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.leftMargin: 16
                                anchors.rightMargin: 16

                                headerText: qsTr("Connection options %1").arg(protocolName)
                            }
                        }

                        model: 1

                        delegate: ColumnLayout {
                            width: drawerListView.width

                            spacing: 0

                            TextArea {
                                id: configText

                                activeFocusOnTab: false

                                Layout.fillWidth: true
                                Layout.leftMargin: 16
                                Layout.rightMargin: 16
                                Layout.bottomMargin: 16 + PageController.safeAreaBottomMargin

                                padding: 0
                                leftPadding: 0
                                rightPadding: 0

                                color: AmneziaStyle.color.textSecondary
                                selectionColor: AmneziaStyle.color.bgStateLayerAccent
                                selectedTextColor: AmneziaStyle.color.textSecondary

                                font.pixelSize: 16
                                font.weight: Font.Normal
                                font.letterSpacing: -0.4
                                font.family: "PT Root UI VF"

                                text: rawConfig

                                wrapMode: Text.Wrap

                                background: null
                            }
                        }
                    }
                }
            }
        }

        footer: ColumnLayout {
            width: listView.width

            spacing: 0

            ListItemType {
                id: removeButton

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: ServersUiController.isProcessedServerHasWriteAccess()

                text: qsTr("Remove ") + ContainersModel.getProcessedContainerName()
                destructive: true

                onClicked: {
                    var headerText = qsTr("Remove %1 from server?").arg(ContainersModel.getProcessedContainerName())
                    var descriptionText = qsTr("All users with whom you shared a connection with will no longer be able to connect to it.")
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        PageController.goToPage(PageEnum.PageDeinstalling)
                        InstallController.removeContainer(ServersUiController.processedServerId, ServersUiController.processedContainerIndex)
                    }
                    var noButtonFunction = function() {}

                    showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 16 + PageController.safeAreaBottomMargin
            }
        }
    }
}

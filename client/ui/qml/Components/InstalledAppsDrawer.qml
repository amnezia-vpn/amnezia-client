import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../Controls2"
import "../Controls2/TextTypes"

import InstalledAppsModel 1.0

DrawerType2 {
    id: root

    anchors.fill: parent
    expandedHeight: parent.height * 0.9

    onAboutToShow: {
        PageController.showBusyIndicator(true)
        installedAppsModel.updateModel()
        PageController.showBusyIndicator(false)
    }

    InstalledAppsModel {
        id: installedAppsModel
    }

    expandedContent: Item {
        id: container

        implicitHeight: expandedHeight

        ColumnLayout {
            id: backButton

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: addButton.top
            anchors.topMargin: 16

            BackButtonType {
                backButtonImage: "qrc:/images/controls/arrow-left.svg"
                backButtonFunction: function() {
                    root.close()
                }
            }

            Header2Type {
                id: header
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                headerText: qsTr("Choose application")
            }

            ListView {
                id: listView

                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.topMargin: 16
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                clip: true
                interactive: true

                model: installedAppsModel

                ScrollBar.vertical: ScrollBar {
                    id: scrollBar
                    policy: ScrollBar.AlwaysOn
                }

                ButtonGroup {
                    id: buttonGroup
                }

                delegate: Item {
                    implicitWidth: root.width
                    implicitHeight: delegateContent.implicitHeight

                    ColumnLayout {
                        id: delegateContent

                        anchors.fill: parent

                        RowLayout {
                            Layout.fillWidth: true

                            Image {
                                source: "image://installedAppImage/" + appIcon

                                sourceSize.width: 48
                                sourceSize.height: 48
                            }

                            CheckBoxType {
                                Layout.fillWidth: true
                                Layout.rightMargin: 24

                                text: appName

                                onCheckedChanged: {
                                    listView.model.selectedStateChanged(index, checked)
                                }
                            }
                        }

                        DividerType {}
                    }
                }
            }
        }

        BasicButtonType {
            id: addButton

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 16
            anchors.rightMargin: 16
            anchors.leftMargin: 16

            text: qsTr("Add selected")

            clickedFunc: function() {
                PageController.showBusyIndicator(true)
                AppSplitTunnelingController.addApps(listView.model.getSelectedAppsInfo())
                PageController.showBusyIndicator(false)
                root.close()
            }
        }
    }
}

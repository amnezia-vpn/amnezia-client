import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../Controls2"
import "../Controls2/TextTypes"

import SortFilterProxyModel 0.2

import InstalledAppsModel 1.0
import Style 1.0

SheetType {
    id: root

    expandedHeight: windowHeight * 0.9

    onAboutToShow: {
        PageController.showBusyIndicator(true)
        installedAppsModel.updateModel()
        PageController.showBusyIndicator(false)
    }

    InstalledAppsModel {
        id: installedAppsModel
    }

    Item {
        id: container
        anchors.fill: parent

        SectionHeaderType {
            id: header

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 12
            anchors.leftMargin: 16
            anchors.rightMargin: 16

            headerText: qsTr("Choose application")
        }

        ListViewType {
            id: listView

            anchors.top: header.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: searchField.top
            anchors.topMargin: 32
            anchors.bottomMargin: 8

            leftMargin: 16
            rightMargin: 16
            topMargin: 16

            model: SortFilterProxyModel {
                id: proxyInstalledAppsModel
                sourceModel: installedAppsModel
                filters: RegExpFilter {
                    roleName: "appName"
                    pattern: ".*" + searchField.text + ".*"
                    caseSensitivity: Qt.CaseInsensitive
                }
            }

            delegate: Item {
                width: listView.width - listView.leftMargin - listView.rightMargin
                implicitHeight: Math.max(72, row.implicitHeight + 24)

                RowLayout {
                    id: row

                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 12

                    spacing: 12

                    CheckBoxType {
                        Layout.fillWidth: true

                        spacing: 20

                        text: appName
                        checked: isAppSelected
                        onToggled: {
                            installedAppsModel.selectedStateChanged(proxyInstalledAppsModel.mapToSource(index), checked)
                        }
                    }

                    Item {
                        Layout.preferredWidth: 48
                        Layout.preferredHeight: 48

                        Image {
                            anchors.centerIn: parent

                            source: "image://installedAppImage/" + appIcon

                            sourceSize.width: 24
                            sourceSize.height: 24
                        }
                    }
                }

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom

                    height: 1
                    color: AmneziaStyle.color.borderSoft
                }
            }
        }

        SearchFieldType {
            id: searchField

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: addButton.top
            anchors.bottomMargin: 12
            anchors.rightMargin: 16
            anchors.leftMargin: 16

            placeholderText: qsTr("application name")
        }

        ButtonType {
            id: addButton

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.bottomMargin: Math.max(16, PageController.safeAreaBottomMargin)
            anchors.rightMargin: 16
            anchors.leftMargin: 16

            text: qsTr("Add selected")

            onClicked: {
                PageController.showBusyIndicator(true)
                AppSplitTunnelingController.addApps(installedAppsModel.getSelectedAppsInfo())
                PageController.showBusyIndicator(false)
                root.close()
            }
        }
    }
}

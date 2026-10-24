import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"
import Qt.labs.platform 1.1

PageType {
    id: root

    property string selectedConfigName: ""
    property int    selectedConfigIndex: -1

    Component.onCompleted: XrayConfigSnapshotsModel.reload()

    function saveConfigToFile(json) {
        var fileName = ""
        if (GC.isMobile()) {
            fileName = "amnezia_xray_config.json"
        } else {
            fileName = SystemController.getFileName(
                qsTr("Save XRay configuration"),
                qsTr("JSON files (*.json)"),
                StandardPaths.standardLocations(StandardPaths.DocumentsLocation) + "/amnezia_xray_config",
                true,
                ".json")
        }
        if (fileName !== "") {
            PageController.showBusyIndicator(true)
            ExportController.setConfigFromString(json, fileName)
            PageController.showBusyIndicator(false)
            PageController.showNotificationMessage(qsTr("Configuration saved"))
        }
    }

    ListViewType {
        id: listView

        anchors.fill: parent

        model: XrayConfigSnapshotsModel

        header: ColumnLayout {
            width: listView.width

            z: 2

            spacing: 0

            NavigationBarType {
                Layout.fillWidth: true

                title: qsTr("XRay Configurations")
            }

            ListItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Create configuration based on current settings")
                maximumLineCount: 2
                trailingIconSource: "qrc:/images/controls/chevron-right.svg"

                onClicked: XrayConfigSnapshotsModel.createFromCurrentModel()
            }

            ListItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Export settings")
                trailingIconSource: "qrc:/images/controls/chevron-right.svg"

                onClicked: {
                    var idx = root.selectedConfigIndex >= 0 ? root.selectedConfigIndex : 0
                    if (listView.count > 0) {
                        var json = XrayConfigSnapshotsModel.exportToJson(idx)
                        saveConfigToFile(json)
                    }
                }
            }

            ListItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Import settings")
                subtitle: qsTr("In JSON format")
                trailingIconSource: "qrc:/images/controls/chevron-right.svg"

                onClicked: {
                    var filePath = SystemController.getFileName(
                        qsTr("Open XRay configuration"),
                        qsTr("JSON files (*.json)"))
                    if (filePath !== "") {
                        var jsonContent = ImportController.readTextFile(filePath)
                        if (jsonContent !== "") {
                            if (!XrayConfigSnapshotsModel.importFromJson(jsonContent)) {
                                PageController.showNotificationMessage(qsTr("Failed to import configuration"))
                            } else {
                                PageController.showNotificationMessage(qsTr("Configuration imported successfully"))
                            }
                        }
                    }
                }
            }

            SmallTextType {
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.bottomMargin: 12
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: listView.count > 0
                text: qsTr("Configurations")
            }
        }

        footer: ColumnLayout {
            width: listView.width

            spacing: 0

            BodyTextType {
                Layout.fillWidth: true
                Layout.topMargin: 32
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: listView.count === 0
                text: qsTr("No saved configurations yet.\nCreate one from the current settings.")
                color: AmneziaStyle.color.textTertiary
                horizontalAlignment: Text.AlignHCenter
            }

            Item {
                Layout.preferredHeight: 16 + PageController.safeAreaBottomMargin
            }
        }

        delegate: ColumnLayout {
            width: listView.width

            spacing: 0

            ListItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: configName
                subtitle: configDate
                trailingIconSource: "qrc:/images/controls/more-vertical.svg"

                onClicked: {
                    root.selectedConfigName = configName
                    root.selectedConfigIndex = index
                    configActionsDrawer.open()
                }
            }
        }
    }

    Connections {
        target: XrayConfigSnapshotsModel

        function onImportFailed(errorMessage) {
            PageController.showNotificationMessage(errorMessage)
        }
    }

    SheetType {
        id: configActionsDrawer

        parent: root

        expandedHeight: drawerContent.implicitHeight + 16 + PageController.safeAreaBottomMargin

        ColumnLayout {
            id: drawerContent

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0

            SectionHeaderType {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.bottomMargin: 32
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: root.selectedConfigName
            }

            ListItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Apply configuration")
                trailingIconSource: "qrc:/images/controls/chevron-right.svg"

                onClicked: {
                    configActionsDrawer.close()
                    XrayConfigSnapshotsModel.applyConfigToCurrentModel(root.selectedConfigIndex)
                    PageController.closePage()
                }
            }

            ListItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Export configuration")
                trailingIconSource: "qrc:/images/controls/chevron-right.svg"

                onClicked: {
                    configActionsDrawer.close()
                    var json = XrayConfigSnapshotsModel.exportToJson(root.selectedConfigIndex)
                    saveConfigToFile(json)
                }
            }

            ListItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Delete configuration")
                destructive: true

                onClicked: {
                    configActionsDrawer.close()
                    var yesButtonFunction = function () {
                        XrayConfigSnapshotsModel.removeConfig(root.selectedConfigIndex)
                        root.selectedConfigIndex = -1
                        root.selectedConfigName = ""
                    }
                    Dialogs.askQuestion(
                        qsTr("Delete configuration?"),
                        qsTr("This action cannot be undone."),
                        qsTr("Delete"), qsTr("Cancel"),
                        yesButtonFunction, function () {
                        })
                }
            }
        }
    }
}

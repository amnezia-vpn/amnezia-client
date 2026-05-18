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

    // Reload the list every time we open this page
    Component.onCompleted: XrayConfigSnapshotsModel.reload()

    // ── Save xray config snapshot to file ────────────────────────────
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


    BackButtonType {
        id: backButton
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20 + PageController.safeAreaTopMargin
    }

    ListViewType {
        id: listView
        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        model: XrayConfigSnapshotsModel

        header: ColumnLayout {
            width: listView.width
            spacing: 0

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 0
                Layout.bottomMargin: 24
                headerText: qsTr("XRay Configurations")
            }

            // ── Create from current settings ──────────────────────────
            LabelWithButtonType {
                Layout.fillWidth: true
                text: qsTr("Create configuration based on current settings")
                textMaximumLineCount: 2
                rightImageSource: "qrc:/images/controls/chevron-right.svg"
                clickedFunction: function () {
                    XrayConfigSnapshotsModel.createFromCurrentModel()
                }
            }

            DividerType {
            }

            // ── Export ────────────────────────────────────────────────
            LabelWithButtonType {
                Layout.fillWidth: true
                text: qsTr("Export settings")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"
                clickedFunction: function () {
                    var idx = root.selectedConfigIndex >= 0 ? root.selectedConfigIndex : 0
                    if (listView.count > 0) {
                        var json = XrayConfigSnapshotsModel.exportToJson(idx)
                        saveConfigToFile(json)
                    }
                }
            }

            DividerType {
            }

            // ── Import ────────────────────────────────────────────────
            LabelWithButtonType {
                Layout.fillWidth: true
                text: qsTr("Import settings")
                descriptionText: qsTr("In JSON format")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"
                clickedFunction: function () {
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

            DividerType {
            }

            // ── Section label ─────────────────────────────────────────
            CaptionTextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 24
                Layout.bottomMargin: 8
                text: qsTr("Configurations")
                color: AmneziaStyle.color.mutedGray
                visible: listView.count > 0
            }
        }

        // ── Empty state ───────────────────────────────────────────────
        footer: ColumnLayout {
            width: listView.width
            visible: listView.count === 0
            spacing: 0

            Item {
                Layout.preferredHeight: 32
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: qsTr("No saved configurations yet.\nCreate one from the current settings.")
                color: AmneziaStyle.color.mutedGray
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }
        }

        // ── Config list items ─────────────────────────────────────────
        delegate: ColumnLayout {
            width: listView.width
            spacing: 0

            LabelWithButtonType {
                Layout.fillWidth: true
                text: configName
                descriptionText: configDate
                rightImageSource: "qrc:/images/controls/more-vertical.svg"
                clickedFunction: function () {
                    root.selectedConfigName = configName
                    root.selectedConfigIndex = index
                    configActionsDrawer.openTriggered()
                }
            }

            DividerType {
            }
        }
    }

    // ── Import result handler ─────────────────────────────────────────
    Connections {
        target: XrayConfigSnapshotsModel

        function onImportFailed(errorMessage) {
            PageController.showNotificationMessage(errorMessage)
        }
    }

    // ── Per-config actions drawer ─────────────────────────────────────
    DrawerType2 {
        id: configActionsDrawer
        parent: root
        anchors.fill: parent
        expandedHeight: root.height * 0.35

        expandedStateContent: ColumnLayout {
            id: drawerContent
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: 0

            onImplicitHeightChanged: {
                configActionsDrawer.expandedHeight = drawerContent.implicitHeight + 32
            }

            BackButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 16
                backButtonFunction: function () {
                    configActionsDrawer.closeTriggered()
                }
            }

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 8
                Layout.bottomMargin: 16
                headerText: root.selectedConfigName
            }

            // Apply
            LabelWithButtonType {
                Layout.fillWidth: true
                text: qsTr("Apply configuration")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"
                clickedFunction: function () {
                    configActionsDrawer.closeTriggered()
                    XrayConfigSnapshotsModel.applyConfigToCurrentModel(root.selectedConfigIndex)
                    PageController.closePage()
                }
            }

            DividerType {
            }

            // Export this config
            LabelWithButtonType {
                Layout.fillWidth: true
                text: qsTr("Export configuration")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"
                clickedFunction: function () {
                    configActionsDrawer.closeTriggered()
                    var json = XrayConfigSnapshotsModel.exportToJson(root.selectedConfigIndex)
                    saveConfigToFile(json)
                }
            }

            DividerType {
            }

            // Delete
            LabelWithButtonType {
                Layout.fillWidth: true
                text: qsTr("Delete configuration")
                textColor: AmneziaStyle.color.vibrantRed
                clickedFunction: function () {
                    configActionsDrawer.closeTriggered()
                    var yesButtonFunction = function () {
                        XrayConfigSnapshotsModel.removeConfig(root.selectedConfigIndex)
                        root.selectedConfigIndex = -1
                        root.selectedConfigName = ""
                    }
                    showQuestionDrawer(
                        qsTr("Delete configuration?"),
                        qsTr("This action cannot be undone."),
                        qsTr("Delete"), qsTr("Cancel"),
                        yesButtonFunction, function () {
                        })
                }
            }

            DividerType {
            }
            Item {
                Layout.preferredHeight: 16
            }
        }
    }
}

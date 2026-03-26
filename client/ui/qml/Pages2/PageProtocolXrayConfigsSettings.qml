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

PageType {
    id: root

    property string selectedConfigName:  ""
    property int    selectedConfigIndex: -1

    // Reload the list every time we open this page
    Component.onCompleted: XrayConfigsModel.reload()

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

        model: XrayConfigsModel

        header: ColumnLayout {
            width: listView.width
            spacing: 0

            Header2TextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 0
                Layout.bottomMargin: 24
                text: qsTr("XRay Configurations")
                wrapMode: Text.WordWrap
            }

            // ── Create from current settings ──────────────────────────
            LabelWithButtonType {
                Layout.fillWidth: true
                text: qsTr("Create configuration based on current settings")
                textMaximumLineCount: 2
                rightImageSource: "qrc:/images/controls/chevron-right.svg"
                clickedFunction: function() {
                    XrayConfigsModel.createFromCurrent(XrayConfigModel.getProtocolConfig().serverConfig)
                }
            }

            DividerType {}

            // ── Export ────────────────────────────────────────────────
            LabelWithButtonType {
                Layout.fillWidth: true
                text: qsTr("Export settings")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"
                clickedFunction: function() {
                    if (root.selectedConfigIndex >= 0) {
                        var json = XrayConfigsModel.exportToJson(root.selectedConfigIndex)
                        ExportController.shareText(json, "xray_config.json")
                    } else if (XrayConfigsModel.rowCount() > 0) {
                        // Export the first one if none selected
                        var json = XrayConfigsModel.exportToJson(0)
                        ExportController.shareText(json, "xray_config.json")
                    }
                }
            }

            DividerType {}

            // ── Import ────────────────────────────────────────────────
            LabelWithButtonType {
                Layout.fillWidth: true
                text: qsTr("Import settings")
                descriptionText: qsTr("In JSON format")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"
                clickedFunction: function() {
                    ImportController.importConfig()
                }
            }

            DividerType {}

            // ── Section label ─────────────────────────────────────────
            CaptionTextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 24
                Layout.bottomMargin: 8
                text: qsTr("Configurations")
                color: AmneziaStyle.color.mutedGray
                visible: XrayConfigsModel.rowCount() > 0
            }
        }

        // ── Empty state ───────────────────────────────────────────────
        footer: ColumnLayout {
            width: listView.width
            visible: XrayConfigsModel.rowCount() === 0
            spacing: 0

            Item { Layout.preferredHeight: 32 }

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
                clickedFunction: function() {
                    root.selectedConfigName  = configName
                    root.selectedConfigIndex = index
                    configActionsDrawer.openTriggered()
                }
            }

            DividerType {}
        }
    }

    // ── Import result handler ─────────────────────────────────────────
    Connections {
        target: XrayConfigsModel
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
                backButtonFunction: function() {
                    configActionsDrawer.closeTriggered()
                }
            }

            Header2TextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 8
                Layout.bottomMargin: 16
                text: root.selectedConfigName
                wrapMode: Text.WordWrap
            }

            // Apply
            LabelWithButtonType {
                Layout.fillWidth: true
                text: qsTr("Apply configuration")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"
                clickedFunction: function() {
                    configActionsDrawer.closeTriggered()
                    var serverConfig = XrayConfigsModel.applyConfig(root.selectedConfigIndex)
                    XrayConfigModel.applyServerConfig(serverConfig)
                    PageController.closePage()
                }
            }

            DividerType {}

            // Export this config
            LabelWithButtonType {
                Layout.fillWidth: true
                text: qsTr("Export configuration")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"
                clickedFunction: function() {
                    configActionsDrawer.closeTriggered()
                    var json = XrayConfigsModel.exportToJson(root.selectedConfigIndex)
                    ExportController.shareText(json, "xray_config.json")
                }
            }

            DividerType {}

            // Delete
            LabelWithButtonType {
                Layout.fillWidth: true
                text: qsTr("Delete configuration")
                textColor: AmneziaStyle.color.vibrantRed
                clickedFunction: function() {
                    configActionsDrawer.closeTriggered()
                    var yesButtonFunction = function() {
                        XrayConfigsModel.removeConfig(root.selectedConfigIndex)
                        root.selectedConfigIndex = -1
                        root.selectedConfigName  = ""
                    }
                    showQuestionDrawer(
                        qsTr("Delete configuration?"),
                        qsTr("This action cannot be undone."),
                        qsTr("Delete"), qsTr("Cancel"),
                        yesButtonFunction, function() {})
                }
            }

            DividerType {}
            Item { Layout.preferredHeight: 16 }
        }
    }
}

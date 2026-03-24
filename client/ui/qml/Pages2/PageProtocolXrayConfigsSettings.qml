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

    // Temporary model — will be replaced by real XrayConfigsModel
    ListModel {
        id: configsModel
        ListElement {
            configName: "XHTTP TLS Reality"; configDate: "24.02.2026 11:12"
        }
        ListElement {
            configName: "RAW (TCP) TLS Reality"; configDate: "24.02.2026 11:14"
        }
        ListElement {
            configName: "RAW (TCP) TLS Reality"; configDate: "24.02.2026 11:14"
        }
        ListElement {
            configName: "RAW (TCP) TLS Reality"; configDate: "24.02.2026 11:15"
        }
    }

    // Currently selected config for the drawer
    property string selectedConfigName: ""
    property int selectedConfigIndex: -1

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

        header: ColumnLayout {
            width: listView.width
            spacing: 0

            // ── Header ────────────────────────────────────────────────
            Header2TextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 0
                Layout.bottomMargin: 24
                text: qsTr("XRay Configurations")
                wrapMode: Text.WordWrap
            }

            // ── Create config from current settings ───────────────────
            LabelWithButtonType {
                Layout.fillWidth: true
                text: qsTr("Create configuration based on current settings")
                textMaximumLineCount: 2
                rightImageSource: "qrc:/images/controls/chevron-right.svg"
                clickedFunction: function () {
                    // XrayConfigModel.createConfigFromCurrent()
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
                    // XrayConfigModel.exportSettings()
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
                    // XrayConfigModel.importSettings()
                }
            }

            DividerType {
            }

            // ── Configurations section label ──────────────────────────
            CaptionTextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 24
                Layout.bottomMargin: 8
                text: qsTr("Configurations")
                color: AmneziaStyle.color.mutedGray
            }
        }

        model: configsModel

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

    // ── Per-config actions drawer ─────────────────────────────────────
    DrawerType2 {
        id: configActionsDrawer

        parent: root
        anchors.fill: parent
        expandedHeight: root.height * 0.4

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

            Header2TextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 8
                Layout.bottomMargin: 16
                text: root.selectedConfigName
                wrapMode: Text.WordWrap
            }

            // ── Apply config ──────────────────────────────────────────
            LabelWithButtonType {
                Layout.fillWidth: true
                text: qsTr("Apply configuration")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"
                clickedFunction: function () {
                    configActionsDrawer.closeTriggered()
                    // XrayConfigModel.applyConfig(root.selectedConfigIndex)
                }
            }

            DividerType {
            }

            // ── Delete config ─────────────────────────────────────────
            LabelWithButtonType {
                Layout.fillWidth: true
                text: qsTr("Delete configuration")
                textColor: AmneziaStyle.color.vibrantRed
                clickedFunction: function () {
                    configActionsDrawer.closeTriggered()
                    // XrayConfigModel.deleteConfig(root.selectedConfigIndex)
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

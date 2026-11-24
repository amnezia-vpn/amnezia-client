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

    property string menuPresetId
    property string menuPresetName

    readonly property int builtinCount: listCount - XrayPresetsModel.customCount
    property int listCount: presetsRepeater.count

    Component.onCompleted: console.log("[XrayUI][Presets] opened, presets", presetsRepeater.count, "custom", XrayPresetsModel.customCount, "selected", XraySettingsModel.presetId)
    Component.onDestruction: console.log("[XrayUI][Presets] closed")

    AlertDialogType {
        id: deleteDialog

        headerText: qsTr("Delete preset?")
        descriptionText: qsTr("“%1” will be removed from the list. Server settings stay as they are").arg(root.menuPresetName)
        confirmButtonText: qsTr("Delete")

        onOpened: console.log("[XrayUI][Presets] delete dialog opened for", root.menuPresetId)
        onConfirmed: {
            console.log("[XrayUI][Presets] delete confirmed", root.menuPresetId, root.menuPresetName)
            XrayPresetsModel.removePreset(root.menuPresetId)
        }
    }

    PopupMenuType {
        id: presetMenu

        onOpened: console.log("[XrayUI][Presets] menu opened for", root.menuPresetId)

        PopupMenuItemType {
            text: qsTr("Share preset")
            iconSource: "qrc:/images/controls/share-2.svg"

            onClicked: {
                console.log("[XrayUI][Presets] menu share", root.menuPresetId)
                presetMenu.close()
                PageController.goToPage(PageEnum.PageProtocolXraySharePreset, true, { presetId: root.menuPresetId })
            }
        }

        PopupMenuSeparatorType {}

        PopupMenuItemType {
            text: qsTr("Delete preset")
            iconSource: "qrc:/images/controls/trash.svg"
            destructive: true

            onClicked: {
                console.log("[XrayUI][Presets] menu delete", root.menuPresetId)
                presetMenu.close()
                deleteDialog.open()
            }
        }
    }

    XraySavePresetSheet {
        id: savePresetSheet

        parent: root

        defaultName: XraySettingsModel.presetEdited ? XraySettingsModel.presetName + " · " + qsTr("edited")
                                                    : XraySettingsModel.presetName

        onSaveRequested: function(name) {
            const id = XraySettingsModel.saveAsPreset(name)
            console.log("[XrayUI][Presets] saved current config as preset", id, name)
            PageController.showNotificationMessage(qsTr("Preset saved"))
        }
    }

    FlickableType {
        id: flickable

        anchors.fill: parent

        contentHeight: content.implicitHeight + 16 + PageController.safeAreaBottomMargin

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0

            NavigationBarType {
                Layout.fillWidth: true

                title: qsTr("Presets")
            }

            SmallTextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 12

                text: qsTr("Built-in")
            }

            ListGroupType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                Repeater {
                    id: presetsRepeater

                    model: XrayPresetsModel

                    delegate: RadioItemType {
                        required property int index
                        required property string presetId
                        required property string name
                        required property string description
                        required property bool builtin
                        required property bool recommended
                        required property bool selected

                        Layout.fillWidth: true

                        visible: builtin
                        text: name
                        subtitle: description
                        checked: selected
                        showSeparator: index < root.builtinCount - 1

                        onClicked: {
                            console.log("[XrayUI][Presets] built-in preset clicked", presetId, name)
                            XraySettingsModel.selectPreset(presetId)
                        }

                        RecommendationBadgeType {
                            visible: recommended
                            text: qsTr("Recommended")
                        }
                    }
                }
            }

            SmallTextType {
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 12

                visible: XrayPresetsModel.customCount > 0
                text: qsTr("Custom")
            }

            ListGroupType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: XrayPresetsModel.customCount > 0

                Repeater {
                    model: XrayPresetsModel

                    delegate: RadioItemType {
                        id: customItem

                        required property int index
                        required property string presetId
                        required property string name
                        required property string createdAt
                        required property bool builtin
                        required property bool selected

                        Layout.fillWidth: true

                        visible: !builtin
                        text: name
                        subtitle: createdAt
                        checked: selected
                        showSeparator: index < root.listCount - 1

                        onClicked: {
                            console.log("[XrayUI][Presets] custom preset clicked", presetId, name)
                            XraySettingsModel.selectPreset(presetId)
                        }

                        IconButtonType {
                            id: menuButton

                            icon.source: "qrc:/images/controls/more-vertical.svg"

                            onClicked: {
                                root.menuPresetId = customItem.presetId
                                root.menuPresetName = customItem.name
                                presetMenu.openBelow(menuButton)
                            }
                        }
                    }
                }
            }

            ListGroupType {
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                ListItemType {
                    Layout.fillWidth: true

                    text: qsTr("Import preset")
                    subtitle: qsTr("From a .json file or pasted text")
                    trailingIconSource: "qrc:/images/controls/chevron-right.svg"

                    onClicked: {
                        console.log("[XrayUI][Presets] import preset clicked")
                        PageController.goToPage(PageEnum.PageProtocolXrayImportPreset)
                    }
                }

                ListItemType {
                    Layout.fillWidth: true

                    text: qsTr("Save as preset")
                    trailingIconSource: "qrc:/images/controls/chevron-right.svg"

                    onClicked: {
                        console.log("[XrayUI][Presets] save as preset clicked")
                        savePresetSheet.open()
                    }
                }

                ListItemType {
                    Layout.fillWidth: true

                    showSeparator: false
                    text: qsTr("Share current preset")
                    trailingIconSource: "qrc:/images/controls/chevron-right.svg"

                    onClicked: {
                        console.log("[XrayUI][Presets] share current preset clicked")
                        PageController.goToPage(PageEnum.PageProtocolXraySharePreset, true, { presetId: "" })
                    }
                }
            }
        }
    }
}

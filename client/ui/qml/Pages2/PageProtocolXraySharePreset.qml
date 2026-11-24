import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform 1.1

import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    property string presetId

    readonly property string presetName: root.presetId === "" ? XraySettingsModel.presetName : XrayPresetsModel.presetName(root.presetId)
    readonly property string presetJson: root.presetId === "" ? XraySettingsModel.inboundText : XrayPresetsModel.presetJson(root.presetId)

    function shareAsFile() {
        const baseName = "amnezia_xray_preset"
        let fileName = ""
        if (GC.isMobile()) {
            fileName = baseName + ".json"
        } else {
            fileName = SystemController.getFileName(qsTr("Save XRay preset"), qsTr("JSON files (*.json)"),
                                                    StandardPaths.standardLocations(StandardPaths.DocumentsLocation) + "/" + baseName,
                                                    true, ".json")
        }
        console.log("[XrayUI][SharePreset] share as file, target", fileName, "length", root.presetJson.length)
        if (fileName === "") {
            return
        }
        PageController.showBusyIndicator(true)
        ExportController.setConfigFromString(root.presetJson, fileName)
        PageController.showBusyIndicator(false)
        PageController.showNotificationMessage(qsTr("Preset saved"))
    }

    Component.onCompleted: console.log("[XrayUI][SharePreset] opened, preset", root.presetId === "" ? "current" : root.presetId, root.presetName, "length", root.presetJson.length)
    Component.onDestruction: console.log("[XrayUI][SharePreset] closed")

    ColumnLayout {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: buttons.top
        anchors.bottomMargin: 24

        spacing: 0

        NavigationBarType {
            Layout.fillWidth: true

            title: qsTr("Share preset")
        }

        BodyTextType {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            text: root.presetName
            color: AmneziaStyle.color.textTertiary
        }

        SmallTextType {
            Layout.fillWidth: true
            Layout.topMargin: 16
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.bottomMargin: 24

            text: qsTr("Server keys, UUIDs and certificates are not shared. In the file they are saved as @auto, and the recipient's app fills in its own")
        }

        XrayCodeEditor {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            logTag: "SharePreset"
            readOnly: true
            text: root.presetJson
        }
    }

    ColumnLayout {
        id: buttons

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.bottomMargin: 8 + PageController.safeAreaBottomMargin

        spacing: 12

        ButtonType {
            Layout.fillWidth: true

            leftImageSource: "qrc:/images/controls/share-2.svg"
            text: qsTr("Share as file")

            onClicked: root.shareAsFile()
        }

        ButtonType {
            Layout.fillWidth: true

            variant: ButtonType.Secondary
            leftImageSource: "qrc:/images/controls/copy.svg"
            text: qsTr("Copy JSON")

            onClicked: {
                console.log("[XrayUI][SharePreset] copy JSON, length", root.presetJson.length)
                GC.copyToClipBoard(root.presetJson)
                PageController.showNotificationMessage(qsTr("Copied"))
            }
        }
    }
}

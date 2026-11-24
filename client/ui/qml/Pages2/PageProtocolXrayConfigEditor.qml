import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    focusOnActivation: false

    function leave() {
        console.log("[XrayUI][ConfigEditor] back clicked, modified", XraySettingsModel.draftModified)
        if (XraySettingsModel.draftModified) {
            discardDialog.open()
            return
        }
        PageController.closePage()
    }

    Component.onCompleted: console.log("[XrayUI][ConfigEditor] opened, preset", XraySettingsModel.presetId, "valid", XraySettingsModel.draftValid, "length", XraySettingsModel.draftText.length)
    Component.onDestruction: console.log("[XrayUI][ConfigEditor] closed")

    Connections {
        target: XraySettingsModel

        function onDraftChanged() {
            console.log("[XrayUI][ConfigEditor] draft changed, valid", XraySettingsModel.draftValid, "errorLine", XraySettingsModel.draftErrorLine, "status", XraySettingsModel.draftStatus)
            if (editor.text !== XraySettingsModel.draftText) {
                console.log("[XrayUI][ConfigEditor] editor text replaced from model")
                editor.text = XraySettingsModel.draftText
            }
        }
    }

    AlertDialogType {
        id: discardDialog

        headerText: qsTr("Discard changes?")
        descriptionText: qsTr("Your edits to the config will be lost")
        confirmButtonText: qsTr("Discard")

        onOpened: console.log("[XrayUI][ConfigEditor] discard dialog opened")
        onConfirmed: {
            console.log("[XrayUI][ConfigEditor] discard confirmed")
            PageController.closePage()
        }
    }

    ColumnLayout {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: doneButton.top
        anchors.bottomMargin: 24

        spacing: 0

        NavigationBarType {
            Layout.fillWidth: true

            title: XraySettingsModel.presetName
            backAction: function() { root.leave() }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.bottomMargin: 16

            spacing: 8

            SmallTextType {
                Layout.fillWidth: true

                text: "server.json"
            }

            ButtonType {
                variant: ButtonType.Surface
                size: ButtonType.Small
                leftImageSource: "qrc:/images/controls/braces.svg"
                text: qsTr("Format")

                onClicked: {
                    const formatted = XraySettingsModel.formatDraft()
                    console.log("[XrayUI][ConfigEditor] format clicked, formatted", formatted)
                    if (!formatted) {
                        PageController.showNotificationMessage(qsTr("Fix the JSON syntax first"))
                    }
                }
            }

            ButtonType {
                variant: ButtonType.Surface
                size: ButtonType.Small
                leftImageSource: "qrc:/images/controls/copy.svg"
                text: qsTr("Copy")

                onClicked: {
                    console.log("[XrayUI][ConfigEditor] copy clicked, length", editor.text.length)
                    GC.copyToClipBoard(editor.text)
                    PageController.showNotificationMessage(qsTr("Copied"))
                }
            }
        }

        XrayCodeEditor {
            id: editor

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            logTag: "ConfigEditor"
            text: XraySettingsModel.draftText
            errorLine: XraySettingsModel.draftErrorLine

            onEdited: function(text) {
                XraySettingsModel.setDraft(text)
            }
        }

        SmallTextType {
            Layout.fillWidth: true
            Layout.topMargin: 24
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            text: XraySettingsModel.draftStatus
            color: XraySettingsModel.draftHasError ? AmneziaStyle.color.statusDestructive
                                                   : AmneziaStyle.color.textTertiary
        }
    }

    ButtonType {
        id: doneButton

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.bottomMargin: 8 + PageController.safeAreaBottomMargin + PageController.imeHeight

        enabled: XraySettingsModel.draftValid
        text: qsTr("Done")

        onClicked: {
            const committed = XraySettingsModel.commitDraft()
            console.log("[XrayUI][ConfigEditor] done clicked, committed", committed)
            if (committed) {
                PageController.closePage()
            }
        }
    }
}

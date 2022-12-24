import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.15
import PageEnum 1.0
import "./"
import "../Controls"
import "../Config"

PageBase {
    id: root
    page: PageEnum.ViewConfig
    logic: ViewConfigLogic

    readonly property double rowHeight: ta_last_config.contentHeight / ta_last_config.textArea.lineCount

    BackButton {}

    Caption {
        id: caption
        text: qsTr("Check config")
    }

    FlickableType {
        id: fl
        width: root.width
        anchors.top: caption.bottom
        anchors.topMargin: 20
        anchors.bottom: root.bottom
        anchors.bottomMargin: 20
        anchors.left: root.left
        anchors.leftMargin: 30
        anchors.right: root.right
        anchors.rightMargin: 30

        contentHeight: content.height
        clip: true

        ColumnLayout {
            id: content
            enabled: logic.pageEnabled
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            TextAreaType {
                id: ta_config

                Layout.topMargin: 5
                Layout.bottomMargin: 20
                Layout.fillWidth: true
                Layout.leftMargin: 1
                Layout.rightMargin: 1
                Layout.preferredHeight: ViewConfigLogic.warningActive ? 250 : fl.height - 70
                flickableDirection: Flickable.AutoFlickIfNeeded

                textArea.readOnly: true
                textArea.text: logic.configText
            }

            LabelType {
                id: lb_att
                visible: ViewConfigLogic.warningActive
                text: qsTr("Attention!
The config above contains cached OpenVPN connection profile.
AmneziaVPN detected this profile may contain malicious scripts. Please, carefully review the config and import this config only if you completely trust it.")
                Layout.fillWidth: true
            }

            LabelType {
                visible: ViewConfigLogic.warningActive
                text: qsTr("Suspicious string:")
                Layout.fillWidth: true
            }

            TextAreaType {
                id: ta_mal
                visible: ViewConfigLogic.warningActive

                Layout.topMargin: 5
                Layout.bottomMargin: 20
                Layout.fillWidth: true
                Layout.leftMargin: 1
                Layout.rightMargin: 1
                Layout.preferredHeight: 60
                flickableDirection: Flickable.AutoFlickIfNeeded

                textArea.readOnly: true
                textArea.text: logic.openVpnMalStrings
                textArea.textFormat: TextEdit.RichText
            }

            LabelType {
                visible: ViewConfigLogic.warningActive
                text: qsTr("Cached connection profile:")
                Layout.fillWidth: true
            }

            TextAreaType {
                id: ta_last_config
                visible: ViewConfigLogic.warningActive

                Layout.topMargin: 5
                Layout.bottomMargin: 20
                Layout.fillWidth: true
                Layout.leftMargin: 1
                Layout.rightMargin: 1
                Layout.preferredHeight: 350
                flickableDirection: Flickable.AutoFlickIfNeeded

                textArea.readOnly: true
                textArea.text: logic.openVpnLastConfigs
                textArea.textFormat: TextEdit.RichText

                Connections {
                    target: logic
                    function onWarningStringNumberChanged(n) {
                        ta_last_config.contentY = rowHeight * n - ta_last_config.height / 2
                    }
                }
            }

            RowLayout {
                id: btns_row

                BasicButtonType {
                    Layout.preferredWidth: (content.width - parent.spacing) /2
                    Layout.preferredHeight: 41
                    font.pixelSize: btn_import.font.pixelSize
                    text: qsTr("Cancel")
                    onClicked: {
                        UiLogic.closePage()
                    }
                }

                BlueButtonType {
                    id: btn_import
                    Layout.preferredWidth: (content.width - parent.spacing) /2
                    Layout.preferredHeight: 41
                    text: qsTr("Import config")
                    onClicked: {
                        logic.importConfig()
                    }
                }
            }
        }
    }

 }

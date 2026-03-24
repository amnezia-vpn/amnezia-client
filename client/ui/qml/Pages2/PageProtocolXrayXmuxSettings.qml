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

    // Temporary local state
    property bool   xmuxEnabled: true
    property string maxConcurrencyMin: "0"
    property string maxConcurrencyMax: "0"
    property string maxConnectionsMin: "0"
    property string maxConnectionsMax: "0"
    property string cMaxReuseTimesMin: "0"
    property string cMaxReuseTimesMax: "0"
    property string hMaxRequestTimesMin: "0"
    property string hMaxRequestTimesMax: "0"
    property string hMaxReusableSecsMin: "0"
    property string hMaxReusableSecsMax: "0"
    property string hKeepAlivePeriod: ""

    BackButtonType {
        id: backButton
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20 + PageController.safeAreaTopMargin
    }

    FlickableType {
        id: flickable
        anchors.top: backButton.bottom
        anchors.bottom: saveButton.top
        anchors.left: parent.left
        anchors.right: parent.right
        contentHeight: mainColumn.implicitHeight

        ColumnLayout {
            id: mainColumn
            width: flickable.width
            spacing: 0

            // ── Header ────────────────────────────────────────────────
            Header2TextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 0
                Layout.bottomMargin: 24
                text: qsTr("xmux")
            }

            // ── xmux master switcher ──────────────────────────────────
            SwitcherType {
                Layout.fillWidth: true
                Layout.margins: 16
                text: qsTr("xmux")
                checked: root.xmuxEnabled
                onToggled: root.xmuxEnabled = checked
            }

            DividerType {
            }

            // ── Min/Max pairs (only when enabled) ─────────────────────
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0
                enabled: root.xmuxEnabled

                // maxConcurrency
                CaptionTextType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 24
                    Layout.bottomMargin: 8
                    text: qsTr("maxConcurrency")
                    color: AmneziaStyle.color.mutedGray
                }
                MinMaxRowType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    minValue: root.maxConcurrencyMin
                    maxValue: root.maxConcurrencyMax
                    onMinChanged: root.maxConcurrencyMin = val
                    onMaxChanged: root.maxConcurrencyMax = val
                }

                // maxConnections
                CaptionTextType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 16
                    Layout.bottomMargin: 8
                    text: qsTr("maxConnections")
                    color: AmneziaStyle.color.mutedGray
                }
                MinMaxRowType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    minValue: root.maxConnectionsMin
                    maxValue: root.maxConnectionsMax
                    onMinChanged: root.maxConnectionsMin = val
                    onMaxChanged: root.maxConnectionsMax = val
                }

                // cMaxReuseTimes
                CaptionTextType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 16
                    Layout.bottomMargin: 8
                    text: qsTr("cMaxReuseTimes")
                    color: AmneziaStyle.color.mutedGray
                }
                MinMaxRowType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    minValue: root.cMaxReuseTimesMin
                    maxValue: root.cMaxReuseTimesMax
                    onMinChanged: root.cMaxReuseTimesMin = val
                    onMaxChanged: root.cMaxReuseTimesMax = val
                }

                // hMaxRequestTimes
                CaptionTextType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 16
                    Layout.bottomMargin: 8
                    text: qsTr("hMaxRequestTimes")
                    color: AmneziaStyle.color.mutedGray
                }
                MinMaxRowType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    minValue: root.hMaxRequestTimesMin
                    maxValue: root.hMaxRequestTimesMax
                    onMinChanged: root.hMaxRequestTimesMin = val
                    onMaxChanged: root.hMaxRequestTimesMax = val
                }

                // hMaxReusableSecs
                CaptionTextType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 16
                    Layout.bottomMargin: 8
                    text: qsTr("hMaxReusableSecs")
                    color: AmneziaStyle.color.mutedGray
                }
                MinMaxRowType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    minValue: root.hMaxReusableSecsMin
                    maxValue: root.hMaxReusableSecsMax
                    onMinChanged: root.hMaxReusableSecsMin = val
                    onMaxChanged: root.hMaxReusableSecsMax = val
                }

                // hKeepAlivePeriod — single field
                TextFieldWithHeaderType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 16
                    headerText: qsTr("hKeepAlivePeriod")
                    textField.text: root.hKeepAlivePeriod
                    textField.validator: IntValidator {
                        bottom: 0
                    }
                    textField.onEditingFinished: root.hKeepAlivePeriod = textField.text
                }
            }

            Item {
                Layout.preferredHeight: 16
            }
        }
    }

    // ── Save button ───────────────────────────────────────────────────
    BasicButtonType {
        id: saveButton
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottomMargin: 16 + PageController.safeAreaBottomMargin
        anchors.leftMargin: 16
        anchors.rightMargin: 16

        text: qsTr("Save")
        onClicked: {
            forceActiveFocus()
            // XrayConfigModel.setXmux(...)
        }
        Keys.onEnterPressed: clicked()
        Keys.onReturnPressed: clicked()
    }
}

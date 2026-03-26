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

        model: XrayConfigModel

        delegate: ColumnLayout {
            width: listView.width
            spacing: 0

            Header2TextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 0
                Layout.bottomMargin: 24
                text: qsTr("xmux")
            }

            SwitcherType {
                Layout.fillWidth: true
                Layout.margins: 16
                text: qsTr("xmux")
                checked: xmuxEnabled
                onToggled: xmuxEnabled = checked
            }

            DividerType {
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0
                enabled: xmuxEnabled

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
                    minValue: xmuxMaxConcurrencyMin
                    maxValue: xmuxMaxConcurrencyMax
                    onMinChanged: xmuxMaxConcurrencyMin = val
                    onMaxChanged: xmuxMaxConcurrencyMax = val
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
                    minValue: xmuxMaxConnectionsMin
                    maxValue: xmuxMaxConnectionsMax
                    onMinChanged: xmuxMaxConnectionsMin = val
                    onMaxChanged: xmuxMaxConnectionsMax = val
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
                    minValue: xmuxCMaxReuseTimesMin
                    maxValue: xmuxCMaxReuseTimesMax
                    onMinChanged: xmuxCMaxReuseTimesMin = val
                    onMaxChanged: xmuxCMaxReuseTimesMax = val
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
                    minValue: xmuxHMaxRequestTimesMin
                    maxValue: xmuxHMaxRequestTimesMax
                    onMinChanged: xmuxHMaxRequestTimesMin = val
                    onMaxChanged: xmuxHMaxRequestTimesMax = val
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
                    minValue: xmuxHMaxReusableSecsMin
                    maxValue: xmuxHMaxReusableSecsMax
                    onMinChanged: xmuxHMaxReusableSecsMin = val
                    onMaxChanged: xmuxHMaxReusableSecsMax = val
                }

                TextFieldWithHeaderType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 16
                    headerText: qsTr("hKeepAlivePeriod")
                    textField.text: xmuxHKeepAlivePeriod
                    textField.validator: IntValidator {
                        bottom: 0
                    }
                    textField.onEditingFinished: {
                        if (textField.text !== xmuxHKeepAlivePeriod) xmuxHKeepAlivePeriod = textField.text
                    }
                }
            }

            Item {
                Layout.preferredHeight: 16
            }
        }
    }

    //     BasicButtonType {
    //         id: saveButton
    //         anchors.bottom: parent.bottom
    //         anchors.left: parent.left
    //         anchors.right: parent.right
    //         anchors.bottomMargin: 16 + PageController.safeAreaBottomMargin
    //         anchors.leftMargin: 16
    //         anchors.rightMargin: 16
    //         text: qsTr("Save")
    //         onClicked: {
    //             forceActiveFocus()
    //             PageController.closePage()
    //         }
    //         Keys.onEnterPressed: clicked()
    //         Keys.onReturnPressed: clicked()
    //     }
}


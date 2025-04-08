pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

import Config 1.0

import "../Controls"
import "../Controls/TextTypes"

Popup {
    id: root

    property string title: ""
    property string description: ""
    property string confirmButtonText: qsTr("Confirm")
    property string cancelButtonText: qsTr("Cancel")
    property var onConfirm
    property var onCancel

    modal: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    anchors.centerIn: parent
    width: parent.width - 30
    height: 310
    padding: 24

    background: Rectangle {
        color: Style.color.white
        radius: 20
        border.width: 1
        border.color: Style.color.gray2
    }

    contentItem: ColumnLayout {
        spacing: 40
        width: parent.width

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 12

            Header3TextType {
                Layout.fillWidth: true
                text: root.title
                horizontalAlignment: Text.AlignLeft
            }

            MediumTextType {
                Layout.fillWidth: true
                text: root.description
                visible: description !== ""
                horizontalAlignment: Text.AlignLeft
                wrapMode: Text.WordWrap
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8

            BlueButtonNoBorder {
                Layout.fillWidth: true
                text: root.cancelButtonText
                onClicked: {
                    if (root.onCancel) {
                        root.onCancel()
                    }
                    root.close()
                }
            }

            WhiteButtonWithBorder {
                Layout.fillWidth: true
                text: root.confirmButtonText
                onClicked: {
                    if (root.onConfirm) {
                        root.onConfirm()
                    }
                    root.close()
                }
            }
        }
    }

    Overlay.modal: Item {
        anchors.fill: parent

        ShaderEffectSource {
            id: blurSource
            anchors.fill: parent
            sourceItem: root.parent
        }

        GaussianBlur {
            anchors.fill: parent
            source: blurSource
            radius: 4
        }

        Rectangle {
            anchors.fill: parent
            color: Style.color.transparentWhite
        }
    }
} 
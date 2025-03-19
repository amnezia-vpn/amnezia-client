pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

import Config 1.0

import "TextTypes"

Button {
    id: root

    property string defaultBackgroundColor: "#FFFFFF"
    property string defaultBorderColor: "#D1D1D6"
    property string defaultTextColor: "#000000"
    property string defaultImageColor: "#000000"

    property string hoveredBackgroundColor: "#FFFFFF"
    property string hoveredBorderColor: "#D1D1D6"
    property string hoveredTextColor: "#D1D1D6"
    property string hoveredImageColor: "#D1D1D6"

    property string pressedBackgroundColor: "#FFFFFF"
    property string pressedBorderColor: "#D1D1D6"
    property string pressedTextColor: "#D1D1D6"
    property string pressedImageColor: "#D1D1D6"

    property string disabledBackgroundColor: "#FFFFFF"
    property string disabledBorderColor: "#D1D1D6"
    property string disabledTextColor: "#D1D1D6"
    property string disabledImageColor: "#D1D1D6"

    property string imageSource: "qrc:/images/controls/chevron-down.svg"

    hoverEnabled: true

    background: Rectangle {
        id: focusBorder

        color: root.defaultBackgroundColor
        border.color: root.defaultBorderColor
        border.width: 1

        anchors.fill: parent

        radius: 6
    }

    MouseArea {
        anchors.fill: focusBorder
        enabled: false
        cursorShape: Qt.PointingHandCursor
    }

    contentItem: Item {
        anchors.fill: focusBorder

        implicitWidth: content.implicitWidth
        implicitHeight: content.implicitHeight

        RowLayout {
            id: content
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16

            MediumTextType {
                id: buttonText

                Layout.fillWidth: true
                Layout.topMargin: 12
                Layout.bottomMargin: 12

                color: root.defaultTextColor
                text: root.text

                horizontalAlignment: Qt.AlignLeft
                verticalAlignment: Qt.AlignVCenter
            }

            Image {
                Layout.preferredHeight: 22
                Layout.preferredWidth: 22

                source: root.imageSource
                visible: root.imageSource === "" ? false : true

                layer {
                    enabled: true
                    effect: ColorOverlay {
                        color: root.defaultImageColor
                    }
                }
            }
        }
    }
}

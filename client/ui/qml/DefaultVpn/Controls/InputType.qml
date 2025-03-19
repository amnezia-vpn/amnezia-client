pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

import Config 1.0

import "TextTypes"

TextField {
    id: root

    property string defaultBackgroundColor: Style.color.white
    property string defaultBorderColor: Style.color.gray3
    property string defaultTextColor: Style.color.gray6

    property string hoveredBackgroundColor: Style.color.white
    property string hoveredBorderColor: Style.color.gray6
    property string hoveredTextColor: Style.color.black

    property string disabledBackgroundColor: Style.color.gray2
    property string disabledBorderColor: Style.color.gray3
    property string disabledTextColor: Style.color.gray9


    color: root.enabled ? root.defaultTextColor : (root.hovered || root.pressed) ? root.hoveredTextColor : root.disabledTextColor
    background: Rectangle {
        anchors.fill: parent

        color: root.enabled ? root.defaultBackgroundColor : (root.hovered || root.pressed) ? root.hoveredBackgroundColor : root.disabledBackgroundColor
        border.color: root.enabled ? root.defaultBorderColor : (root.hovered || root.pressed) ? root.hoveredBorderColor : root.disabledBorderColor
        border.width: 1
        radius: 6
    }

    topPadding: 12
    bottomPadding: 12
    leftPadding: 16
    rightPadding: 16

    inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhSensitiveData | Qt.ImhNoPredictiveText

    selectionColor:  Style.color.accent1
    selectedTextColor: Style.color.white

    font.pixelSize: 17
    font.weight: 400
    font.family: Style.font

    wrapMode: TextEdit.Wrap

    verticalAlignment: Text.AlignTop

}

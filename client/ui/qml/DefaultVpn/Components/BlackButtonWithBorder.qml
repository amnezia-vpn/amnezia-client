pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

import Config 1.0

import "../Controls/TextTypes"
import "../Controls"

ButtonType {
    defaultBackgroundColor: Style.color.black
    defaultBorderColor: Style.color.gray7
    defaultTextColor: Style.color.white
    defaultImageColor: Style.color.white

    hoveredBackgroundColor: Style.color.black
    hoveredBorderColor: Style.color.gray5
    hoveredTextColor: Style.color.white
    hoveredImageColor: Style.color.white

    pressedBackgroundColor: Style.color.gray9
    pressedBorderColor: Style.color.gray5
    pressedTextColor: Style.color.white
    pressedImageColor: Style.color.white

    disabledBackgroundColor: Style.color.gray8
    disabledBorderColor: Style.color.gray9
    disabledTextColor: Style.color.gray2
    disabledImageColor: Style.color.gray2

    defaultBorderWidth: 1
    disabledBorderWidth: 1
    hoveredBorderWidth: 1
}

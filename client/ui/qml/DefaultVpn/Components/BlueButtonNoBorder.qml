pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

import Config 1.0

import "../Controls/TextTypes"
import "../Controls"

ButtonType {
    defaultBackgroundColor: Style.color.accent1
    defaultBorderColor: Style.color.gray3
    defaultTextColor: Style.color.white
    defaultImageColor: Style.color.white

    hoveredBackgroundColor: Style.color.accent2
    hoveredBorderColor: Style.color.gray3
    hoveredTextColor: Style.color.white
    hoveredImageColor: Style.color.white

    pressedBackgroundColor: Style.color.accent3
    pressedBorderColor: Style.color.gray3
    pressedTextColor: Style.color.white
    pressedImageColor: Style.color.white

    disabledBackgroundColor: Style.color.gray6
    disabledBorderColor: Style.color.gray3
    disabledTextColor: Style.color.gray2
    disabledImageColor: Style.color.gray2

    defaultBorderWidth: 0
    disabledBorderWidth: 0
}

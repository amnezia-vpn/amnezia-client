pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

import Config 1.0

import "../Controls/TextTypes"
import "../Controls"

ButtonType {
    defaultBackgroundColor: Style.color.white
    defaultBorderColor: Style.color.gray3
    defaultTextColor: Style.color.black
    defaultImageColor: Style.color.black

    hoveredBackgroundColor: Style.color.white
    hoveredBorderColor: Style.color.gray6
    hoveredTextColor: Style.color.black
    hoveredImageColor: Style.color.black

    pressedBackgroundColor: Style.color.gray1
    pressedBorderColor: Style.color.gray6
    pressedTextColor: Style.color.black
    pressedImageColor: Style.color.black

    disabledBackgroundColor: Style.color.gray3
    disabledBorderColor: Style.color.gray2
    disabledTextColor: Style.color.gray9
    disabledImageColor: Style.color.gray9

    defaultBorderWidth: 1
    disabledBorderWidth: 1
    hoveredBorderWidth: 1
}

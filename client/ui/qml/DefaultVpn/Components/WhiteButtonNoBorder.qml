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
    defaultTextColor: Style.color.accent1
    defaultImageColor: Style.color.accent1

    hoveredBackgroundColor: Style.color.gray1
    hoveredBorderColor: Style.color.gray3
    hoveredTextColor: Style.color.accent2
    hoveredImageColor: Style.color.accent2

    pressedBackgroundColor: Style.color.gray2
    pressedBorderColor: Style.color.gray3
    pressedTextColor: Style.color.accent3
    pressedImageColor: Style.color.accent3

    disabledBackgroundColor: Style.color.white
    disabledBorderColor: Style.color.gray3
    disabledTextColor: Style.color.gray8
    disabledImageColor: Style.color.gray8

    defaultBorderWidth: 0
    disabledBorderWidth: 0
}

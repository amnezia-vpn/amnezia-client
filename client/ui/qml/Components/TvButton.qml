import QtQuick
import QtQuick.Controls

Button {
    id: control

    // Style knobs
    property int tvFontPixelSize: 30
    property int tvCornerRadius: 18
    property color normalColor: "#1A1A1D"
    property color focusedColor: "#3F2E07"
    property color pressedColor: "#5C3F08"
    property color disabledColor: "#111114"
    property color normalBorderColor: "#3F3F46"
    property color focusedBorderColor: "#FACC15"
    property color pressedBorderColor: "#FACC15"
    property color textColor: "#F8FAFC"
    property color disabledTextColor: "#71717A"

    focusPolicy: Qt.StrongFocus
    activeFocusOnTab: true

    leftPadding: 16
    rightPadding: 16
    topPadding: 0
    bottomPadding: 0
    spacing: 0

    font.pixelSize: tvFontPixelSize
    font.bold: activeFocus

    // Subtle focus pop so user always sees what is selected on TV.
    scale: control.activeFocus ? 1.04 : 1.0
    Behavior on scale {
        NumberAnimation { duration: 120; easing.type: Easing.OutQuad }
    }

    background: Rectangle {
        radius: control.tvCornerRadius
        color: !control.enabled
                ? control.disabledColor
                : control.pressed
                    ? control.pressedColor
                    : control.activeFocus
                        ? control.focusedColor
                        : control.normalColor
        border.width: control.activeFocus || control.pressed ? 3 : 1
        border.color: !control.enabled
                ? control.normalBorderColor
                : control.pressed
                    ? control.pressedBorderColor
                    : control.activeFocus
                        ? control.focusedBorderColor
                        : control.normalBorderColor

        Behavior on color {
            ColorAnimation { duration: 120 }
        }
        Behavior on border.color {
            ColorAnimation { duration: 120 }
        }
    }

    contentItem: Text {
        text: control.text
        color: control.enabled ? control.textColor : control.disabledTextColor
        font: control.font
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        maximumLineCount: 1
    }
}

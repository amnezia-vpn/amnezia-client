import QtQuick
import QtQuick.Controls

Button {
    id: control

    property int tvFontPixelSize: 30
    property color normalColor: "#18181B"
    property color focusedColor: "#2A2104"
    property color disabledColor: "#111114"
    property color normalBorderColor: "#3F3F46"
    property color focusedBorderColor: "#FACC15"
    property color textColor: "#F8FAFC"
    property color disabledTextColor: "#71717A"

    focusPolicy: Qt.StrongFocus
    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    bottomPadding: 0
    spacing: 0
    font.pixelSize: tvFontPixelSize
    font.bold: activeFocus

    background: Rectangle {
        radius: 18
        color: !control.enabled ? control.disabledColor
              : control.activeFocus ? control.focusedColor
              : control.normalColor
        border.width: control.activeFocus ? 3 : 1
        border.color: control.activeFocus ? control.focusedBorderColor : control.normalBorderColor
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

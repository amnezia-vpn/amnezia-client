import QtQuick

import Style 1.0

// Focus affordance drawn over a control. Off a TV it stays the hairline outline
// the desktop UI has always used; on a TV it becomes a thick accent outline with
// a tinted fill so the selected control is readable from across the room.
//
// Drop it in as the last child of the control's background and point `control`
// at the item whose activeFocus should drive it.
Rectangle {
    id: indicator

    property Item control: parent
    property int baseRadius: 16
    // Overridable so a container can light up while focus sits on a small child
    // of its own, e.g. the chevron inside a settings row.
    property bool active: control ? control.activeFocus : false

    // stays inside the control's own bounds so a surrounding Flickable or
    // ListView with clip: true cannot cut the outline off
    anchors.fill: parent
    radius: baseRadius
    z: 100

    visible: opacity > 0
    opacity: active ? 1 : 0

    color: AmneziaStyle.focus.overlayColor
    border.color: AmneziaStyle.focus.borderColor
    border.width: AmneziaStyle.focus.borderWidth

    Behavior on opacity {
        PropertyAnimation { duration: 200 }
    }
}

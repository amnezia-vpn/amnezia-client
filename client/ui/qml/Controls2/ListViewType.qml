import QtQuick
import QtQuick.Controls

ListView {
    id: root

    ScrollBar.vertical: ScrollBarType {}

    clip: true

    interactive: root.orientation === ListView.Vertical
                 ? root.contentHeight + root.topMargin + root.bottomMargin > root.height
                 : root.contentWidth + root.leftMargin + root.rightMargin > root.width
}

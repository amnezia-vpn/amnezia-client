import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

Control {
    id: root

    property var model: []
    property int currentIndex: 0
    property int size: SegmentType.Large
    property bool fill: root.size === SegmentType.Large

    signal activated(int index)

    function syncChecked() {
        for (let i = 0; i < repeater.count; ++i) {
            const item = repeater.itemAt(i)
            if (item) {
                item.checked = i === root.currentIndex
            }
        }
    }

    onCurrentIndexChanged: root.syncChecked()

    padding: 0

    background: Rectangle {
        radius: 16
        color: AmneziaStyle.color.bgControlTrack
    }

    contentItem: RowLayout {
        spacing: 0

        Repeater {
            id: repeater

            model: root.model

            onItemAdded: root.syncChecked()

            SegmentType {
                required property int index
                required property var modelData

                Layout.fillWidth: root.fill
                Layout.preferredWidth: root.fill ? 1 : implicitWidth

                size: root.size
                text: modelData
                ButtonGroup.group: group

                onClicked: {
                    root.currentIndex = index
                    root.activated(index)
                }
            }
        }
    }

    ButtonGroup {
        id: group
    }
}

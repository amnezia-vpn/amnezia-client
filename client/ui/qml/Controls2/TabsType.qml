import QtQuick
import QtQuick.Controls

TabBar {
    id: root

    readonly property int visibleTabCount: {
        let visibleCount = 0
        for (let i = 0; i < root.count; ++i) {
            const tab = root.itemAt(i)
            if (tab && tab.visible) {
                ++visibleCount
            }
        }
        return visibleCount
    }

    spacing: 0
    background: null
}

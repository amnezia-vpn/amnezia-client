import QtQuick

import "../Controls2"

CountrySectionHeader {
    id: root

    required property ListView listView
    required property CollapsingHeaderType header

    property int revision: 0

    readonly property real visibleTop: root.listView.contentY + root.header.listOcclusion

    readonly property int topRow: {
        root.revision
        return root.listView.indexAt(root.listView.width / 2, root.visibleTop + 1)
    }

    readonly property real pushOffset: {
        root.revision
        const probe = root.listView.indexAt(root.listView.width / 2, root.visibleTop + root.height)
        if (probe < 0 || probe === root.topRow || !root.listModel.isSectionHeaderRow(probe)) {
            return 0
        }
        const next = root.listView.itemAtIndex(probe)
        if (!next) {
            return 0
        }
        return Math.min(0, next.y - root.visibleTop - root.height)
    }

    anchors.top: parent.top
    anchors.topMargin: root.header.pinnedBottom + root.pushOffset
    anchors.left: parent.left
    anchors.right: parent.right

    isPinnedOverlay: true
    sectionKey: root.topRow >= 0 ? root.listModel.sectionKeyAtRow(root.topRow) : ""
    row: {
        root.revision
        return root.sectionKey !== "" ? root.listModel.rowForSectionHeader(root.sectionKey) : -1
    }

    visible: root.listModel.isGrouped
             && root.listModel.hasResults
             && root.header.pinnedStuck
             && root.sectionKey !== ""

    onToggled: function(sectionKey) {
        Qt.callLater(function() {
            const headerRow = root.listModel.rowForSectionHeader(sectionKey)
            if (headerRow >= 0) {
                root.listView.positionViewAtIndex(headerRow, ListView.Beginning)
                root.listView.contentY = root.header.clampContentY(root.listView.contentY - root.header.pinnedHeight)
            }
        })
    }
}

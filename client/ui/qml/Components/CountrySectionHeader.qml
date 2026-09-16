import QtQuick
import QtQuick.Layouts

import Style 1.0

import "../Config"
import "../Controls2/TextTypes"

Item {
    id: root

    property var listModel: null

    property string sectionKey: ""
    property bool isPinnedOverlay: false

    readonly property bool isRealSection: root.listModel !== null && root.sectionKey !== ""

    visible: root.isRealSection

    implicitHeight: root.isRealSection ? 60 : 0

    readonly property string regionId: root.isRealSection ? root.listModel.sectionRegionId(root.sectionKey) : ""
    readonly property string subregionId: root.isRealSection ? root.listModel.sectionSubregionId(root.sectionKey) : ""

    readonly property bool collapsed: {
        if (!root.isRealSection) {
            return false
        }
        root.listModel.collapsedRevision
        return root.listModel.isSectionCollapsed(root.sectionKey)
    }

    readonly property string title: root.subregionId !== ""
                                    ? (CountryRegionNames.subregionNames[root.subregionId] || root.subregionId)
                                    : (CountryRegionNames.regionNames[root.regionId] || root.regionId)

    Rectangle {
        anchors.fill: parent
        color: root.isPinnedOverlay ? AmneziaStyle.color.midnightBlack
                                    : AmneziaStyle.color.transparent
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.topMargin: 24
        anchors.bottomMargin: 12
        spacing: 8

        ParagraphTextType {
            Layout.fillWidth: true

            color: AmneziaStyle.color.textTertiary
            font.letterSpacing: -0.4
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter

            text: root.collapsed
                  ? "%1 (%2)".arg(root.title).arg(root.listModel.sectionCount(root.sectionKey))
                  : root.title
        }

        Image {
            Layout.preferredWidth: 24
            Layout.preferredHeight: 24

            source: root.collapsed ? "qrc:/images/controls/chevron-down.svg"
                                   : "qrc:/images/controls/chevron-up.svg"
        }
    }

    HoverHandler {
        cursorShape: Qt.PointingHandCursor
    }

    TapHandler {
        gesturePolicy: TapHandler.ReleaseWithinBounds

        onTapped: root.listModel.toggleSection(root.sectionKey)
    }
}

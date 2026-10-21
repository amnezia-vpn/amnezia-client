import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.impl

import Style 1.0

import "../Config"
import "../Controls2"
import "../Controls2/TextTypes"

Item {
    id: root

    property var listModel: null

    activeFocusOnTab: root.isRealSection && !root.isPinnedOverlay

    property string sectionKey: ""
    property bool isPinnedOverlay: false
    property int row: -1

    readonly property bool canToggle: root.isRealSection && !root.listModel.isSearchActive

    signal toggled(string sectionKey)

    readonly property bool isRealSection: root.listModel !== null && root.sectionKey !== ""

    visible: root.isRealSection

    readonly property bool isDirectlyUnderParent: {
        if (!root.isRealSection || root.row <= 0) {
            return false
        }
        root.listModel.layoutRevision
        if (!root.listModel.isSectionHeaderRow(root.row - 1)) {
            return false
        }
        return root.listModel.sectionKeyAtRow(root.row - 1).split("/").length < root.sectionKey.split("/").length
    }

    readonly property bool isDirectlyAboveChild: {
        if (!root.isRealSection || root.row < 0) {
            return false
        }
        root.listModel.layoutRevision
        if (!root.listModel.isSectionHeaderRow(root.row + 1)) {
            return false
        }
        return root.listModel.sectionKeyAtRow(root.row + 1).split("/").length > root.sectionKey.split("/").length
    }

    readonly property int contentTopMargin: root.isDirectlyUnderParent ? 0 : (root.level === 1 ? 24 : 32)
    readonly property int contentBottomMargin: {
        if (root.collapsed || root.isDirectlyAboveChild) {
            return 0
        }
        return root.level === 1 ? 4 : 12
    }

    implicitHeight: root.isRealSection ? root.contentTopMargin + 32 + root.contentBottomMargin : 0

    readonly property string regionId: root.isRealSection ? root.listModel.sectionRegionId(root.sectionKey) : ""
    readonly property string subregionId: root.isRealSection ? root.listModel.sectionSubregionId(root.sectionKey) : ""
    readonly property string subsubregionId: root.isRealSection ? root.listModel.sectionSubsubregionId(root.sectionKey) : ""

    readonly property int level: root.subsubregionId !== "" ? 3 : (root.subregionId !== "" ? 2 : 1)
    readonly property color chevronColor: root.level === 1 ? AmneziaStyle.color.iconDefault
                                                         : AmneziaStyle.color.iconSecondary

    readonly property string displayTitle: {
        if (!root.isRealSection) {
            return ""
        }
        root.listModel.layoutRevision
        return "%1 · %2".arg(root.title).arg(root.listModel.sectionCount(root.sectionKey))
    }

    readonly property bool collapsed: {
        if (!root.isRealSection) {
            return false
        }
        root.listModel.collapsedRevision
        return root.listModel.isSectionCollapsed(root.sectionKey)
    }

    readonly property string title: {
        if (root.subsubregionId !== "") {
            return CountryRegionNames.subsubregionNames[root.subsubregionId] || root.subsubregionId
        }
        if (root.subregionId !== "") {
            return CountryRegionNames.subregionNames[root.subregionId] || root.subregionId
        }
        return CountryRegionNames.regionNames[root.regionId] || root.regionId
    }

    Rectangle {
        anchors.fill: parent
        color: root.isPinnedOverlay ? AmneziaStyle.color.bgBase
                                    : AmneziaStyle.color.transparent
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: root.level === 3 ? 32 : 16
        anchors.rightMargin: 20
        anchors.topMargin: root.contentTopMargin
        anchors.bottomMargin: root.contentBottomMargin
        spacing: 8

        BodyTextType {
            Layout.fillWidth: true

            visible: root.level === 1
            color: AmneziaStyle.color.textPrimary
            font.weight: 600
            text: root.displayTitle
        }

        SmallTextType {
            Layout.fillWidth: true

            visible: root.level !== 1
            text: root.displayTitle
        }

        IconImage {
            Layout.preferredWidth: 24
            Layout.preferredHeight: 24
            Layout.leftMargin: 10

            visible: root.canToggle
            source: root.collapsed ? "qrc:/images/controls/chevron-down.svg"
                                   : "qrc:/images/controls/chevron-up.svg"
            sourceSize: Qt.size(24, 24)
            color: root.chevronColor
        }
    }

    HoverHandler {
        cursorShape: root.canToggle ? Qt.PointingHandCursor : Qt.ArrowCursor
    }

    TapHandler {
        gesturePolicy: TapHandler.ReleaseWithinBounds

        onTapped: root.toggle()
    }

    function toggle() {
        if (root.canToggle) {
            root.listModel.toggleSection(root.sectionKey)
            root.toggled(root.sectionKey)
        }
    }

    Keys.onEnterPressed: root.toggle()
    Keys.onReturnPressed: root.toggle()
    Keys.onSpacePressed: root.toggle()

    FocusRingType {
        anchors.margins: 0
        radius: 8
        visible: root.activeFocus
    }
}

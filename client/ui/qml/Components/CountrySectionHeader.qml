import QtQuick
import QtQuick.Layouts

import Style 1.0

import "../Config"
import "../Controls2/TextTypes"

Item {
    id: root

    property var listModel: null

    property bool isFocusable: root.isRealSection && !root.isPinnedOverlay

    property string sectionKey: ""
    property bool isPinnedOverlay: false

    readonly property bool canToggle: root.isRealSection && !root.listModel.isSearchActive

    signal toggled(string sectionKey)

    readonly property bool isRealSection: root.listModel !== null && root.sectionKey !== ""

    visible: root.isRealSection

    implicitHeight: root.isRealSection ? 60 : 0

    readonly property string regionId: root.isRealSection ? root.listModel.sectionRegionId(root.sectionKey) : ""
    readonly property string subregionId: root.isRealSection ? root.listModel.sectionSubregionId(root.sectionKey) : ""
    readonly property string subsubregionId: root.isRealSection ? root.listModel.sectionSubsubregionId(root.sectionKey) : ""

    readonly property int level: root.subsubregionId !== "" ? 3 : (root.subregionId !== "" ? 2 : 1)

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
        color: root.isPinnedOverlay ? AmneziaStyle.color.midnightBlack
                                    : AmneziaStyle.color.transparent
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: root.level === 3 ? 32 : 16
        anchors.rightMargin: 16
        anchors.topMargin: 24
        anchors.bottomMargin: 12
        spacing: 8

        ParagraphTextType {
            Layout.fillWidth: true

            color: root.level === 1 ? AmneziaStyle.color.textPrimary
                                    : AmneziaStyle.color.textTertiary
            font.pixelSize: root.level === 1 ? 18 : (root.level === 2 ? 16 : 14)
            font.weight: root.level === 1 ? 700 : 400
            font.letterSpacing: -0.4
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter

            text: {
                if (!root.isRealSection) {
                    return ""
                }
                root.listModel.layoutRevision
                return "%1 · %2".arg(root.title).arg(root.listModel.sectionCount(root.sectionKey))
            }
        }

        Image {
            Layout.preferredWidth: 24
            Layout.preferredHeight: 24

            visible: root.canToggle
            source: root.collapsed ? "qrc:/images/controls/chevron-down.svg"
                                   : "qrc:/images/controls/chevron-up.svg"
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

    Keys.onTabPressed: {
        FocusController.nextKeyTabItem()
    }

    Keys.onBacktabPressed: {
        FocusController.previousKeyTabItem()
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 2

        color: AmneziaStyle.color.transparent
        radius: 8

        border.width: root.activeFocus ? 1 : 0
        border.color: AmneziaStyle.color.paleGray
    }
}

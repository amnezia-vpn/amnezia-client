import QtQuick
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

import Style 1.0

import "../Config"
import "../Controls2/TextTypes"

Item {
    id: root

    property var listModel: null

    property bool isFocusable: root.isRealSection && !root.isPinnedOverlay

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

    readonly property int contentTopMargin: root.isDirectlyUnderParent ? 0 : 24
    readonly property int contentBottomMargin: root.collapsed ? 0 : (root.level === 1 ? 4 : 8)

    implicitHeight: root.isRealSection ? root.contentTopMargin + 32 + root.contentBottomMargin : 0

    readonly property string regionId: root.isRealSection ? root.listModel.sectionRegionId(root.sectionKey) : ""
    readonly property string subregionId: root.isRealSection ? root.listModel.sectionSubregionId(root.sectionKey) : ""
    readonly property string subsubregionId: root.isRealSection ? root.listModel.sectionSubsubregionId(root.sectionKey) : ""

    readonly property int level: root.subsubregionId !== "" ? 3 : (root.subregionId !== "" ? 2 : 1)
    readonly property color titleColor: root.level === 1 ? AmneziaStyle.color.textPrimary
                                                         : AmneziaStyle.color.textTertiary

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
        anchors.rightMargin: 20
        anchors.topMargin: root.contentTopMargin
        anchors.bottomMargin: root.contentBottomMargin
        spacing: 8

        ParagraphTextType {
            Layout.fillWidth: true

            color: root.titleColor
            lineHeight: (root.level === 1 ? 24 : 18) + LanguageUiController.getLineHeightAppend()
            font.pixelSize: root.level === 1 ? 16 : 14
            font.weight: root.level === 1 ? 700 : 400
            font.letterSpacing: root.level === 1 ? -0.4 : 0
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter

            text: {
                if (!root.isRealSection) {
                    return ""
                }
                root.listModel.layoutRevision
                if (!root.collapsed) {
                    return root.title
                }
                return "%1 (%2)".arg(root.title).arg(root.listModel.sectionCount(root.sectionKey))
            }
        }

        Image {
            Layout.preferredWidth: 24
            Layout.preferredHeight: 24
            Layout.leftMargin: 10

            visible: root.canToggle
            source: root.collapsed ? "qrc:/images/controls/chevron-down.svg"
                                   : "qrc:/images/controls/chevron-up.svg"

            layer {
                enabled: true
                effect: ColorOverlay {
                    color: root.titleColor
                }
            }
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

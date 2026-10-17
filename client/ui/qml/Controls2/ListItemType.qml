import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

import Style 1.0

ItemDelegate {
    id: root

    property string overline
    property string subtitle
    property color subtitleColor: AmneziaStyle.color.textTertiary
    property string trailingIconSource
    property string flagCode
    property int maximumLineCount: 2
    property bool destructive: false
    property bool locked: false
    property bool showSeparator: true
    property bool interactive: true
    property bool dense: false
    property bool trailingContentVisible: trailingSlot.children.length > 0
    property color separatorColor: AmneziaStyle.color.borderSoft
    property int leadingSlotWidth: 48
    property bool showIconBadge: false

    readonly property bool rowHovered: root.interactive && root.enabled && root.hovered && !trailingHover.hovered
    readonly property bool rowPressed: root.interactive && root.enabled && root.down

    default property alias trailingContent: trailingSlot.data

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(root.dense ? 72 : 56, implicitContentHeight + topPadding + bottomPadding)

    padding: 12
    verticalPadding: 4
    spacing: 12

    hoverEnabled: root.interactive
    focusPolicy: root.interactive ? Qt.TabFocus : Qt.NoFocus
    opacity: enabled ? 1.0 : 0.4

    icon.width: 24
    icon.height: 24
    icon.color: AmneziaStyle.color.iconDefault

    Accessible.name: root.text
    Accessible.description: root.locked ? qsTr("Available in Premium") : root.subtitle

    Keys.onEnterPressed: root.click()
    Keys.onReturnPressed: root.click()

    background: ListItemBackgroundType {
        control: root.interactive ? root : null
        showSeparator: root.showSeparator
        separatorColor: root.separatorColor
    }

    contentItem: RowLayout {
        spacing: root.spacing

        Item {
            Layout.preferredWidth: root.leadingSlotWidth
            Layout.preferredHeight: 48

            visible: root.icon.source.toString() !== ""

            IconImage {
                anchors.centerIn: parent
                width: root.icon.width
                height: root.icon.height

                source: root.icon.source
                sourceSize: Qt.size(root.icon.width, root.icon.height)
                color: root.icon.color

                DotBadgeType {
                    anchors.horizontalCenter: parent.right
                    anchors.verticalCenter: parent.top

                    visible: root.showIconBadge
                    color: AmneziaStyle.color.accentDefault
                }
            }
        }

        ListItemLabelType {
            Layout.fillWidth: true
            Layout.leftMargin: 4
            Layout.topMargin: 10
            Layout.bottomMargin: 10

            overline: root.overline
            dense: root.dense
            title: root.text
            subtitle: root.subtitle
            subtitleColor: root.subtitleColor
            maximumLineCount: root.maximumLineCount
            textOpacity: root.locked ? 0.65 : 1.0
            titleColor: {
                if (root.destructive) {
                    return AmneziaStyle.color.statusDestructive
                }
                if (root.locked) {
                    return AmneziaStyle.color.textSecondary
                }
                return AmneziaStyle.color.textPrimary
            }
        }

        Image {
            Layout.preferredWidth: 20
            Layout.preferredHeight: 15
            Layout.rightMargin: 4

            visible: root.flagCode !== ""
            opacity: root.locked ? 0.4 : 1.0
            source: root.flagCode !== "" ? "qrc:/countriesFlags/images/flagKit/" + root.flagCode + ".svg" : ""
            sourceSize: Qt.size(20, 15)
        }

        RowLayout {
            id: trailingSlot

            spacing: 0
            visible: root.trailingContentVisible

            HoverHandler {
                id: trailingHover
            }
        }

        Item {
            Layout.preferredWidth: 48
            Layout.preferredHeight: 48

            visible: root.trailingIconSource !== ""

            ListItemStateLayerType {
                anchors.centerIn: parent

                hovered: root.rowHovered
                pressed: root.rowPressed
            }

            IconImage {
                anchors.centerIn: parent
                width: 24
                height: 24

                source: root.trailingIconSource
                sourceSize: Qt.size(24, 24)
                color: root.destructive ? AmneziaStyle.color.statusDestructive : AmneziaStyle.color.iconDefault
            }
        }
    }
}

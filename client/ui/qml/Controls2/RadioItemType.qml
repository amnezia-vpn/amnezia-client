import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

import Style 1.0

RadioDelegate {
    id: root

    property string subtitle
    property color subtitleColor: AmneziaStyle.color.textTertiary
    property string flagCode
    property int maximumLineCount: 2
    property bool locked: false
    property bool showSeparator: true
    property color separatorColor: AmneziaStyle.color.borderSoft

    default property alias trailingContent: trailingSlot.data

    readonly property bool rowHovered: root.enabled && root.hovered && !trailingHover.hovered
    readonly property bool rowPressed: root.enabled && root.down

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(56, implicitContentHeight + topPadding + bottomPadding)

    padding: 12
    verticalPadding: 4
    leftPadding: 12 + 48 + spacing
    spacing: 12

    hoverEnabled: true
    focusPolicy: Qt.TabFocus
    opacity: enabled ? 1.0 : 0.4

    icon.width: 24
    icon.height: 24
    icon.color: AmneziaStyle.color.iconDefault

    Accessible.name: root.text
    Accessible.description: root.locked ? qsTr("Available in Premium") : root.subtitle

    Keys.onEnterPressed: root.click()
    Keys.onReturnPressed: root.click()

    background: ListItemBackgroundType {
        control: root
        showSeparator: root.showSeparator
        separatorColor: root.separatorColor
    }

    indicator: Item {
        x: root.leftPadding - width - root.spacing
        y: (root.height - height) / 2
        width: 48
        height: 48

        ListItemStateLayerType {
            anchors.centerIn: parent

            hovered: root.rowHovered
            pressed: root.rowPressed
        }

        RadioIndicatorType {
            anchors.centerIn: parent
            visible: root.icon.source.toString() === ""
            checked: root.checked
        }

        IconImage {
            anchors.centerIn: parent
            width: root.icon.width
            height: root.icon.height

            visible: root.icon.source.toString() !== ""
            source: root.icon.source
            sourceSize: Qt.size(root.icon.width, root.icon.height)
            color: root.icon.color
        }
    }

    contentItem: RowLayout {
        spacing: root.spacing

        ListItemLabelType {
            Layout.fillWidth: true
            Layout.leftMargin: 4
            Layout.topMargin: 10
            Layout.bottomMargin: 10

            title: root.text
            subtitle: root.subtitle
            subtitleColor: root.subtitleColor
            maximumLineCount: root.maximumLineCount
            textOpacity: root.locked ? 0.65 : 1.0
            titleColor: {
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
            visible: children.length > 0

            HoverHandler {
                id: trailingHover
            }
        }
    }
}

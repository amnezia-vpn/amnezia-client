import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "../Controls2"

ItemDelegate {
    id: root

    property bool favorite: false
    property string countryFlagCode
    property string subtitle
    property int maximumLineCount: 2
    property bool showSeparator: true

    default property alias trailingContent: trailingRow.data

    readonly property bool rowHovered: root.enabled && root.hovered && !favoriteButton.hovered && !trailingHover.hovered

    signal favoriteToggled()

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(56, implicitContentHeight + topPadding + bottomPadding)

    padding: 12
    verticalPadding: 4
    leftPadding: 16
    spacing: 12

    hoverEnabled: true
    focusPolicy: Qt.TabFocus
    opacity: enabled ? 1.0 : 0.4

    Accessible.name: root.text
    Accessible.description: root.subtitle

    Keys.onEnterPressed: root.click()
    Keys.onReturnPressed: root.click()

    background: Item {
        implicitHeight: 56

        HoverHandler {
            cursorShape: Qt.PointingHandCursor
            enabled: root.enabled
        }

        Rectangle {
            anchors.fill: parent
            anchors.topMargin: 4
            anchors.bottomMargin: 4

            radius: 16
            color: {
                if (root.enabled && root.down) {
                    return AmneziaStyle.color.bgStateLayerPressed
                }
                return root.rowHovered ? AmneziaStyle.color.bgStateLayer : AmneziaStyle.color.transparent
            }
            border.width: root.enabled && root.down || root.highlighted ? 1 : 0
            border.color: AmneziaStyle.color.borderBold

            Behavior on color {
                ColorAnimation { duration: 150 }
            }

            FocusRingType {
                radius: 16
                visible: root.visualFocus
            }
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom

            height: 1
            visible: root.showSeparator
            color: AmneziaStyle.color.borderSoft
        }
    }

    contentItem: RowLayout {
        spacing: root.spacing

        Item {
            Layout.preferredWidth: 24
            Layout.preferredHeight: 48

            visible: root.countryFlagCode !== ""

            Image {
                anchors.centerIn: parent
                width: 21
                height: 15

                source: root.countryFlagCode !== "" ? "qrc:/countriesFlags/images/flagKit/" + root.countryFlagCode + ".svg" : ""
                sourceSize: Qt.size(21, 15)
            }
        }

        ListItemLabelType {
            Layout.fillWidth: true
            Layout.leftMargin: 4
            Layout.topMargin: 10
            Layout.bottomMargin: 10

            title: root.text
            subtitle: root.subtitle
            maximumLineCount: root.maximumLineCount
        }

        FavoriteButton {
            id: favoriteButton

            favorite: root.favorite

            onClicked: root.favoriteToggled()
        }

        Row {
            id: trailingRow

            Layout.leftMargin: -root.spacing

            visible: trailingRow.children.length > 0

            HoverHandler {
                id: trailingHover
            }
        }
    }
}

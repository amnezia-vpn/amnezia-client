import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

import Style 1.0

import "TextTypes"

Item {
    id: root

    property string headerText
    property int headerTextFormat: Text.AutoText
    property int headerHorizontalAlignment: Text.AlignLeft

    property string descriptionText

    property string descriptionLinkText
    property string descriptionLinkUrl

    property string actionButtonImage
    property bool showSwitcher: false

    readonly property alias actionButton: actionButton
    readonly property alias switcher: switcher
    readonly property alias titleItem: titleText

    signal actionClicked()
    signal switcherToggled(bool checked)

    implicitWidth: content.implicitWidth
    implicitHeight: content.implicitHeight

    ColumnLayout {
        id: content

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        spacing: 0

        RowLayout {
            Layout.fillWidth: true

            spacing: 8

            H1TextType {
                id: titleText

                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter

                text: root.headerText
                textFormat: root.headerTextFormat
                horizontalAlignment: root.headerHorizontalAlignment
            }

            IconButtonType {
                id: actionButton

                Layout.alignment: Qt.AlignVCenter

                visible: root.actionButtonImage !== ""
                icon.source: root.actionButtonImage

                onClicked: root.actionClicked()
            }

            SwitchType {
                id: switcher

                Layout.alignment: Qt.AlignVCenter

                visible: root.showSwitcher

                onToggled: root.switcherToggled(switcher.checked)
            }
        }

        BodyTextType {
            Layout.fillWidth: true

            visible: root.descriptionText !== ""

            text: root.descriptionText
            color: AmneziaStyle.color.textSecondary
        }

        AbstractButton {
            id: descriptionLink

            Layout.fillWidth: true

            visible: root.descriptionLinkText !== "" && root.descriptionLinkUrl !== ""

            implicitHeight: linkRow.implicitHeight
            hoverEnabled: true

            text: root.descriptionLinkText

            Accessible.role: Accessible.Link

            background: null

            contentItem: Item {
                implicitHeight: linkRow.implicitHeight

                RowLayout {
                    id: linkRow

                    width: Math.min(implicitWidth, parent.width)

                    spacing: 8

                    BodyTextType {
                        Layout.fillWidth: true

                        text: descriptionLink.text
                        color: AmneziaStyle.color.textAccent
                        font.underline: descriptionLink.hovered
                    }

                    IconImage {
                        Layout.preferredWidth: 14
                        Layout.preferredHeight: 14

                        source: "qrc:/images/controls/external-link.svg"
                        sourceSize: Qt.size(14, 14)
                        color: AmneziaStyle.color.textAccent
                    }
                }

                FocusRingType {
                    target: linkRow
                    radius: 4
                    visible: descriptionLink.visualFocus
                }
            }

            HoverHandler {
                cursorShape: Qt.PointingHandCursor
            }

            onClicked: Qt.openUrlExternally(root.descriptionLinkUrl)

            Keys.onEnterPressed: descriptionLink.clicked()
            Keys.onReturnPressed: descriptionLink.clicked()
        }
    }
}

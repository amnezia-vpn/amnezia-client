import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "TextTypes"

Item {
    id: root

    enum Level { Title, Section }

    property string headerText
    property string descriptionText
    property int level: SectionHeaderType.Title

    property string actionButtonImage

    readonly property alias actionButton: actionButton

    signal actionClicked()

    implicitWidth: content.implicitWidth
    implicitHeight: content.implicitHeight

    ColumnLayout {
        id: content

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        spacing: root.level === SectionHeaderType.Title ? 8 : 4

        RowLayout {
            Layout.fillWidth: true

            spacing: 8

            H2TextType {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter

                visible: root.level === SectionHeaderType.Title

                text: root.headerText
            }

            H3TextType {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter

                visible: root.level === SectionHeaderType.Section

                text: root.headerText
            }

            IconButtonType {
                id: actionButton

                Layout.alignment: Qt.AlignVCenter

                size: root.level === SectionHeaderType.Title ? IconButtonType.Regular : IconButtonType.Small
                visible: root.actionButtonImage !== ""
                icon.source: root.actionButtonImage

                onClicked: root.actionClicked()
            }
        }

        BodyTextType {
            Layout.fillWidth: true

            visible: root.level === SectionHeaderType.Title && root.descriptionText !== ""

            text: root.descriptionText
            color: AmneziaStyle.color.textSecondary
        }

        CaptionTextType {
            Layout.fillWidth: true

            visible: root.level === SectionHeaderType.Section && root.descriptionText !== ""

            text: root.descriptionText
        }
    }
}

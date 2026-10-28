import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

import Style 1.0

import "TextTypes"
import "../Config"

AbstractButton {
    id: root

    enum Variant { Default, Primary }

    property int variant: DropDownType.Default

    property string descriptionText
    property string placeholderText

    property string headerText
    property string headerBackButtonImage

    property real drawerHeight: 0.9
    property bool fitContent: false
    property Component listView

    readonly property bool isPrimary: root.variant === DropDownType.Primary

    function open() {
        menu.open()
    }

    function close() {
        menu.close()
    }

    implicitWidth: implicitContentWidth + leftPadding + rightPadding
    implicitHeight: implicitContentHeight + topPadding + bottomPadding

    leftPadding: 16
    rightPadding: root.isPrimary ? 4 : 16
    topPadding: root.isPrimary ? 0 : 4
    bottomPadding: root.isPrimary ? 0 : 4

    hoverEnabled: true

    opacity: root.enabled ? 1 : 0.4

    onClicked: menu.open()

    background: Rectangle {
        radius: 16
        color: {
            if (!root.isPrimary) {
                return AmneziaStyle.color.bgSurface
            }
            if (root.down) {
                return AmneziaStyle.color.bgActionPress
            }
            return root.hovered ? AmneziaStyle.color.bgActionHover : AmneziaStyle.color.bgAction
        }

        Behavior on color {
            ColorAnimation { duration: 150 }
        }

        Rectangle {
            anchors.fill: parent

            radius: parent.radius
            visible: !root.isPrimary && root.enabled && (root.hovered || root.down)
            color: AmneziaStyle.color.bgStateLayer
        }

        FocusRingType {
            radius: 16
            visible: root.visualFocus
        }
    }

    contentItem: RowLayout {
        spacing: 8

        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter

            spacing: 4

            CaptionTextType {
                Layout.fillWidth: true

                visible: text !== ""
                text: root.descriptionText
            }

            H3TextType {
                Layout.fillWidth: true

                visible: !root.isPrimary
                text: root.text !== "" ? root.text : root.placeholderText
                color: root.text !== "" ? AmneziaStyle.color.textPrimary : AmneziaStyle.color.textQuaternary
                maximumLineCount: 2
                elide: Text.ElideRight
            }

            ButtonTextType {
                Layout.fillWidth: true

                visible: root.isPrimary
                text: root.text
                color: AmneziaStyle.color.textInverted
                maximumLineCount: 1
                elide: Text.ElideRight
            }
        }

        Item {
            Layout.preferredWidth: 48
            Layout.preferredHeight: 48
            Layout.alignment: Qt.AlignVCenter

            IconImage {
                anchors.centerIn: parent

                width: 24
                height: 24

                source: "qrc:/images/controls/chevron-down.svg"
                sourceSize: Qt.size(24, 24)
                color: root.isPrimary ? AmneziaStyle.color.iconInverted : AmneziaStyle.color.iconDefault
            }
        }
    }

    HoverHandler {
        cursorShape: Qt.PointingHandCursor
    }

    Keys.onEnterPressed: root.clicked()
    Keys.onReturnPressed: root.clicked()

    SheetType {
        id: menu

        readonly property real measuredContentHeight: container.fitHeight
        expandedHeight: (root.fitContent && measuredContentHeight > 0)
            ? Math.min(measuredContentHeight, windowHeight * root.drawerHeight)
            : windowHeight * root.drawerHeight

        Item {
            id: container
            anchors.fill: parent

            property real fitHeight: backButton.implicitHeight + titleLabel.implicitHeight
                                     + (listViewLoader.item ? listViewLoader.item.contentHeight : 0) + 48

            ColumnLayout {
                id: header

                anchors.fill: parent
                anchors.topMargin: 16

                BackButtonType {
                    id: backButton
                    backButtonImage: root.headerBackButtonImage
                    backButtonFunction: function() { menu.close() }
                }

                SectionHeaderType {
                    id: titleLabel
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.bottomMargin: 32
                    Layout.fillWidth: true

                    headerText: root.headerText
                }

                Loader {
                    id: listViewLoader
                    sourceComponent: root.listView

                    Layout.fillHeight: true
                    Layout.fillWidth: true
                }
            }
        }
    }
}

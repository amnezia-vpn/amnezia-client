import QtQuick
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

import Style 1.0

import "TextTypes"

Item {
    id: root

    default property alias trailingContent: trailingSlot.data

    property string text
    property string descriptionText

    property string leftImageSource
    property string rightImageSource

    property color textColor: AmneziaStyle.color.textPrimary
    property color leftImageColor: AmneziaStyle.color.surfaceInverse

    property bool showDivider: true

    property var clickedFunction

    activeFocusOnTab: true

    Keys.onEnterPressed: root.click()
    Keys.onReturnPressed: root.click()

    function click() {
        if (clickedFunction && typeof clickedFunction === "function") {
            clickedFunction()
        }
    }

    implicitHeight: Math.max(72, content.implicitHeight + content.anchors.topMargin + content.anchors.bottomMargin)

    Rectangle {
        id: background

        anchors.fill: parent
        anchors.topMargin: 4
        anchors.bottomMargin: 4

        radius: 16
        color: (mouseArea.pressed || mouseArea.containsMouse) ? AmneziaStyle.color.translucentWhite : AmneziaStyle.color.transparent
        border.color: (mouseArea.pressed || root.activeFocus) ? AmneziaStyle.color.textTertiary : AmneziaStyle.color.transparent
        border.width: (mouseArea.pressed || root.activeFocus) ? 1 : 0

        Behavior on color {
            PropertyAnimation { duration: 200 }
        }
    }

    MouseArea {
        id: mouseArea

        anchors.fill: parent

        enabled: root.enabled
        hoverEnabled: root.enabled
        cursorShape: Qt.PointingHandCursor

        onClicked: root.click()
    }

    RowLayout {
        id: content

        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        anchors.topMargin: 12
        anchors.bottomMargin: 12

        spacing: 12

        Item {
            Layout.preferredWidth: 48
            Layout.preferredHeight: 48

            visible: root.leftImageSource !== ""

            Image {
                id: leftImage

                anchors.centerIn: parent
                width: 24
                height: 24

                source: root.leftImageSource
                sourceSize: Qt.size(24, 24)
                visible: false
            }

            ColorOverlay {
                anchors.fill: leftImage
                source: leftImage
                color: root.leftImageColor
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 4

            spacing: 4

            AppH3TextType {
                Layout.fillWidth: true

                text: root.text
                color: root.textColor
            }

            AppSmallTextType {
                Layout.fillWidth: true

                visible: text !== ""

                text: root.descriptionText
            }
        }

        Item {
            id: trailingSlot

            Layout.preferredWidth: childrenRect.width
            Layout.preferredHeight: childrenRect.height

            visible: children.length > 0
        }

        Item {
            Layout.preferredWidth: 48
            Layout.preferredHeight: 48

            visible: root.rightImageSource !== ""

            Image {
                id: rightImage

                anchors.centerIn: parent
                width: 24
                height: 24

                source: root.rightImageSource
                sourceSize: Qt.size(24, 24)
                visible: false
            }

            ColorOverlay {
                anchors.fill: rightImage
                source: rightImage
                color: AmneziaStyle.color.surfaceInverse
            }
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        height: 1

        visible: root.showDivider
        color: AmneziaStyle.color.borderSoft
    }
}

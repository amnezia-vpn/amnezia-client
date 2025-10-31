import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes

import Style 1.0

import "../Config"
import "../Controls2"
import "../Controls2/TextTypes"

Rectangle {
    id: root

    property real contentHeight: content.implicitHeight + content.anchors.topMargin + content.anchors.bottomMargin
    property bool isFocusable: true

    gradient: Gradient {
        orientation: Gradient.Horizontal
        GradientStop { position: 0.0; color: AmneziaStyle.color.translucentSlateGray }
        GradientStop { position: 1.0; color: AmneziaStyle.color.translucentOnyxBlack }
    }
    border.width: 1
    border.color: AmneziaStyle.color.onyxBlack
    radius: 13

    visible: ServersModel.isAdVisible

    Keys.onTabPressed: {
        FocusController.nextKeyTabItem()
    }

    Keys.onBacktabPressed: {
        FocusController.previousKeyTabItem()
    }

    Keys.onUpPressed: {
        FocusController.nextKeyUpItem()
    }

    Keys.onDownPressed: {
        FocusController.nextKeyDownItem()
    }

    Keys.onLeftPressed: {
        FocusController.nextKeyLeftItem()
    }

    Keys.onRightPressed: {
        FocusController.nextKeyRightItem()
    }

    Keys.onEnterPressed: {
        Qt.openUrlExternally(ServersModel.getDefaultServerData("adEndpoint"))
    }

    Keys.onReturnPressed: {
        Qt.openUrlExternally(ServersModel.getDefaultServerData("adEndpoint"))
    }

    RowLayout {
        id: content
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 12
        anchors.topMargin: 12
        anchors.bottomMargin: 12
        spacing: 20

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            CaptionTextType {
                Layout.fillWidth: true
                text: ServersModel.adHeader
                color: AmneziaStyle.color.paleGray
                font.pixelSize: 14
                font.weight: 700

                textFormat: Text.RichText
            }

            CaptionTextType {
                Layout.fillWidth: true
                text: ServersModel.adDescription
                color: AmneziaStyle.color.mutedGray
                wrapMode: Text.WordWrap
                lineHeight: 18
                lineHeightMode: Text.FixedHeight
                font.pixelSize: 14

                visible: text !== ""
            }
        }

        Item {
            implicitWidth: 40
            implicitHeight: 40
            Layout.alignment: Qt.AlignVCenter

            Rectangle {
                id: chevronBackground
                anchors.fill: parent
                radius: 12
                color: AmneziaStyle.color.transparent
                border.width: root.activeFocus ? 1 : 0
                border.color: AmneziaStyle.color.paleGray

                Behavior on color {
                    PropertyAnimation { duration: 200 }
                }

                Behavior on border.width {
                    PropertyAnimation { duration: 200 }
                }
            }

            Image {
                anchors.centerIn: parent
                source: "qrc:/images/controls/chevron-right.svg"
                sourceSize: Qt.size(24, 24)
            }
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        hoverEnabled: true

        onEntered: {
            chevronBackground.color = AmneziaStyle.color.slateGray
        }

        onExited: {
            chevronBackground.color = AmneziaStyle.color.transparent
        }

        onPressedChanged: {
            chevronBackground.color = pressed ? AmneziaStyle.color.charcoalGray : containsMouse ? AmneziaStyle.color.slateGray : AmneziaStyle.color.transparent
        }

        onClicked: function() {
            root.forceActiveFocus()
            Qt.openUrlExternally(ServersModel.getDefaultServerData("adEndpoint"))
        }
    }
}

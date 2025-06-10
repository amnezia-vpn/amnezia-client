pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

import Config 1.0

import "../Controls/TextTypes"
import "../Controls"

Button {
    id: settingsButton
    Layout.fillWidth: true

    property alias buttonText: headerText.text
    signal buttonClicked()

    background: Rectangle {
        anchors.fill: parent
        radius: 8
        color: Style.color.transparent

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            enabled: false
        }
    }

    contentItem: Item {
        implicitWidth: content.implicitWidth
        implicitHeight: content.implicitHeight

        RowLayout {
            id: content
            anchors.fill: parent

            Header3TextType {
                id: headerText
                Layout.fillWidth: true
                Layout.leftMargin: 8
                Layout.topMargin: 19
                Layout.bottomMargin: 19
                horizontalAlignment: Text.AlignLeft
                text: buttonText
                color: Style.color.black
            }

            Item { Layout.fillWidth: true }

            Image {
                Layout.rightMargin: 8
                source: "qrc:/images/controls/chevron-right.svg"
                layer {
                    enabled: true
                    effect: ColorOverlay {
                        color: Style.color.black
                    }
                }
            }
        }
    }

    onClicked: buttonClicked()
}

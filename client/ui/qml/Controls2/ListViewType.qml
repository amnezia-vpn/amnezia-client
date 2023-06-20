import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "TextTypes"

ListView {
    id: menuContent

    property var rootWidth
    property var selectedText
    property string imageSource
    property var clickedFunction
    property bool dividerVisible: false

    width: rootWidth
    height: menuContent.contentItem.height

    clip: true
    interactive: false

    ButtonGroup {
        id: buttonGroup
    }

    delegate: Item {
        implicitWidth: rootWidth
        implicitHeight: content.implicitHeight

        ColumnLayout {
            id: content

            anchors.fill: parent

            RadioButton {
                id: radioButton

                implicitWidth: parent.width
                implicitHeight: radioButtonContent.implicitHeight

                hoverEnabled: true

                indicator: Rectangle {
                    anchors.fill: parent
                    color: radioButton.hovered ? "#2C2D30" : "#1C1D21"

                    Behavior on color {
                        PropertyAnimation { duration: 200 }
                    }
                }

                RowLayout {
                    id: radioButtonContent
                    anchors.fill: parent

                    anchors.rightMargin: 16
                    anchors.leftMargin: 16

                    z: 1

                    ParagraphTextType {
                        Layout.fillWidth: true
                        Layout.topMargin: 20
                        Layout.bottomMargin: 20

                        text: name
                    }

                    Image {
                        source: imageSource ? imageSource : "qrc:/images/controls/check.svg"
                        visible: imageSource ? true : radioButton.checked

                        width: 24
                        height: 24

                        Layout.rightMargin: 8
                    }
                }

                ButtonGroup.group: buttonGroup
                checked: menuContent.currentIndex === index

                onClicked: {
                    menuContent.currentIndex = index
                    menuContent.selectedText = name
                    if (clickedFunction && typeof clickedFunction === "function") {
                        clickedFunction()
                    }
                }
            }

            DividerType {
                Layout.fillWidth: true
                Layout.bottomMargin: 4

                visible: dividerVisible
            }
        }

        Component.onCompleted: {
            if (menuContent.currentIndex === index) {
                menuContent.selectedText = name
            }
        }
    }
}

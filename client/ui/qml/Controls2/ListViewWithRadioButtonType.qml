import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "TextTypes"

ListView {
    id: root

    property var rootWidth

    property var selectedText

    property int textMaximumLineCount: 2
    property int textElide: Qt.ElideRight

    property string imageSource: "qrc:/images/controls/check.svg"

    property var clickedFunction

    property int selectedIndex: 0

    width: rootWidth
    height: root.contentItem.height

    clip: true
    reuseItems: true

    property bool isFocusable: true

    ScrollBar.vertical: ScrollBarType {}

    ButtonGroup {
        id: buttonGroup
    }

    function triggerCurrentItem() {
        var item = root.itemAtIndex(selectedIndex)
        item.selectable.clicked()
    }

    delegate: ColumnLayout {
        id: content

        property alias selectable: radioButton

        implicitWidth: rootWidth

        RadioButton {
            id: radioButton

            implicitWidth: parent.width
            implicitHeight: radioButtonContent.implicitHeight

            hoverEnabled: true

            property bool isFocusable: true

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

            indicator: Rectangle {
                width: parent.width - 1
                height: parent.height
                color: radioButton.hovered ? AmneziaStyle.color.slateGray : AmneziaStyle.color.onyxBlack
                border.color: radioButton.focus ? AmneziaStyle.color.paleGray : AmneziaStyle.color.transparent
                border.width: radioButton.focus ? 1 : 0

                Behavior on color {
                    PropertyAnimation { duration: 200 }
                }
                Behavior on border.color {
                    PropertyAnimation { duration: 200 }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    enabled: false
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
                    maximumLineCount: root.textMaximumLineCount
                    elide: root.textElide

                }

                Image {
                    source: imageSource
                    visible: radioButton.checked

                    width: 24
                    height: 24

                    Layout.rightMargin: 8
                }
            }

            ButtonGroup.group: buttonGroup
            checked: root.selectedIndex === index

            onClicked: {
                root.selectedIndex = index
                root.selectedText = name
                if (clickedFunction && typeof clickedFunction === "function") {
                    clickedFunction()
                }
            }
        }

        Component.onCompleted: {
            if (root.selectedIndex === index) {
                root.selectedText = name
            }
        }

        Keys.onReturnPressed: {
            radioButton.clicked()
        }

        Keys.onEnterPressed: {
            radioButton.clicked()
        }
    }
}

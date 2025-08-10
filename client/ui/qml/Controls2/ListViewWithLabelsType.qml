import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "TextTypes"

ListView {
    id: menuContent

    property var rootWidth

    property var selectedText

    property string imageSource: "qrc:/images/controls/check.svg"

    property var clickedFunction

    property bool dividerVisible: false

    property int selectedIndex: 0

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

            LabelWithButtonType {
                Layout.fillWidth: true

                text: name
                rightImageSource: imageSource

                clickedFunction: function() {
                    menuContent.selectedIndex = index
                    menuContent.selectedText = name
                    if (menuContent.clickedFunction && typeof menuContent.clickedFunction === "function") {
                        menuContent.clickedFunction()
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
            if (menuContent.selectedIndex === index) {
                menuContent.selectedText = name
            }
        }
    }
}

import QtQuick
import QtQuick.Layouts

import Style 1.0

BaseHeaderType {
    id: root

    property string actionButtonImage
    property string actionButtonAccessibleName: root.headerText
    property var actionButtonFunction
    property alias actionButton: headerActionButton

    Component.onCompleted: {
        headerRow.children.push(headerActionButton)
    }

    ImageButtonType {
        id: headerActionButton
        implicitWidth: 40
        implicitHeight: 40
        Layout.alignment: Qt.AlignRight
        image: root.actionButtonImage
        imageColor: AmneziaStyle.color.paleGray
        accessibleName: root.actionButtonAccessibleName
        visible: image ? true : false

        onClicked: {
            if (actionButtonFunction && typeof actionButtonFunction === "function") {
                actionButtonFunction()
            }
        }
    }

    Keys.onEnterPressed: {
        if (actionButtonFunction && typeof actionButtonFunction === "function") {
            actionButtonFunction()
        }
    }

    Keys.onReturnPressed: {
        if (actionButtonFunction && typeof actionButtonFunction === "function") {
            actionButtonFunction()
        }
    }
} 

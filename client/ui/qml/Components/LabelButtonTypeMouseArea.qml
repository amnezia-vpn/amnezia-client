import QtQuick
import QtQuick.Controls

MouseArea {
    anchors.fill: parent
    cursorShape: Qt.PointingHandCursor
    hoverEnabled: root.enabled

    onEntered: {
        if (rightImageSource) {
            rightImageBackground.color = rightImage.hoveredColor
        } else if (leftImageSource) {
            leftImageBackground.color = rightImage.hoveredColor
        }
        root.textOpacity = 0.8
    }

    onExited: {
        if (rightImageSource) {
            rightImageBackground.color = rightImage.defaultColor
        } else if (leftImageSource) {
            leftImageBackground.color = rightImage.defaultColor
        }
        root.textOpacity = 1
    }

    onPressedChanged: {
        if (rightImageSource) {
            rightImageBackground.color = pressed ? rightImage.pressedColor : entered ? rightImage.hoveredColor : rightImage.defaultColor
        } else if (leftImageSource) {
            leftImageBackground.color = pressed ? rightImage.pressedColor : entered ? rightImage.hoveredColor : rightImage.defaultColor
        }
        root.textOpacity = 0.7
    }

    onClicked: {
        if (clickedFunction && typeof clickedFunction === "function") {
            clickedFunction()
        }
    }
}

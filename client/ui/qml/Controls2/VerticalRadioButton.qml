import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

RadioButton {
    id: root

    property string descriptionText

    property string hoveredColor: Qt.rgba(1, 1, 1, 0.05)
    property string defaultColor: Qt.rgba(1, 1, 1, 0)
    property string disabledColor: Qt.rgba(1, 1, 1, 0)
    property string selectedColor: Qt.rgba(1, 1, 1, 0)

    property string textColor: "#0E0E11"

    property string pressedBorderColor: Qt.rgba(251/255, 178/255, 106/255, 0.3)
    property string selectedBorderColor: "#FBB26A"
    property string defaultBodredColor: "transparent"
    property int borderWidth: 0

    property string defaultCircleBorderColor: "#878B91"
    property string selectedCircleBorderColor: "#A85809"
    property string pressedCircleBorderColor: Qt.rgba(251/255, 178/255, 106/255, 0.3)

    property string defaultInnerCircleColor: "#FBB26A"

    hoverEnabled: true

    indicator: Rectangle {
        implicitWidth: 56
        implicitHeight: 56
        radius: 16

        color: {
            if (root.enabled) {
                if (root.hovered || contentMouseArea.entered) {
                    return hoveredColor
                } else if (root.checked) {
                    return selectedColor
                }
                return defaultColor
            } else {
                return disabledColor
            }
        }

        Behavior on color {
            PropertyAnimation { duration: 200 }
        }

        Rectangle {
            width: 24
            height: 24
            radius: 16

            anchors.centerIn: parent

            color: "transparent"
            border.color: {
                if (root.enabled) {
                    if (root.pressed) {
                        return pressedCircleBorderColor
                    } else if (root.checked) {
                        return selectedCircleBorderColor
                    }
                }
                return defaultCircleBorderColor
            }

            border.width: 1

            Behavior on border.color {
                PropertyAnimation { duration: 200 }
            }

            Rectangle {
                id: innerCircle

                width: 12
                height: 12
                radius: 16

                anchors.centerIn: parent

                color: "transparent"
                border.color: defaultInnerCircleColor
                border.width: {
                    if (root.enabled) {
                        if(root.checked) {
                            return 6
                        }
                        return root.pressed ? 6 : 0
                    } else {
                        return 0
                    }
                }

                Behavior on border.width {
                    PropertyAnimation { duration: 200 }
                }
            }

            DropShadow {
                anchors.fill: innerCircle
                horizontalOffset: 0
                verticalOffset: 0
                radius: 12
                samples: 13
                color: "#FBB26A"
                source: innerCircle
            }
        }
    }

    contentItem: ColumnLayout {
        id: content
        anchors.left: indicator.right
        anchors.top: parent.top
        anchors.leftMargin: 8
        spacing: 16

        Text {
            text: root.text
            wrapMode: Text.WordWrap
            color: "#D7D8DB"
            font.pixelSize: 16
            font.weight: 400
            font.family: "PT Root UI VF"

            height: 24
            Layout.fillWidth: true
        }

        Text {
            font.family: "PT Root UI"
            font.styleName: "normal"
            font.pixelSize: 13
            font.letterSpacing: 0.02
            color: "#878B91"
            text: root.descriptionText
            wrapMode: Text.WordWrap

            visible: root.descriptionText !== ""

            Layout.fillWidth: true
            height: 16
        }
    }

    MouseArea {
        id: contentMouseArea

        anchors.fill: content
        cursorShape: Qt.PointingHandCursor
        hoverEnabled: true

//            onEntered: {
//                checkBoxBackground.color = hoveredColor
//            }

//            onExited: {
//                checkBoxBackground.color = defaultColor
//            }

//            onPressedChanged: {
//                indicator.source = pressed ? imageSource : ""
//                imageColor.color = pressed ? hoveredImageColor : defaultImageColor
//                checkBoxBackground.color = pressed ? pressedColor : entered ? hoveredColor : defaultColor
//            }

//            onClicked: {
//                checkBox.checked = !checkBox.checked
//                indicator.source = checkBox.checked ? imageSource : ""
//                imageColor.color = checkBox.checked ? checkedImageColor : defaultImageColor
//                imageBorder.border.color = checkBox.checked ? checkedBorderColor : defaultBorderColor
//            }
    }
}

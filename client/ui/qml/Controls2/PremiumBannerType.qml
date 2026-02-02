import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

Button {
    id: root

    property string headerText: qsTr("Try Amnesia Premium")
    property string bodyText: qsTr("High speed and 20 countries to connect to 7 days free.")

    property string hoveredColor: AmneziaStyle.color.charcoalGray
    property string defaultColor: AmneziaStyle.color.onyxBlack
    property int paddingContent: 20

    hoverEnabled: true
    height: contentItemRoot.implicitHeight
    width: contentItemRoot.implicitWidth

    background: Rectangle {
        id: backgroundRect

        anchors.fill: parent
        radius: 16

        color: root.hovered ? hoveredColor : defaultColor

        Behavior on color {
            PropertyAnimation { duration: 200 }
        }
    }

    contentItem: Item {
        id: contentItemRoot
        anchors.fill: parent
        implicitHeight: content.implicitHeight + root.paddingContent * 2
        implicitWidth: content.implicitWidth + root.paddingContent * 2
        anchors.margins: root.paddingContent

        GridLayout {
            id: content
            anchors.fill: parent
            columns: 2
            columnSpacing: 5

            Item {
                Layout.fillWidth: true
                Layout.minimumWidth: 0  // Позволяет сжиматься
                implicitHeight: textColumn.implicitHeight

                Column {
                    id: textColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    spacing: 6

                    // Заголовок с выделением "Premium"
                    Text {
                        width: parent.width
                        text: qsTr("Try Amnezia ") + '<span style="color: #E6007A;">Premium</span>'
                        textFormat: Text.RichText
                        color: AmneziaStyle.color.paleGray
                        font.pixelSize: 20
                        font.weight: 700
                        font.family: "PT Root UI VF"
                        wrapMode: Text.WordWrap
                    }

                    // Описание
                    Text {
                        text: root.bodyText
                        wrapMode: Text.WordWrap
                        color: AmneziaStyle.color.mutedGray
                        font.pixelSize: 14
                        font.weight: 400
                        font.family: "PT Root UI VF"
                        lineHeight: 20
                        lineHeightMode: Text.FixedHeight
                        width: parent.width
                    }
                }
            }

            // Стрелка справа
            Item {
                implicitWidth: 40
                implicitHeight: 40
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                Layout.minimumWidth: 40
                Layout.maximumWidth: 40
                Layout.preferredWidth: 40

                Image {
                    anchors.centerIn: parent
                    source: "qrc:/images/controls/chevron-right.svg"
                    sourceSize: Qt.size(24, 24)
                }
            }
        }
    }


    MouseArea {
        anchors.fill: parent

        cursorShape: Qt.PointingHandCursor
        hoverEnabled: true
        enabled: root.enabled

        onEntered: {
            backgroundRect.color = root.hoveredColor
        }

        onExited: {
            backgroundRect.color = root.defaultColor
        }

        onClicked: {
            root.clicked()
        }
    }
}

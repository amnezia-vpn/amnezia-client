import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../Controls2/TextTypes"
import "../Controls2"

Item {
    id: root

    property string text
    property int textMaximumLineCount: 2
    property int textElide: Qt.ElideRight

    property string descriptionText

    property var clickedFunction

    property string rightImageSource

    property string textColor: "#d7d8db"
    property string descriptionColor: "#878B91"
    property real textOpacity: 1.0

    property string rightImageColor: "#d7d8db"

    property bool descriptionOnTop: false

    property string defaultServerHostName
    property string defaultContainerName

    implicitWidth: content.implicitWidth + content.anchors.topMargin + content.anchors.bottomMargin
    implicitHeight: content.implicitHeight + content.anchors.leftMargin + content.anchors.rightMargin

    ColumnLayout {
        id: content

        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: parent.bottom

        RowLayout {
            Layout.topMargin: 24
            Layout.leftMargin: 24
            Layout.rightMargin: 24
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

            Header1TextType {
                Layout.maximumWidth: root.width - 48 - 18 - 12 // todo

                maximumLineCount: 2
                elide: Qt.ElideRight

                text: root.text
                Layout.alignment: Qt.AlignLeft
            }


            ImageButtonType {
                id: rightImage

                hoverEnabled: false
                image: rightImageSource
                imageColor: rightImageColor

                icon.width: 18
                icon.height: 18
                backgroundRadius: 16
                horizontalPadding: 4
                topPadding: 4
                bottomPadding: 3
            }
        }

        LabelTextType {
            Layout.bottomMargin: 44
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

            text: {
                var description = ""
                if (ServersModel.isDefaultServerHasWriteAccess()) {
                    if (SettingsController.isAmneziaDnsEnabled()
                            && ContainersModel.isAmneziaDnsContainerInstalled(ServersModel.getDefaultServerIndex())) {
                        description += "Amnezia DNS | "
                    }
                } else {
                    if (ServersModel.isDefaultServerConfigContainsAmneziaDns()) {
                        description += "Amnezia DNS | "
                    }
                }

                description += root.defaultContainerName + " | " + root.defaultServerHostName
                return description
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        hoverEnabled: true

        onEntered: {
            rightImage.backgroundColor = rightImage.hoveredColor

            root.textOpacity = 0.8
        }

        onExited: {
            rightImage.backgroundColor = rightImage.defaultColor

            root.textOpacity = 1
        }

        onPressedChanged: {
            rightImage.backgroundColor = pressed ? rightImage.pressedColor : entered ? rightImage.hoveredColor : rightImage.defaultColor

            root.textOpacity = 0.7
        }

        onClicked: {
            if (clickedFunction && typeof clickedFunction === "function") {
                clickedFunction()
            }
        }
    }
}

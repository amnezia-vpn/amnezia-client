import QtQuick
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

Item {
    id: root

    property string backButtonImage: "qrc:/images/controls/arrow-left.svg"
    property var backButtonFunction

    implicitWidth: content.implicitWidth
    implicitHeight: content.implicitHeight

    RowLayout {
        id: content

        anchors.fill: parent

        ImageButtonType {
            image: backButtonImage
            imageColor: "#D7D8DB"

            onClicked: {
                if (backButtonFunction && typeof backButtonFunction === "function") {
                    backButtonFunction()
                } else {
                    closePage()
                }
            }
        }

        Rectangle {
            id: background
            Layout.fillWidth: true

            color: "transparent"

            ShaderEffectSource {
                id: effectSource

                sourceItem: background
                anchors.fill: background
                sourceRect: Qt.rect(x,y, width, height)
            }

            FastBlur {
                id: blur
                anchors.fill: effectSource

                source: effectSource
                radius: 100
            }
        }
    }
}

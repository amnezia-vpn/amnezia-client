import QtQuick
import QtQuick.Layouts

import Style 1.0

Rectangle {
    id: root

    property alias source: image.source

    implicitWidth: 358
    implicitHeight: root.width

    Layout.preferredHeight: root.width

    color: AmneziaStyle.color.bgStaticWhite
    radius: 16

    Image {
        id: image

        anchors.fill: parent
        anchors.margins: 12

        smooth: false
        fillMode: Image.PreserveAspectFit
        sourceSize.width: width
        sourceSize.height: height
    }
}

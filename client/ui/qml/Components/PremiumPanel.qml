import QtQuick
import QtQuick.Layouts

import Style 1.0

Rectangle {
    id: root

    property color fillColor: FBLinkStyle.color.onyxBlack
    property color outlineColor: Qt.rgba(39/255, 39/255, 42/255, 1.0)
    property color accentColor: "#EAB308"
    property bool accentVisible: false
    property int padding: 18

    default property alias contentData: contentLayout.data

    radius: 20
    color: fillColor
    border.color: outlineColor
    border.width: 1

    implicitHeight: contentLayout.implicitHeight + (padding * 2) + (accentVisible ? 10 : 0)

    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 10
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        height: accentVisible ? 3 : 0
        radius: 2
        visible: accentVisible
        color: root.accentColor
        opacity: 0.9
    }

    ColumnLayout {
        id: contentLayout
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: root.padding
        anchors.topMargin: root.padding + (root.accentVisible ? 8 : 0)
        spacing: 12
    }
}

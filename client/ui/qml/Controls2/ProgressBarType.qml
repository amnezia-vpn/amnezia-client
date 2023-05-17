import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ProgressBar {
    id: root

    implicitHeight: 4

    background: Rectangle {
        color: "#412102"
    }

    contentItem: Rectangle {
        width: root.visualPosition * root.width
        height: root.height
        color: "#FBB26A"
    }
}

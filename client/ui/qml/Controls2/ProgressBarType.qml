import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ProgressBar {
    id: root

    implicitHeight: 4

    background: Rectangle {
        color: "#633303"
    }

    contentItem: Item {
        Rectangle {
            width: root.visualPosition * parent.width
            height: parent.height
            color: "#FBB26A"
        }
    }
}

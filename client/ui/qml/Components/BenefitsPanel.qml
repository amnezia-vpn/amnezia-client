import QtQuick
import QtQuick.Layouts

import "."

import Style 1.0

Rectangle {
    id: root

    property var benefitItems: []

    radius: 16
    color: "#1C1C1E"
    implicitHeight: inner.implicitHeight + 24

    ColumnLayout {
        id: inner

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 12
        spacing: 20

        Repeater {
            model: root.benefitItems ? root.benefitItems.length : 0

            delegate: BenefitRow {
                Layout.fillWidth: true
                iconSource: root.benefitItems[index].icon
                titleText: root.benefitItems[index].title
                bodyText: root.benefitItems[index].body
            }
        }
    }
}

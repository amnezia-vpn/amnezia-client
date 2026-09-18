import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../Controls2"
import "../Controls2/TextTypes"

DrawerType2 {
    id: root

    property var listModel: null

    width: parent.width
    height: parent.height

    expandedStateContent: ColumnLayout {
        id: content

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 0

        Component.onCompleted: {
            root.expandedHeight = content.implicitHeight + 32
        }

        ButtonGroup {
            id: sortModeGroup
        }

        Header2TextType {
            Layout.fillWidth: true
            Layout.topMargin: 24
            Layout.rightMargin: 16
            Layout.leftMargin: 16
            Layout.bottomMargin: 16

            text: qsTr("Sort countries")
        }

        VerticalRadioButton {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            text: qsTr("By region")

            ButtonGroup.group: sortModeGroup
            checked: root.listModel !== null && root.listModel.sortMode === 0

            onClicked: {
                root.listModel.sortMode = 0
                root.closeTriggered()
            }
        }

        DividerType {
            Layout.fillWidth: true
        }

        VerticalRadioButton {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            text: qsTr("Alphabetically")

            ButtonGroup.group: sortModeGroup
            checked: root.listModel !== null && root.listModel.sortMode === 1

            onClicked: {
                root.listModel.sortMode = 1
                root.closeTriggered()
            }
        }

        DividerType {
            Layout.fillWidth: true
        }
    }
}

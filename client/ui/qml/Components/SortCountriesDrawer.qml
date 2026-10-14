import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../Controls2"
import "../Controls2/TextTypes"

SheetType {
    id: root

    property var listModel: null

    expandedHeight: content.implicitHeight + 32 + PageController.safeAreaBottomMargin

    ColumnLayout {
        id: content

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 0

        ButtonGroup {
            id: sortModeGroup
        }

        SectionHeaderType {
            Layout.fillWidth: true
            Layout.topMargin: 16
            Layout.rightMargin: 16
            Layout.leftMargin: 16
            Layout.bottomMargin: 16

            headerText: qsTr("Sort countries")
        }

        RadioItemType {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            text: qsTr("By region")

            ButtonGroup.group: sortModeGroup
            checked: root.listModel !== null && root.listModel.sortMode === 0

            onClicked: {
                root.listModel.sortMode = 0
                root.close()
            }
        }

        RadioItemType {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            text: qsTr("Alphabetically")
            showSeparator: false

            ButtonGroup.group: sortModeGroup
            checked: root.listModel !== null && root.listModel.sortMode === 1

            onClicked: {
                root.listModel.sortMode = 1
                root.close()
            }
        }
    }
}

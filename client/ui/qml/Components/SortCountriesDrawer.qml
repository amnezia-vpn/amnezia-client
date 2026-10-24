import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../Controls2"
import "../Controls2/TextTypes"

SheetType {
    id: root

    property var listModel: null

    expandedHeight: content.implicitHeight + 24 + PageController.safeAreaBottomMargin

    ColumnLayout {
        id: content

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 0

        SectionHeaderType {
            Layout.fillWidth: true
            Layout.topMargin: 18
            Layout.rightMargin: 16
            Layout.leftMargin: 16
            Layout.bottomMargin: 32

            headerText: qsTr("Filters")
            descriptionText: qsTr("Sorting")
        }

        RadioItemType {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.preferredHeight: 80

            text: qsTr("By region")

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
            Layout.preferredHeight: 80

            text: qsTr("Alphabetically")

            checked: root.listModel !== null && root.listModel.sortMode === 1

            onClicked: {
                root.listModel.sortMode = 1
                root.close()
            }
        }
    }
}

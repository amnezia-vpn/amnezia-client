import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"

SheetType {
    id: root

    readonly property var accentNames: [qsTr("Orange"), qsTr("Purple"), qsTr("Teal"), qsTr("Blue")]
    readonly property string currentAccentName: accentNames[SettingsController.accentColor] ?? accentNames[0]

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

            headerText: qsTr("Accent color")
        }

        Repeater {
            model: root.accentNames

            delegate: RadioItemType {
                required property string modelData
                required property int index

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.preferredHeight: 80

                text: modelData
                showSeparator: index < root.accentNames.length - 1

                checked: SettingsController.accentColor === index

                onClicked: {
                    SettingsController.accentColor = index
                    root.close()
                }

                Rectangle {
                    Layout.preferredWidth: 24
                    Layout.preferredHeight: 24
                    Layout.rightMargin: 4

                    radius: width / 2
                    color: AmneziaStyle.accents[index].swatch
                    border.width: 1
                    border.color: AmneziaStyle.color.borderSoft
                }
            }
        }
    }
}

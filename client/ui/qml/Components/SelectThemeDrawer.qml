import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../Controls2"
import "../Controls2/TextTypes"

SheetType {
    id: root

    readonly property var themes: [
        { scheme: Qt.ColorScheme.Unknown, name: qsTr("System") },
        { scheme: Qt.ColorScheme.Light, name: qsTr("Light") },
        { scheme: Qt.ColorScheme.Dark, name: qsTr("Dark") }
    ]
    readonly property string currentThemeName: {
        const current = themes.find(theme => theme.scheme === SettingsController.colorScheme)
        return current ? current.name : themes[0].name
    }

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

            headerText: qsTr("Theme")
        }

        Repeater {
            model: root.themes

            delegate: RadioItemType {
                required property var modelData
                required property int index

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.preferredHeight: 80

                text: modelData.name
                showSeparator: index < root.themes.length - 1

                checked: SettingsController.colorScheme === modelData.scheme

                onClicked: {
                    SettingsController.colorScheme = modelData.scheme
                    root.close()
                }
            }
        }
    }
}

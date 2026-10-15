import QtQuick
import QtQuick.Layouts

import Style 1.0

import "TextTypes"

Item {
    id: root

    property string text
    property string descriptionText
    property alias checked: switcher.checked
    readonly property alias switcher: switcher

    signal toggled()

    implicitWidth: content.implicitWidth
    implicitHeight: content.implicitHeight

    opacity: root.enabled ? 1 : 0.4

    ColumnLayout {
        id: content

        anchors.left: parent.left
        anchors.right: parent.right

        spacing: 8

        RowLayout {
            Layout.fillWidth: true

            spacing: 16

            BodyTextType {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter

                text: root.text
                color: AmneziaStyle.color.textPrimary

                TapHandler {
                    enabled: root.enabled
                    onTapped: {
                        switcher.toggle()
                        switcher.toggled()
                    }
                }
            }

            SwitchType {
                id: switcher

                Layout.alignment: Qt.AlignVCenter

                Accessible.name: root.text
                Accessible.description: root.descriptionText

                opacity: 1

                onToggled: root.toggled()
            }
        }

        SmallTextType {
            Layout.fillWidth: true

            visible: text !== ""
            text: root.descriptionText
        }
    }
}

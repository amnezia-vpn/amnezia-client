import QtQuick
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

import Style 1.0

import "TextTypes"

ColumnLayout {
    id: root

    property string iconSource
    property string title
    property var items: []

    spacing: 10

    visible: items && items.length > 0

    RowLayout {
        Layout.fillWidth: true
        spacing: 8

        Image {
            Layout.preferredWidth: 18
            Layout.preferredHeight: 18

            source: root.iconSource
            sourceSize: Qt.size(18, 18)

            layer.enabled: true
            layer.effect: ColorOverlay {
                color: AmneziaStyle.color.accentSuccess
            }
        }

        AppH3EmphasizedTextType {
            Layout.fillWidth: true

            text: root.title
        }
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 0

        Repeater {
            model: root.items

            delegate: RowLayout {
                Layout.fillWidth: true
                spacing: 0

                AppTextType {
                    Layout.alignment: Qt.AlignTop
                    Layout.preferredWidth: 24

                    horizontalAlignment: Text.AlignHCenter
                    color: AmneziaStyle.color.textTertiary

                    text: "•"
                }

                AppTextType {
                    Layout.fillWidth: true

                    color: AmneziaStyle.color.textTertiary

                    text: modelData
                }
            }
        }
    }
}

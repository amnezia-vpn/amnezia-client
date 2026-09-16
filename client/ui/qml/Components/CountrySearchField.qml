import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "../Controls2"

Rectangle {
    id: root

    property alias textField: input
    property alias text: input.text

    signal cleared()

    implicitHeight: 64

    color: AmneziaStyle.color.surfaceBase
    radius: 16
    border.width: 1
    border.color: input.activeFocus ? AmneziaStyle.color.paleGray : AmneziaStyle.color.borderSoft

    Behavior on border.color {
        PropertyAnimation { duration: 200 }
    }

    function clear() {
        input.text = ""
        root.cleared()
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 8
        spacing: 8

        TextField {
            id: input

            Layout.fillWidth: true

            color: AmneziaStyle.color.textPrimary
            placeholderText: qsTr("Country or region")
            placeholderTextColor: AmneziaStyle.color.mutedGray

            selectionColor: AmneziaStyle.color.richBrown
            selectedTextColor: AmneziaStyle.color.textPrimary

            font.pixelSize: 16
            font.weight: 400
            font.family: "PT Root UI VF"

            inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText

            topPadding: 0
            bottomPadding: 0
            leftPadding: 0
            rightPadding: 0

            background: Rectangle {
                color: AmneziaStyle.color.transparent
            }

            Keys.onEscapePressed: root.clear()

            ContextMenu.menu: ContextMenuType {
                textObj: input
            }
        }

        ImageButtonType {
            visible: input.text !== ""

            implicitWidth: 40
            implicitHeight: 40

            hoverEnabled: true
            image: "qrc:/images/controls/close.svg"
            imageColor: AmneziaStyle.color.paleGray

            onClicked: root.clear()
            Keys.onEnterPressed: root.clear()
            Keys.onReturnPressed: root.clear()
        }
    }
}

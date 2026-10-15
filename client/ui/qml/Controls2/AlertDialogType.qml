import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "TextTypes"

Popup {
    id: root

    enum Role { Default, Destructive }

    property string headerText
    property string descriptionText
    property string cancelButtonText: qsTr("Cancel")
    property string confirmButtonText
    property int role: AlertDialogType.Default

    signal confirmed()

    parent: Overlay.overlay

    x: Math.round((parent.width - width) / 2)
    y: Math.round((parent.height - height) / 2)
    width: Math.min(parent.width - 32, 400)

    padding: 0
    topPadding: 20

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    Overlay.modal: Rectangle {
        color: AmneziaStyle.color.bgOverlay
    }

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 150 }
    }

    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 150 }
    }

    onOpened: {
        cancelButton.forceActiveFocus()
    }

    background: Rectangle {
        radius: 16
        color: AmneziaStyle.color.bgSurfaceAlt
    }

    contentItem: ColumnLayout {
        spacing: 2

        ColumnLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.bottomMargin: 16

            spacing: 8

            H3TextType {
                Layout.fillWidth: true

                text: root.headerText
            }

            SmallTextType {
                Layout.fillWidth: true

                visible: text !== ""

                text: root.descriptionText
            }
        }

        RowLayout {
            Layout.fillWidth: true

            spacing: 0

            DialogButtonType {
                id: cancelButton

                Layout.fillWidth: true
                Layout.preferredWidth: 1

                text: root.cancelButtonText

                onClicked: root.close()
            }

            DialogButtonType {
                id: confirmButton

                Layout.fillWidth: true
                Layout.preferredWidth: 1

                text: root.confirmButtonText
                emphasized: true
                textColor: root.role === AlertDialogType.Destructive ? AmneziaStyle.color.statusDestructive
                                                                     : AmneziaStyle.color.textAccent

                onClicked: {
                    root.close()
                    root.confirmed()
                }
            }
        }
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "TextTypes"

Popup {
    id: root

    property string headerText
    property string descriptionText
    property string cancelButtonText: qsTr("Cancel")
    property string confirmButtonText

    signal confirmed()

    parent: Overlay.overlay

    x: Math.round((parent.width - width) / 2)
    y: Math.round((parent.height - height) / 2)
    width: parent.width - 32

    padding: 0
    topPadding: 20

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    Overlay.modal: Rectangle {
        color: AmneziaStyle.color.overlayScrim
    }

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 150 }
    }

    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 150 }
    }

    onOpened: {
        FocusController.pushRootObject(root)
        cancelButton.forceActiveFocus()
    }

    onClosed: {
        FocusController.dropRootObject(root)
    }

    background: Rectangle {
        radius: 16
        color: AmneziaStyle.color.surfaceElevated
    }

    contentItem: ColumnLayout {
        spacing: 0

        ColumnLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.bottomMargin: 16

            spacing: 8

            AppH3TextType {
                Layout.fillWidth: true

                text: root.headerText
            }

            AppSmallTextType {
                Layout.fillWidth: true

                visible: text !== ""

                text: root.descriptionText
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 8
            Layout.rightMargin: 8
            Layout.bottomMargin: 8

            spacing: 8

            AlertDialogButton {
                id: cancelButton

                Layout.fillWidth: true

                text: root.cancelButtonText
                textColor: AmneziaStyle.color.textTertiary

                onClicked: {
                    root.close()
                }
            }

            AlertDialogButton {
                id: confirmButton

                Layout.fillWidth: true

                text: root.confirmButtonText
                textColor: AmneziaStyle.color.accentDanger
                font.weight: 600

                onClicked: {
                    root.close()
                    root.confirmed()
                }
            }
        }
    }

    component AlertDialogButton: AbstractButton {
        id: button

        property color textColor

        property bool isFocusable: true

        implicitHeight: 48

        hoverEnabled: true

        font.family: "PT Root UI VF"
        font.pixelSize: 16
        font.weight: 400
        font.letterSpacing: -0.4

        Keys.onTabPressed: FocusController.nextKeyTabItem()
        Keys.onBacktabPressed: FocusController.previousKeyTabItem()
        Keys.onLeftPressed: FocusController.nextKeyLeftItem()
        Keys.onRightPressed: FocusController.nextKeyRightItem()
        Keys.onEnterPressed: button.clicked()
        Keys.onReturnPressed: button.clicked()

        background: Rectangle {
            radius: 16
            color: (button.pressed || button.hovered) ? AmneziaStyle.color.translucentWhite : AmneziaStyle.color.transparent
            border.color: (button.pressed || button.visualFocus) ? AmneziaStyle.color.textTertiary : AmneziaStyle.color.transparent
            border.width: (button.pressed || button.visualFocus) ? 1 : 0

            Behavior on color {
                PropertyAnimation { duration: 200 }
            }
        }

        contentItem: Text {
            text: button.text
            font: button.font
            color: button.textColor

            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        MouseArea {
            anchors.fill: parent
            enabled: false
            cursorShape: Qt.PointingHandCursor
        }
    }
}

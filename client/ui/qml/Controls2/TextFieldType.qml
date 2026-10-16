import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "TextTypes"

Item {
    id: root

    property string headerText
    property string helperText
    property string errorText
    property bool checkEmptyText: false

    property bool passwordToggle: false
    property bool passwordRevealed: false
    property bool clearable: false

    property string buttonText
    property string buttonImageSource
    property bool buttonJoined: root.buttonText !== ""
    property var clickedFunc
    property bool rightButtonClickedOnEnter: false

    property alias textField: textField
    property alias placeholderText: textField.placeholderText

    readonly property bool hasError: root.errorText !== ""

    implicitWidth: content.implicitWidth
    implicitHeight: content.implicitHeight

    opacity: root.enabled ? 1 : 0.4

    function runClickedFunc() {
        if (root.clickedFunc && typeof root.clickedFunc === "function") {
            root.clickedFunc()
        }
    }

    ColumnLayout {
        id: content

        anchors.fill: parent
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            spacing: root.buttonJoined ? 0 : 8

            TextField {
                id: textField

                Layout.fillWidth: true
                Layout.preferredHeight: root.headerText !== "" ? 64 : 56

                leftPadding: 16
                rightPadding: actions.visible ? actions.width + 16 : 16
                topPadding: root.headerText !== "" ? 28 : 16
                bottomPadding: root.headerText !== "" ? 12 : 16
                verticalAlignment: TextInput.AlignVCenter

                color: AmneziaStyle.color.textPrimary
                placeholderTextColor: AmneziaStyle.color.textTertiary
                selectionColor: AmneziaStyle.color.accentDeep
                selectedTextColor: AmneziaStyle.color.textPrimary

                font.pixelSize: 16
                font.weight: 400
                font.family: "PT Root UI VF"
                font.letterSpacing: -0.4
                font.features: { "ss08": 1, "ss09": 1, "lnum": 1, "tnum": 1 }

                inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhSensitiveData | Qt.ImhNoPredictiveText
                echoMode: root.passwordToggle && !root.passwordRevealed ? TextInput.Password : TextInput.Normal

                background: Rectangle {
                    radius: 16
                    topRightRadius: root.buttonJoined && attachedButton.visible ? 0 : 16
                    bottomRightRadius: root.buttonJoined && attachedButton.visible ? 0 : 16

                    color: AmneziaStyle.color.bgSurfaceAlt
                    border.width: textField.activeFocus ? 2 : 1
                    border.color: {
                        if (root.hasError) {
                            return AmneziaStyle.color.statusDestructive
                        }
                        if (textField.activeFocus) {
                            return AmneziaStyle.color.borderFocus
                        }
                        return AmneziaStyle.color.borderBold
                    }

                    CaptionTextType {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.leftMargin: 16
                        anchors.rightMargin: textField.rightPadding
                        anchors.topMargin: 12

                        visible: root.headerText !== ""
                        text: root.headerText
                        maximumLineCount: 1
                        elide: Text.ElideRight
                    }
                }

                onTextChanged: {
                    root.errorText = ""
                }

                onActiveFocusChanged: {
                    if (root.checkEmptyText && text === "") {
                        root.errorText = qsTr("The field can't be empty")
                    }
                }

                ContextMenu.menu: contextMenu.useNativeEditMenu ? null : contextMenu
                ContextMenu.onRequested: function(position) {
                    contextMenu.requestNative(position)
                }

                TapHandler {
                    enabled: contextMenu.useNativeEditMenu
                    acceptedDevices: PointerDevice.TouchScreen
                    onDoubleTapped: function(eventPoint) {
                        textField.forceActiveFocus()
                        textField.cursorPosition = textField.positionAt(eventPoint.position.x, eventPoint.position.y)
                        textField.selectWord()
                        contextMenu.requestNative(eventPoint.position)
                    }
                }

                ContextMenuType {
                    id: contextMenu
                    textObj: textField
                }

                Row {
                    id: actions

                    anchors.right: parent.right
                    anchors.rightMargin: 12
                    anchors.verticalCenter: parent.verticalCenter

                    visible: clearButton.visible || eyeButton.visible

                    IconButtonType {
                        id: clearButton

                        implicitWidth: 40
                        implicitHeight: 40

                        visible: root.clearable && textField.text !== "" && !textField.readOnly
                        icon.source: "qrc:/images/controls/x-circle.svg"

                        onClicked: {
                            textField.clear()
                            textField.forceActiveFocus()
                        }
                    }

                    IconButtonType {
                        id: eyeButton

                        implicitWidth: 40
                        implicitHeight: 40

                        visible: root.passwordToggle
                        icon.source: root.passwordRevealed ? "qrc:/images/controls/eye-off.svg" : "qrc:/images/controls/eye.svg"

                        onClicked: root.passwordRevealed = !root.passwordRevealed
                    }
                }
            }

            InputButtonType {
                id: attachedButton

                Layout.preferredHeight: textField.height

                visible: root.buttonText !== "" || root.buttonImageSource !== ""
                joined: root.buttonJoined

                text: root.buttonText
                imageSource: root.buttonImageSource

                onClicked: root.runClickedFunc()
            }
        }

        SmallTextType {
            Layout.fillWidth: true
            Layout.topMargin: 4

            visible: text !== ""
            text: root.hasError ? root.errorText : root.helperText
            color: root.hasError ? AmneziaStyle.color.statusDestructive : AmneziaStyle.color.textTertiary
        }
    }

    Keys.onEnterPressed: {
        if (root.rightButtonClickedOnEnter) {
            root.runClickedFunc()
        }
    }

    Keys.onReturnPressed: {
        if (root.rightButtonClickedOnEnter) {
            root.runClickedFunc()
        }
    }
}

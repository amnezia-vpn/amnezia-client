import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "."
import "TextTypes"
import "../Config"

Popup {
    id: root

    property string captchaId
    property string captchaImageBase64
    property string hint: qsTr("Enter the digits from the image to continue")

    signal captchaSolved(string captchaId, string solution)
    signal refreshCaptchaRequested()

    readonly property int keyboardHeight: {
        if (PageController.imeHeight > 0) {
            return PageController.imeHeight
        }
        if (Qt.platform.os === "ios" && Qt.inputMethod.visible) {
            const r = Qt.inputMethod.keyboardRectangle
            if (r.height <= 0) {
                return 0
            }
            return Math.max(0, Math.round(parent.height - r.y))
        }
        return 0
    }

    parent: Overlay.overlay

    width: parent ? parent.width : 0
    height: Math.min(contentLayout.implicitHeight + contentLayout.anchors.topMargin + 8 + PageController.safeAreaBottomMargin,
                     parent ? parent.height - 20 - root.keyboardHeight : 0)

    x: 0
    y: parent ? parent.height - height - root.keyboardHeight : 0

    padding: 0

    Behavior on y {
        NumberAnimation {
            duration: 150
            easing.type: Easing.OutCubic
        }
    }

    modal: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    Overlay.modal: Rectangle {
        color: AmneziaStyle.color.bgOverlay
    }

    focus: true

    onOpened: {
        solutionField.textField.text = ""
        solutionField.textField.forceActiveFocus()
    }

    onCaptchaIdChanged: {
        if (opened) {
            solutionField.textField.text = ""
        }
    }

    onCaptchaImageBase64Changed: {
        if (opened) {
            solutionField.textField.text = ""
        }
    }

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 150 }
    }

    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 150 }
    }

    background: Item {
        Rectangle {
            anchors.fill: parent
            anchors.bottomMargin: -border.width

            topLeftRadius: 16
            topRightRadius: 16
            color: AmneziaStyle.color.bgSurfaceAlt
            border.color: AmneziaStyle.color.borderFaint
            border.width: 1
        }

        SheetGrabberType {
            anchors.top: parent.top
            anchors.topMargin: 8
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }

    contentItem: Flickable {
        contentWidth: width
        contentHeight: contentLayout.implicitHeight + contentLayout.anchors.topMargin + 8 + PageController.safeAreaBottomMargin
        boundsBehavior: Flickable.StopAtBounds
        interactive: contentHeight > height
        clip: true

        ColumnLayout {
            id: contentLayout

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 8 + 5 + 16
            anchors.leftMargin: 16
            anchors.rightMargin: 16

            spacing: 0

            H2TextType {
                id: titleText

                Layout.fillWidth: true

                text: root.hint
            }

            Rectangle {
                id: imagePanel

                Layout.fillWidth: true
                Layout.preferredHeight: 128
                Layout.topMargin: 32

                color: AmneziaStyle.color.bgSurfaceInverse
                radius: 16
                clip: true

                Image {
                    id: captchaImage

                    anchors.fill: parent
                    fillMode: Image.PreserveAspectFit
                    cache: false

                    Component.onCompleted: {
                        if (captchaImageBase64 !== "") {
                            source = "data:image/png;base64," + captchaImageBase64
                        }
                    }

                    Connections {
                        target: root
                        function onCaptchaImageBase64Changed() {
                            captchaImage.source = "data:image/png;base64," + root.captchaImageBase64
                        }
                    }
                }

                SpinnerType {
                    anchors.centerIn: parent
                    color: AmneziaStyle.color.textInverted
                    visible: captchaImage.status === Image.Loading
                }

                IconButtonType {
                    id: refreshButton

                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.rightMargin: 16
                    anchors.bottomMargin: 12

                    filled: true
                    icon.source: "qrc:/images/controls/refresh-cw.svg"

                    onClicked: root.refreshCaptchaRequested()
                }
            }

            TextFieldType {
                id: solutionField

                Layout.fillWidth: true
                Layout.topMargin: 32

                headerText: qsTr("Digits from the image")

                textField.placeholderText: qsTr("_ _ _ _ _ _")
                textField.inputMethodHints: Qt.ImhDigitsOnly | Qt.ImhNoPredictiveText
                textField.maximumLength: 6
                textField.validator: RegularExpressionValidator { regularExpression: /^[0-9]{0,6}$/ }
                textField.font.letterSpacing: 2

                textField.onAccepted: {
                    submitIfNonEmpty()
                }
            }

            ButtonType {
                id: continueButton

                Layout.fillWidth: true
                Layout.topMargin: 12

                text: qsTr("Continue")

                onClicked: {
                    submitIfNonEmpty()
                }
            }
        }
    }

    function submitIfNonEmpty() {
        const t = solutionField.textField.text.trim()
        if (t !== "") {
            root.captchaSolved(root.captchaId, t)
        }
    }
}

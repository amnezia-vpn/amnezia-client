import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "TextTypes"
import "../Config"

Popup {
    id: root

    property string captchaId
    property string captchaImageBase64
    property string hint: "Please solve the CAPTCHA to continue"

    signal captchaSolved(string captchaId, string solution)
    signal refreshCaptchaRequested()

    leftMargin: 25
    rightMargin: 25
    bottomMargin: 70 + SettingsController.safeAreaBottomMargin

    width: parent.width - leftMargin - rightMargin

    anchors.centerIn: parent
    modal: true
    closePolicy: Popup.NoAutoClose

    Overlay.modal: Rectangle {
        color: AmneziaStyle.color.translucentMidnightBlack
    }

    onOpened: {
        timer.start()
        solutionInput.text = ""
        solutionInput.focus = true
    }

    onClosed: {
        FocusController.dropRootObject(root)
    }

    background: Rectangle {
        anchors.fill: parent
        color: "white"
        radius: 4
    }

    Timer {
        id: timer
        interval: 200
        onTriggered: {
            FocusController.pushRootObject(root)
            FocusController.setFocusItem(solutionInput)
        }
        repeat: false
        running: true
    }

    contentItem: Item {
        implicitWidth: contentLayout.implicitWidth
        implicitHeight: contentLayout.implicitHeight

        anchors.fill: parent

        ColumnLayout {
            id: contentLayout

            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            anchors.topMargin: 16
            anchors.bottomMargin: 16

            spacing: 12

            CaptionTextType {
                text: qsTr("CAPTCHA Verification")
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignLeft
            }

            ParagraphTextType {
                text: hint
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignLeft
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 200
                color: AmneziaStyle.color.lightGray
                radius: 4

                Image {
                    id: captchaImage
                    anchors.centerIn: parent
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

                BusyIndicator {
                    anchors.centerIn: parent
                    running: captchaImage.status === Image.Loading
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                ParagraphTextType {
                    text: qsTr("Can't read the image?")
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignLeft
                }

                BasicButtonType {
                    text: qsTr("Refresh")
                    implicitHeight: 32

                    onClicked: {
                        root.refreshCaptchaRequested()
                    }
                }
            }

            ParagraphTextType {
                text: qsTr("Enter the numbers from the image:")
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignLeft
            }

            TextField {
                id: solutionInput

                Layout.fillWidth: true
                implicitHeight: 40

                placeholderText: qsTr("Enter CAPTCHA solution")

                background: Rectangle {
                    border.color: AmneziaStyle.color.charcoalGray
                    border.width: 1
                    radius: 4
                    color: "white"
                }

                onAccepted: {
                    if (solutionInput.text.trim() !== "") {
                        root.captchaSolved(root.captchaId, solutionInput.text.trim())
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                BasicButtonType {
                    id: submitButton

                    Layout.fillWidth: true
                    implicitHeight: 40

                    text: qsTr("Submit")

                    onClicked: {
                        if (solutionInput.text.trim() !== "") {
                            root.captchaSolved(root.captchaId, solutionInput.text.trim())
                        }
                    }
                }

                BasicButtonType {
                    id: cancelButton

                    Layout.fillWidth: true
                    implicitHeight: 40

                    text: qsTr("Cancel")
                    defaultColor: AmneziaStyle.color.lightGray
                    hoveredColor: AmneziaStyle.color.charcoalGray
                    textColor: AmneziaStyle.color.midnightBlack

                    onClicked: {
                        root.close()
                    }
                }
            }
        }
    }
}

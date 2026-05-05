import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import Qt5Compat.GraphicalEffects

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
        solutionField.textField.text = ""
        solutionField.textField.focus = true
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

    onClosed: {
        FocusController.dropRootObject(root)
    }

    background: Rectangle {
        anchors.fill: parent
        color: AmneziaStyle.color.slateGray
        radius: 22
    }

    Timer {
        id: timer
        interval: 200
        onTriggered: {
            FocusController.pushRootObject(root)
            FocusController.setFocusItem(solutionField.textField)
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
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            anchors.topMargin: 20
            anchors.bottomMargin: 20

            spacing: 16

            Text {
                id: titleText

                Layout.fillWidth: true
                Layout.alignment: Qt.AlignLeft | Qt.AlignTop

                text: root.hint
                wrapMode: Text.WordWrap
                color: AmneziaStyle.color.paleGray
                font.pixelSize: 18
                font.weight: Font.Bold
                font.family: "PT Root UI VF"
                lineHeight: 24 + LanguageUiController.getLineHeightAppend()
                lineHeightMode: Text.FixedHeight
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignTop
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 200

                Rectangle {
                    id: imagePanel

                    anchors.fill: parent
                    color: AmneziaStyle.color.pearlGray
                    radius: 16

                    Image {
                        id: captchaImage

                        anchors.centerIn: parent
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

                    BusyIndicator {
                        anchors.centerIn: parent
                        running: captchaImage.status === Image.Loading
                    }

                    Rectangle {
                        id: refreshHit

                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.margins: 10
                        width: 44
                        height: 44
                        radius: width / 2
                        color: AmneziaStyle.color.charcoalGray

                        Image {
                            id: refreshIcon

                            anchors.centerIn: parent
                            width: 26
                            height: 26
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                            mipmap: true
                            antialiasing: true
                            source: "qrc:/images/controls/refresh-cw.svg"
                            // Rasterize SVG at high resolution, then scale down — avoids blocky edges on HiDPI.
                            readonly property real _dpr: (Window.window && Window.window.screen)
                                ? Window.window.screen.devicePixelRatio : 2.0
                            readonly property int _raster: Math.ceil(64 * Math.min(Math.max(_dpr, 1.0), 4.0))
                            sourceSize: Qt.size(_raster, _raster)

                            layer.enabled: true
                            layer.smooth: true
                            layer.textureSize: Qt.size(_raster, _raster)
                            layer.effect: ColorOverlay {
                                color: AmneziaStyle.color.goldenApricot
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.refreshCaptchaRequested()
                        }
                    }
                }
            }

            TextFieldWithHeaderType {
                id: solutionField

                Layout.fillWidth: true
                Layout.alignment: Qt.AlignLeft

                headerText: qsTr("Digits from the image")
                headerTextColor: AmneziaStyle.color.mutedGray

                textField.placeholderText: qsTr("_ _ _ _ _ _")
                textField.placeholderTextColor: AmneziaStyle.color.mutedGray
                textField.inputMethodHints: Qt.ImhDigitsOnly | Qt.ImhNoPredictiveText
                textField.maximumLength: 6
                textField.font.letterSpacing: 2

                textField.onAccepted: {
                    submitIfNonEmpty()
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 8
            }

            BasicButtonType {
                id: continueButton

                Layout.fillWidth: true
                implicitHeight: 52

                text: qsTr("Continue")
                defaultColor: AmneziaStyle.color.paleGray
                hoveredColor: AmneziaStyle.color.lightGray
                pressedColor: AmneziaStyle.color.mutedGray
                textColor: AmneziaStyle.color.midnightBlack

                clickedFunc: function() {
                    submitIfNonEmpty()
                }
            }

            BasicButtonType {
                id: closeButton

                Layout.fillWidth: true
                implicitHeight: 52

                text: qsTr("Close")
                defaultColor: AmneziaStyle.color.transparent
                hoveredColor: AmneziaStyle.color.translucentWhite
                pressedColor: AmneziaStyle.color.sheerWhite
                textColor: AmneziaStyle.color.paleGray
                borderWidth: 1
                borderColor: AmneziaStyle.color.mutedGray
                borderFocusedColor: AmneziaStyle.color.paleGray

                clickedFunc: function() {
                    root.close()
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

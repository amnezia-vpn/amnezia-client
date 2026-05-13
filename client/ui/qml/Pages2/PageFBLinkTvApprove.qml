import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Config"
import "../Controls2/TextTypes"
import "../Components"

// FBLink VPN — "I have a TV" approval page for the mobile/desktop client.
//
// The TV-side login uses a device-flow / QR-code: the TV shows a short
// `XXXX-XXXX` user code and asks the user to confirm sign-in elsewhere.
// This page is the in-app confirmation surface so users don't have to
// open a browser at all — they pick the page from PageHome, type the
// code shown on the TV, tap "Подтвердить вход", and the backend
// approves the TV's pending login under the currently signed-in user.
PageType {
    id: root

    property bool isLoading: false
    property string errorMessage: ""
    property bool approvedShown: false
    readonly property bool wideLayout: GC.isWideWidth(width)
    readonly property real sideMargin: GC.pageHorizontalMargin(width)
    readonly property real maxContentWidth: GC.pageMaxWidth(width)

    Connections {
        target: FBLinkController

        function onTvApproveSuccess() {
            root.isLoading = false
            PageController.showBusyIndicator(false)
            root.errorMessage = ""
            root.approvedShown = true
        }

        function onTvApproveError(message) {
            root.isLoading = false
            PageController.showBusyIndicator(false)
            root.errorMessage = message
        }
    }

    Flickable {
        anchors.fill: parent
        contentHeight: content.implicitHeight + 28
        clip: true

        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        Item {
            width: parent.width
            height: content.implicitHeight + 28

            ColumnLayout {
                id: content
                width: Math.min(root.maxContentWidth, parent.width - root.sideMargin * 2)
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                spacing: 18

                BackButtonType {
                    Layout.topMargin: 20 + SettingsController.safeAreaTopMargin
                    Layout.leftMargin: 4
                }

                Rectangle {
                    Layout.fillWidth: true
                    radius: 26
                    color: Qt.rgba(12/255, 12/255, 12/255, 0.98)
                    border.width: 1
                    border.color: Qt.rgba(63/255, 63/255, 70/255, 0.9)
                    clip: true
                    implicitHeight: card.implicitHeight + (root.wideLayout ? 68 : 44)

                    ColumnLayout {
                        id: card
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: root.wideLayout ? 34 : 22
                        spacing: 16

                        LabelTextType {
                            Layout.fillWidth: true
                            text: qsTr("Войти на телевизоре")
                            font.pixelSize: root.wideLayout ? 32 : 26
                            font.weight: 700
                            color: "#F5F5F5"
                            wrapMode: Text.WordWrap
                        }

                        CaptionTextType {
                            Layout.fillWidth: true
                            text: qsTr("Откройте FBLink VPN на Android TV, выберите вход по QR-коду и введите ниже код, который покажет телевизор. После подтверждения ТВ войдёт автоматически.")
                            color: FBLinkStyle.color.mutedGray
                            font.pixelSize: 14
                            wrapMode: Text.WordWrap
                        }

                        WarningType {
                            Layout.fillWidth: true
                            visible: root.errorMessage !== ""
                            textString: root.errorMessage
                            iconPath: "qrc:/images/controls/alert-circle.svg"
                            backGroundColor: Qt.rgba(239/255, 68/255, 68/255, 0.12)
                            imageColor: "#EF4444"
                            textColor: "#FFB4B4"
                        }

                        WarningType {
                            Layout.fillWidth: true
                            visible: root.approvedShown
                            textString: qsTr("Вход подтверждён. Телевизор откроет приложение через несколько секунд.")
                            iconPath: "qrc:/images/controls/check.svg"
                            backGroundColor: Qt.rgba(16/255, 185/255, 129/255, 0.12)
                            imageColor: "#10B981"
                            textColor: "#B6F2D2"
                        }

                        TextFieldWithHeaderType {
                            id: codeField
                            Layout.fillWidth: true
                            headerText: qsTr("КОД С ЭКРАНА ТЕЛЕВИЗОРА")
                            textField.placeholderText: "XXXX-XXXX"
                            textField.inputMethodHints: Qt.ImhUppercaseOnly | Qt.ImhLatinOnly | Qt.ImhNoPredictiveText
                            textField.maximumLength: 16
                            visible: !root.approvedShown
                        }

                        BasicButtonType {
                            Layout.fillWidth: true
                            visible: !root.approvedShown
                            implicitHeight: 56
                            defaultColor: "#EAB308"
                            hoveredColor: "#FACC15"
                            pressedColor: "#CA8A04"
                            disabledColor: FBLinkStyle.color.mutedGray
                            textColor: "#111111"
                            enabled: !root.isLoading
                            text: root.isLoading
                                ? qsTr("Подтверждение...")
                                : qsTr("Подтвердить вход")
                            clickedFunc: function() {
                                root.errorMessage = ""
                                var code = codeField.textField.text.trim()
                                if (code === "") {
                                    root.errorMessage = qsTr("Введите код с экрана телевизора")
                                    return
                                }
                                root.isLoading = true
                                PageController.showBusyIndicator(true)
                                FBLinkController.approveTvLogin(code)
                            }
                        }

                        BasicButtonType {
                            Layout.fillWidth: true
                            visible: root.approvedShown
                            implicitHeight: 56
                            defaultColor: "#27272A"
                            hoveredColor: "#3F3F46"
                            pressedColor: "#18181B"
                            textColor: "#F5F5F5"
                            text: qsTr("Готово")
                            clickedFunc: function() {
                                PageController.closePage()
                            }
                        }
                    }
                }
            }
        }
    }
}

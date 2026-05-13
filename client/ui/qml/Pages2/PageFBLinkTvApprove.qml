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

// FBLink VPN — "I have a TV" entry point on the mobile / desktop client.
//
// The TV side starts a device-flow login and shows the user a QR code and
// a short XXXX-XXXX code. This page is the in-app surface where the
// already-signed-in user confirms that sign-in: either by scanning the
// QR with the phone camera (opens PageFBLinkTvScan) or by typing the
// short code into a TextField and tapping "Подтвердить вход".
PageType {
    id: root

    property bool isLoading: false
    property string errorMessage: ""
    property bool approvedShown: false
    property bool codeMode: false
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
                            text: qsTr("Откройте FBLink VPN на Android TV — приложение покажет QR-код и код. Отсканируйте QR камерой или введите код вручную, и ТВ войдёт автоматически.")
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

                        // ---- Chooser tiles (default) ------------------
                        BasicButtonType {
                            Layout.fillWidth: true
                            visible: !root.approvedShown && !root.codeMode
                            implicitHeight: 64
                            defaultColor: "#EAB308"
                            hoveredColor: "#FACC15"
                            pressedColor: "#CA8A04"
                            textColor: "#111111"
                            text: qsTr("Сканировать QR-код")
                            clickedFunc: function() {
                                root.errorMessage = ""
                                PageController.goToPage(PageEnum.PageFBLinkTvScan)
                            }
                        }

                        BasicButtonType {
                            Layout.fillWidth: true
                            visible: !root.approvedShown && !root.codeMode
                            implicitHeight: 56
                            defaultColor: "#27272A"
                            hoveredColor: "#3F3F46"
                            pressedColor: "#18181B"
                            textColor: "#F5F5F5"
                            text: qsTr("Ввести код вручную")
                            clickedFunc: function() {
                                root.errorMessage = ""
                                root.codeMode = true
                            }
                        }

                        // ---- Manual code entry ------------------------
                        TextFieldWithHeaderType {
                            id: codeField
                            Layout.fillWidth: true
                            headerText: qsTr("КОД С ЭКРАНА ТЕЛЕВИЗОРА")
                            textField.placeholderText: "XXXX-XXXX"
                            textField.inputMethodHints: Qt.ImhUppercaseOnly | Qt.ImhLatinOnly | Qt.ImhNoPredictiveText
                            textField.maximumLength: 16
                            visible: !root.approvedShown && root.codeMode
                        }

                        BasicButtonType {
                            Layout.fillWidth: true
                            visible: !root.approvedShown && root.codeMode
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
                            visible: !root.approvedShown && root.codeMode
                            implicitHeight: 48
                            defaultColor: "transparent"
                            hoveredColor: Qt.rgba(255/255, 255/255, 255/255, 0.05)
                            pressedColor: Qt.rgba(255/255, 255/255, 255/255, 0.03)
                            textColor: FBLinkStyle.color.mutedGray
                            text: qsTr("Назад к выбору")
                            clickedFunc: function() {
                                root.codeMode = false
                                root.errorMessage = ""
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

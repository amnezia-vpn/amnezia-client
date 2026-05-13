import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import QRCodeReader 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Config"
import "../Controls2/TextTypes"
import "../Components"

// FBLink VPN — TV-login QR scanner.
//
// Opens the device camera through `QRCodeReader`, parses the scanned
// payload to extract the device-flow user code, and calls
// `FBLinkController.approveTvLogin(code)`. Works with both kinds of
// content the TV's QR code can carry:
//   - the full verification URL "https://.../tv?code=ABCDEFGH"
//   - the raw user code "ABCDEFGH" or "ABCD-EFGH"
PageType {
    id: root

    property string statusMessage: qsTr("Наведите камеру на QR-код, показанный на телевизоре.")
    property string errorMessage: ""
    property bool isLoading: false
    property bool consumed: false

    // Pulls a user_code out of any payload our TV side may produce. We
    // intentionally accept a few shapes so the QR keeps working even
    // when the backend changes how it formats the verification URI.
    function extractUserCode(payload) {
        if (!payload) {
            return ""
        }
        const queryMatch = /[?&]code=([A-Za-z0-9-]+)/.exec(payload)
        if (queryMatch) {
            return queryMatch[1]
        }
        const slashMatch = /\/tv-approve\/([A-Za-z0-9-]+)/.exec(payload)
        if (slashMatch) {
            return slashMatch[1]
        }
        const trimmed = payload.trim()
        if (/^[A-Za-z0-9-]{4,16}$/.test(trimmed)) {
            return trimmed
        }
        return ""
    }

    Connections {
        target: FBLinkController

        function onTvApproveSuccess() {
            root.isLoading = false
            PageController.showBusyIndicator(false)
            root.errorMessage = ""
            // Pop the scanner; the underlying PageFBLinkTvApprove will
            // catch the same signal and render its success state.
            PageController.closePage()
        }

        function onTvApproveError(message) {
            root.isLoading = false
            root.consumed = false
            PageController.showBusyIndicator(false)
            root.errorMessage = message
            root.statusMessage = qsTr("Попробуйте отсканировать ещё раз.")
        }
    }

    BackButtonType {
        id: backButton
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.topMargin: 20 + SettingsController.safeAreaTopMargin
    }

    ColumnLayout {
        id: header
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: backButton.bottom
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        spacing: 12

        LabelTextType {
            Layout.fillWidth: true
            text: qsTr("Сканируйте QR-код с телевизора")
            font.pixelSize: 22
            font.weight: 700
            color: "#F5F5F5"
            wrapMode: Text.WordWrap
        }

        CaptionTextType {
            Layout.fillWidth: true
            text: root.statusMessage
            color: FBLinkStyle.color.mutedGray
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
    }

    Rectangle {
        id: cameraRect
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.top: header.bottom
        anchors.topMargin: 20
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.bottomMargin: 34

        color: FBLinkStyle.color.transparent

        QRCodeReader {
            id: qrCodeReader

            onCodeReaded: function(code) {
                if (root.consumed || root.isLoading) {
                    return
                }
                const userCode = root.extractUserCode(code)
                if (userCode === "") {
                    root.errorMessage = qsTr("QR-код не подходит для входа на ТВ.")
                    return
                }
                root.consumed = true
                root.isLoading = true
                root.errorMessage = ""
                root.statusMessage = qsTr("Подтверждаем вход на ТВ...")
                PageController.showBusyIndicator(true)
                FBLinkController.approveTvLogin(userCode)
            }

            Component.onCompleted: {
                qrCodeReader.setCameraSize(Qt.rect(cameraRect.x,
                                                   cameraRect.y,
                                                   cameraRect.width,
                                                   cameraRect.height))
                qrCodeReader.startReading()
            }
            Component.onDestruction: qrCodeReader.stopReading()
        }
    }
}

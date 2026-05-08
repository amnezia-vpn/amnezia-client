import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QRCodeReader 1.0
import PageEnum 1.0
import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    /** 0 = scan QR, 1 = confirm before sending subscription */
    property int pairingWizardStep: 0
    /** True after optimistic close: keep request running in background while page is closing. */
    property bool keepPhonePairingInBackgroundOnClose: false

    property bool pairingCameraOpen: false
    property int lastInvalidPairingQrToastClockMs: 0
    /** iOS may deliver many QR frames; guard duplicate step transitions. */
    property bool addDeviceConfirmNavigationScheduled: false

    Timer {
        id: pairingCameraKickTimer
        interval: 180
        repeat: false
        onTriggered: root.restartPairingIosCamera()
    }

    function restartPairingIosCamera() {
        if (Qt.platform.os !== "ios" || !root.pairingCameraOpen) {
            return
        }
        if (cameraSlot.width < 32 || cameraSlot.height < 32) {
            console.info("[PairingQr] cameraSlot too small wxh=", cameraSlot.width, cameraSlot.height, "retry")
            pairingCameraKickTimer.restart()
            return
        }
        var p = cameraSlot.mapToItem(root, 0, 0)
        console.info("[PairingQr] start preview frame", p.x, p.y, cameraSlot.width, cameraSlot.height)
        pairingQrReader.stopReading()
        pairingQrReader.setCameraSize(Qt.rect(Math.round(p.x), Math.round(p.y), Math.round(cameraSlot.width), Math.round(cameraSlot.height)))
        pairingQrReader.startReading()
    }

    Component.onDestruction: {
        if (!root.keepPhonePairingInBackgroundOnClose && !PairingUiController.phonePairingBusy) {
            PairingUiController.cancelAllPairingActivity()
        }
    }

    Connections {
        target: root
        function onVisibleChanged() {
            if (root.visible) {
                root.addDeviceConfirmNavigationScheduled = false
            } else {
                pairingCameraKickTimer.stop()
                pairingQrReader.stopReading()
                root.pairingCameraOpen = false
                root.pairingWizardStep = 0
                if (!root.keepPhonePairingInBackgroundOnClose && !PairingUiController.phonePairingBusy) {
                    PairingUiController.cancelAllPairingActivity()
                }
            }
        }
    }

    Connections {
        target: root
        function onPairingCameraOpenChanged() {
            if (!root.pairingCameraOpen) {
                pairingCameraKickTimer.stop()
                pairingQrReader.stopReading()
                return
            }
            if (Qt.platform.os === "ios") {
                pairingCameraKickTimer.restart()
            }
        }
    }

    Connections {
        target: cameraSlot
        enabled: Qt.platform.os === "ios" && root.pairingCameraOpen
        function onWidthChanged() {
            pairingCameraKickTimer.restart()
        }
        function onHeightChanged() {
            pairingCameraKickTimer.restart()
        }
    }

    FlickableType {
        anchors.fill: parent
        contentHeight: layout.implicitHeight
        interactive: contentHeight > height

        ColumnLayout {
            id: layout
            width: root.width
            spacing: 0

            BackButtonType {
                Layout.topMargin: 20 + PageController.safeAreaTopMargin
                backButtonFunction: function() {
                    if (root.pairingWizardStep === 1) {
                        PairingUiController.cancelAllPairingActivity()
                        root.pairingWizardStep = 0
                        root.addDeviceConfirmNavigationScheduled = false
                    } else {
                        PageController.closePage()
                    }
                }
            }

            StackLayout {
                id: stepStack
                Layout.fillWidth: true
                currentIndex: root.pairingWizardStep

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Label {
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        Layout.topMargin: 8
                        text: qsTr("Add device via QR")
                        font.pixelSize: 28
                        font.bold: true
                        color: AmneziaStyle.color.paleGray
                        wrapMode: Text.Wrap
                    }

                    ParagraphTextType {
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        text: qsTr("Scan the session QR shown on the device you want to add. You will confirm before the subscription is sent.")
                        wrapMode: Text.Wrap
                    }

                    BasicButtonType {
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        Layout.topMargin: 16
                        visible: Qt.platform.os === "android" || Qt.platform.os === "ios"
                        text: {
                            if (Qt.platform.os === "ios" && root.pairingCameraOpen) {
                                return qsTr("Hide camera")
                            }
                            return qsTr("Scan QR code")
                        }
                        enabled: !PairingUiController.phonePairingBusy
                        clickedFunc: function() {
                            if (Qt.platform.os === "android") {
                                PairingUiController.openPairingQrScanner()
                            } else {
                                root.pairingCameraOpen = !root.pairingCameraOpen
                            }
                        }
                    }

                    Item {
                        id: cameraSlot
                        Layout.fillWidth: true
                        Layout.preferredHeight: (root.pairingCameraOpen && Qt.platform.os === "ios") ? 220 : 0
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        visible: Layout.preferredHeight > 0
                        clip: true

                        QRCodeReader {
                            id: pairingQrReader

                            onCodeReaded: function(code) {
                                if (root.addDeviceConfirmNavigationScheduled) {
                                    return
                                }
                                if (PairingUiController.applyScannedTextAsPairingUuid(code)) {
                                    root.addDeviceConfirmNavigationScheduled = true
                                    pairingQrReader.stopReading()
                                    root.pairingCameraOpen = false
                                } else {
                                    const now = new Date().getTime()
                                    if (now - root.lastInvalidPairingQrToastClockMs >= 2200) {
                                        root.lastInvalidPairingQrToastClockMs = now
                                        PageController.showNotificationMessage(
                                                    qsTr("This QR code is not a pairing session. Show the code from the other device’s “receive config” screen."))
                                    }
                                }
                            }
                        }

                        onVisibleChanged: {
                            if (!visible) {
                                pairingQrReader.stopReading()
                                return
                            }
                            if (Qt.platform.os === "ios") {
                                pairingCameraKickTimer.restart()
                            }
                        }
                    }

                    ParagraphTextType {
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        Layout.bottomMargin: 24 + PageController.safeAreaBottomMargin
                        visible: root.pairingWizardStep === 0 && PairingUiController.phoneStatusMessage.length > 0
                        text: PairingUiController.phoneStatusMessage
                        wrapMode: Text.Wrap
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 16

                    Label {
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        Layout.topMargin: 8
                        text: qsTr("Add a new device to the subscription?")
                        font.pixelSize: 28
                        font.bold: true
                        color: AmneziaStyle.color.paleGray
                        wrapMode: Text.Wrap
                    }

                    ParagraphTextType {
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        text: qsTr("Devices available with Amnezia Premium: %1").arg(ApiAccountInfoModel.data("availableDeviceSlots"))
                        wrapMode: Text.Wrap
                    }

                    BasicButtonType {
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        Layout.topMargin: 16

                        text: qsTr("Add Device")
                        defaultColor: AmneziaStyle.color.paleGray
                        hoveredColor: AmneziaStyle.color.lightGray
                        pressedColor: AmneziaStyle.color.mutedGray
                        textColor: AmneziaStyle.color.midnightBlack

                        clickedFunc: function() {
                            root.keepPhonePairingInBackgroundOnClose = true
                            PairingUiController.submitPhonePairing(PairingUiController.pendingPhonePairingUuid,
                                                                   ServersUiController.getProcessedServerIndex())
                            Qt.callLater(function() {
                                PageController.closePage()
                            })
                        }
                    }

                    BasicButtonType {
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16

                        defaultColor: AmneziaStyle.color.transparent
                        hoveredColor: AmneziaStyle.color.translucentWhite
                        pressedColor: AmneziaStyle.color.sheerWhite
                        textColor: AmneziaStyle.color.paleGray
                        borderColor: AmneziaStyle.color.paleGray
                        borderWidth: 1
                        text: qsTr("Cancel")

                        clickedFunc: function() {
                            PairingUiController.cancelAllPairingActivity()
                            root.pairingWizardStep = 0
                            root.addDeviceConfirmNavigationScheduled = false
                        }
                    }

                }
            }
        }
    }

    Connections {
        target: PairingUiController

        function onPairingUuidFromScan(uuid) {
            if (root.addDeviceConfirmNavigationScheduled) {
                return
            }
            root.addDeviceConfirmNavigationScheduled = true
            pairingQrReader.stopReading()
            root.pairingCameraOpen = false
            PairingUiController.pendingPhonePairingUuid = uuid
            Qt.callLater(function() {
                root.pairingWizardStep = 1
            })
        }
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    readonly property bool useIosNativePairingQrOverlay: PairingUiController.iosNativePairingQrOverlayBuild

    readonly property bool useAndroidNativePairingQrOverlay: PairingUiController.androidNativePairingQrOverlayBuild
                                                             && GC.isMobile()
                                                             && Qt.platform.os === "android"

    property int pairingWizardStep: 0
    property bool keepPhonePairingInBackgroundOnClose: false

    property int lastInvalidPairingQrToastClockMs: 0
    property bool addDeviceConfirmNavigationScheduled: false
    property bool awaitingCameraPermissionForScan: false
    property bool waitingSettingsReturnForScan: false

    /** Suppress double startActivity when StackView fires both Component.onCompleted and onVisibleChanged. */
    property int _androidPairingReaderLastStartMs: 0

    function stopMobileScanner() {
        if (root.useIosNativePairingQrOverlay) {
            PairingUiController.setPairingQrTorchEnabled(false)
            PairingUiController.dismissIosPairingQrNativeOverlayScanner()
        } else if (Qt.platform.os === "android") {
            PairingUiController.setPairingQrTorchEnabled(false)
        }
    }

    function startMobileScanner() {
        if (!GC.isMobile()) {
            return
        }
        if (!root.visible) {
            return
        }
        if (root.pairingWizardStep !== 0) {
            return
        }
        if (addDeviceConfirmNavigationScheduled) {
            return
        }
        if (!PairingUiController.isPairingCameraAccessGranted()) {
            awaitingCameraPermissionForScan = true
            PairingUiController.requestPairingCameraAccess()
            return
        }
        if (root.useIosNativePairingQrOverlay) {
            PairingUiController.presentIosPairingQrNativeOverlayScanner(
                        qsTr("Add device via QR"),
                        qsTr("Scan the session QR shown on the device you want to add. You will confirm before the subscription is sent."))
            return
        }
        if (Qt.platform.os === "android") {
            const coolUntil = PairingUiController.androidPairingReaderCooldownUntilEpochMs
            if (Date.now() < coolUntil) {
                return
            }
            const now = Date.now()
            if (now - _androidPairingReaderLastStartMs < 700) {
                return
            }
            _androidPairingReaderLastStartMs = now
            PairingUiController.openPairingQrScanner()
        }
    }

    function showScanCameraDeniedDrawer() {
        showQuestionDrawer(
                    qsTr("Camera access is required"),
                    qsTr("Allow camera access to scan the pairing QR code. You can enable it in the system settings for Amnezia VPN."),
                    qsTr("Open settings"),
                    qsTr("Cancel"),
                    function() {
                        PairingUiController.openPairingCameraAppSettings()
                    },
                    function() {
                        root.waitingSettingsReturnForScan = false
                    })
    }

    function tryResumeScanAfterCameraSettings() {
        if (!waitingSettingsReturnForScan || !visible || pairingWizardStep !== 0) {
            return
        }
        if (PairingUiController.isPairingCameraAccessGranted()) {
            waitingSettingsReturnForScan = false
            startMobileScanner()
        }
    }

    Component.onDestruction: {
        if (!keepPhonePairingInBackgroundOnClose && !PairingUiController.phonePairingBusy) {
            PairingUiController.cancelAllPairingActivity()
        }
    }

    onVisibleChanged: {
        if (visible) {
            if (pairingWizardStep === 0) {
                addDeviceConfirmNavigationScheduled = false
                Qt.callLater(startMobileScanner)
            }
        } else if (!PairingUiController.phonePairingBusy) {
            stopMobileScanner()
            _androidPairingReaderLastStartMs = 0
            pairingWizardStep = 0
            waitingSettingsReturnForScan = false
            if (!keepPhonePairingInBackgroundOnClose) {
                PairingUiController.cancelAllPairingActivity()
            }
        }
    }

    onPairingWizardStepChanged: {
        if (pairingWizardStep !== 0) {
            stopMobileScanner()
        } else if (root.visible) {
            Qt.callLater(startMobileScanner)
        }
    }

    Component.onCompleted: {
        if (visible && pairingWizardStep === 0) {
            Qt.callLater(startMobileScanner)
        }
    }

    Connections {
        target: Qt.application

        function onStateChanged() {
            if (Qt.application.state !== Qt.ApplicationActive) {
                return
            }
            root.tryResumeScanAfterCameraSettings()
            if (!root.useIosNativePairingQrOverlay || root.pairingWizardStep !== 0
                    || !PairingUiController.isPairingCameraAccessGranted()) {
                return
            }
            Qt.callLater(function () {
                if (!root.visible || root.pairingWizardStep !== 0 || !GC.isMobile()) {
                    return
                }
                PairingUiController.restartIosPairingQrNativeOverlayCapture()
            })
        }
    }

    Connections {
        target: SettingsController

        enabled: Qt.platform.os === "android"

        function onActivityResumed() {
            root.tryResumeScanAfterCameraSettings()
        }
    }

    Item {
        anchors.fill: parent

        Rectangle {
            anchors.fill: parent
            visible: pairingWizardStep === 0 && root.useAndroidNativePairingQrOverlay
            color: AmneziaStyle.color.midnightBlack
            z: 1
        }

        BackButtonType {
            visible: pairingWizardStep === 0 && (root.useAndroidNativePairingQrOverlay || !GC.isMobile())
            anchors.top: parent.top
            anchors.topMargin: PageController.safeAreaTopMargin
            anchors.left: parent.left
            width: parent.width
            z: 2
            backButtonFunction: function() {
                PageController.closePage()
            }
        }

        Column {
            anchors.centerIn: parent
            width: parent.width - 48
            spacing: 12
            visible: pairingWizardStep === 0 && !GC.isMobile()

            Label {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                color: AmneziaStyle.color.mutedGray
                font.pixelSize: 15
                text: qsTr("QR pairing is available in the mobile app.")
            }
        }

        ColumnLayout {
            id: confirmStep
            anchors.fill: parent
            visible: pairingWizardStep === 1
            z: 10
            spacing: 16

            BackButtonType {
                Layout.topMargin: 20 + PageController.safeAreaTopMargin
                Layout.leftMargin: 0
                backButtonFunction: function() {
                    addDeviceConfirmNavigationScheduled = false
                    pairingWizardStep = 0
                    PairingUiController.cancelAllPairingActivity()
                }
            }

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
                    keepPhonePairingInBackgroundOnClose = true
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
                    addDeviceConfirmNavigationScheduled = false
                    pairingWizardStep = 0
                    PairingUiController.cancelAllPairingActivity()
                }
            }

            Item {
                Layout.fillHeight: true
            }
        }
    }

    Connections {
        target: PairingUiController

        function onPairingCameraAccessFinished(granted) {
            if (!awaitingCameraPermissionForScan) {
                return
            }
            awaitingCameraPermissionForScan = false
            if (granted) {
                if (root.pairingWizardStep === 0) {
                    startMobileScanner()
                }
            } else {
                waitingSettingsReturnForScan = true
                showScanCameraDeniedDrawer()
            }
        }

        function onPairingUuidFromScan(uuid) {
            if (addDeviceConfirmNavigationScheduled) {
                return
            }
            addDeviceConfirmNavigationScheduled = true
            stopMobileScanner()
            PairingUiController.pendingPhonePairingUuid = uuid
            pairingWizardStep = 1
        }

        function onPairingSendQrScanRejectedInvalidPayload() {
            if (!root.useIosNativePairingQrOverlay || root.pairingWizardStep !== 0) {
                return
            }
            const now = new Date().getTime()
            if (now - lastInvalidPairingQrToastClockMs >= 2200) {
                lastInvalidPairingQrToastClockMs = now
                PageController.showNotificationMessage(
                            qsTr("This QR code is not a pairing session. Show the code from the other device’s “receive config” screen."))
            }
        }

        function onPairingIosNativeQrOverlayBackRequested() {
            stopMobileScanner()
            PageController.closePage()
        }

        function onPairingAndroidNativeQrScannerUserDismissed() {
            if (!root.useAndroidNativePairingQrOverlay) {
                return
            }
            stopMobileScanner()
            PairingUiController.cancelAllPairingActivity()
            addDeviceConfirmNavigationScheduled = false
            PageController.closePage()
        }
    }
}

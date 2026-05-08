import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts

import QRCodeReader 1.0
import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    /** Loud dim colors when true (red/blue/cyan/orange regions). Sync with PageStart.pairingQrChromeDebug. */
    property bool pairingQrChromeDebug: false

    /** iOS (and any non-Android mobile): native QRCodeReader; Qt may not always report os === "ios". */
    readonly property bool useIosStyleNativeQrReader: GC.isMobile() && Qt.platform.os !== "android"

    /** iOS-only: full-screen UIKit UIWindow scanner (PairingUiController.iosNativePairingQrOverlayBuild — not Qt.platform.os). */
    readonly property bool useIosNativePairingQrOverlay: PairingUiController.iosNativePairingQrOverlayBuild

    /** Let dimming draw into window chrome (status bar + tab bar) when camera underlay is active. */
    readonly property bool extendScanDimToScreenEdges: GC.isMobile() && pairingWizardStep === 0
                                                     && PairingUiController.embeddedPairingQrCameraActive
    clip: !extendScanDimToScreenEdges

    /** QQuickWindow (not Item); do not type as Item — breaks binding on Qt 6. */
    readonly property var appWindow: Window.window
    /** Pixels of window above this page (status bar / safe area gap). */
    readonly property real scanDimBleedTop: {
        if (!extendScanDimToScreenEdges || !appWindow || !appWindow.contentItem)
            return 0
        let bleed = Math.max(0, root.mapToItem(appWindow.contentItem, 0, 0).y)
        if (bleed < 2 && root.useIosStyleNativeQrReader)
            bleed = Math.max(bleed, PageController.safeAreaTopMargin)
        return bleed
    }
    /** Pixels of window below this page (tab bar + home indicator). */
    readonly property real scanDimBleedBottom: {
        if (!extendScanDimToScreenEdges || !appWindow || !appWindow.contentItem)
            return 0
        const o = root.mapToItem(appWindow.contentItem, 0, root.height)
        let bleed = Math.max(0, appWindow.height - o.y)
        const slack = Math.max(0, appWindow.height - root.height - scanDimBleedTop)
        if (bleed < slack - 1)
            bleed = Math.max(bleed, slack)
        if (bleed < 2 && root.useIosStyleNativeQrReader)
            bleed = Math.max(bleed, PageController.safeAreaBottomMargin + 72)
        return bleed
    }

    /**
     * Bottom bleed for dimLayer only. On iOS embedded native QR, keep dim inside the page — semi-opaque dim
     * extended into the tab stack seam composites badly with opaque tab chrome (persistent hairline).
     * Native bottom mask still uses scanDimBleedBottom via pushIosNativeBottomBleedSync().
     */
    readonly property real scanDimBleedBottomForDimLayer: (root.useIosStyleNativeQrReader
                                                          && PairingUiController.embeddedPairingQrCameraActive) ? 0 : scanDimBleedBottom

    /** iOS: extend UIKit bottom dim under QML tab bar (see iosPairingCameraAccess + PairingUiController). */
    function pushIosNativeBottomBleedSync() {
        if (!root.useIosStyleNativeQrReader || !PairingUiController.embeddedPairingQrCameraActive) {
            return
        }
        PairingUiController.syncIosEmbeddedPairingQrNativeBottomExtra(Math.max(0, Math.round(root.scanDimBleedBottom)))
    }

    onScanDimBleedBottomChanged: {
        if (PairingUiController.embeddedPairingQrCameraActive && root.useIosStyleNativeQrReader) {
            Qt.callLater(root.pushIosNativeBottomBleedSync)
        }
    }

    Timer {
        id: pairingScanLayoutLogTimer
        interval: 50
        repeat: false
        onTriggered: root.logPairingScanLayout("timer")
    }

    Connections {
        target: PairingUiController

        function onEmbeddedPairingQrCameraActiveChanged() {
            if (PairingUiController.embeddedPairingQrCameraActive) {
                pairingScanLayoutLogTimer.restart()
                Qt.callLater(root.pushIosNativeBottomBleedSync)
            }
        }
    }

    function logPairingScanLayout(tag) {
        const w = Window.window
        const ci = w && w.contentItem ? w.contentItem : null
        let m00 = null
        let m0h = null
        let scanBot = null
        let dimTL = null
        let dimBR = null
        let dimHoleB = null
        if (ci) {
            m00 = root.mapToItem(ci, 0, 0)
            m0h = root.mapToItem(ci, 0, root.height)
            scanBot = scanStep.mapToItem(ci, 0, scanStep.height)
            dimTL = dimLayer.mapToItem(ci, 0, 0)
            dimBR = dimLayer.mapToItem(ci, dimLayer.width, dimLayer.height)
            dimHoleB = dimLayer.holeBottom
        }
        console.warn("[PairingQrLayout]", tag,
                     "extend=", extendScanDimToScreenEdges,
                     "clip=", clip,
                     "root=", root.width, "x", root.height,
                     "win=", w ? w.width : -1, "x", w ? w.height : -1,
                     "contentItem=", ci ? ci.width : -1, "x", ci ? ci.height : -1,
                     "bleedT/B=", scanDimBleedTop, scanDimBleedBottom,
                     "dimLayerBleedB=", root.scanDimBleedBottomForDimLayer,
                     "safeT/B=", PageController.safeAreaTopMargin, PageController.safeAreaBottomMargin,
                     "map00=", m00 ? m00.x + "," + m00.y : "n/a",
                     "map0h=", m0h ? m0h.x + "," + m0h.y : "n/a",
                     "ci.scanStepBot=", scanBot ? scanBot.x.toFixed(1) + "," + scanBot.y.toFixed(1) : "n/a",
                     "ci.dimTL/BR=", dimTL ? dimTL.x.toFixed(1) + "," + dimTL.y.toFixed(1) : "n/a",
                     dimBR ? dimBR.x.toFixed(1) + "," + dimBR.y.toFixed(1) : "n/a",
                     "dimHoleB=", dimHoleB !== null ? dimHoleB.toFixed(1) : "n/a",
                     "win.screen=", w && w.screen ? w.screen.width + "x" + w.screen.height : "n/a",
                     "dimLayer wh=", dimLayer.width, "x", dimLayer.height)
    }

    /** 0 = scan QR, 1 = confirm before sending subscription */
    property int pairingWizardStep: 0
    property bool keepPhonePairingInBackgroundOnClose: false

    property int lastInvalidPairingQrToastClockMs: 0
    property bool addDeviceConfirmNavigationScheduled: false
    property bool awaitingCameraPermissionForScan: false
    property bool waitingSettingsReturnForScan: false
    property bool torchOn: false

    Timer {
        id: pairingCameraKickTimer
        interval: 220
        repeat: false
        onTriggered: root.restartPairingIosCamera()
    }

    function stopMobileScanner() {
        torchOn = false
        if (root.useIosNativePairingQrOverlay) {
            PairingUiController.setPairingQrTorchEnabled(false)
            PairingUiController.dismissIosPairingQrNativeOverlayScanner()
            return
        }
        if (Qt.platform.os === "android") {
            PairingUiController.setPairingQrTorchEnabled(false)
        } else if (root.useIosStyleNativeQrReader) {
            pairingQrReader.setTorchEnabled(false)
        }
        pairingQrReader.stopReading()
        PairingUiController.embeddedPairingQrCameraActive = false
    }

    function startMobileScanner() {
        if (!GC.isMobile()) {
            return
        }
        console.warn("[PairingQrSend] startMobileScanner Qt.platform.os=", Qt.platform.os,
                       "iosNativePairingQrOverlayBuild=", PairingUiController.iosNativePairingQrOverlayBuild,
                       "useIosNativePairingQrOverlay=", root.useIosNativePairingQrOverlay)
        if (!PairingUiController.isPairingCameraAccessGranted()) {
            awaitingCameraPermissionForScan = true
            PairingUiController.requestPairingCameraAccess()
            return
        }
        if (root.useIosNativePairingQrOverlay) {
            PairingUiController.presentIosPairingQrNativeOverlayScanner(
                        qsTr("Add device via QR"),
                        qsTr("Scan the session QR shown on the device you want to add. You will confirm before the subscription is sent."))
            /** Do not run pairingCameraKickTimer here: restartCapture during first startRunning races the session (torch needs 2–3 taps). */
            return
        }
        PairingUiController.embeddedPairingQrCameraActive = true
        if (root.useIosStyleNativeQrReader) {
            // Session must start here, not only after pairingCameraKickTimer (220ms), otherwise
            // torch/scan run before startReading and native layer never attaches.
            restartPairingIosCamera()
            pairingCameraKickTimer.restart()
            /** After resume embedded can stay true so setEmbedded skips; QUIMetalView may be opaque again. */
            Qt.callLater(function () {
                PairingUiController.refreshIosEmbeddedPairingQrChrome()
            })
        }
    }

    function startPairingScanAfterPermission() {
        startMobileScanner()
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

    function restartPairingIosCamera() {
        if (root.useIosNativePairingQrOverlay) {
            PairingUiController.restartIosPairingQrNativeOverlayCapture()
            return
        }
        if (!root.useIosStyleNativeQrReader || pairingWizardStep !== 0) {
            return
        }
        // Never gate on root.visible here: under StackView the active page often has
        // visible === false while it is on screen, so startReading never ran (no session, no torch).
        pairingQrReader.stopReading()
        pairingQrReader.startReading()
    }

    Component.onDestruction: {
        if (!keepPhonePairingInBackgroundOnClose && !PairingUiController.phonePairingBusy) {
            PairingUiController.cancelAllPairingActivity()
        }
    }

    onVisibleChanged: {
        if (visible) {
            addDeviceConfirmNavigationScheduled = false
            if (pairingWizardStep === 0) {
                Qt.callLater(startMobileScanner)
            }
        } else {
            pairingCameraKickTimer.stop()
            stopMobileScanner()
            pairingWizardStep = 0
            waitingSettingsReturnForScan = false
            if (!keepPhonePairingInBackgroundOnClose && !PairingUiController.phonePairingBusy) {
                PairingUiController.cancelAllPairingActivity()
            }
        }
    }

    onPairingWizardStepChanged: {
        if (pairingWizardStep !== 0) {
            stopMobileScanner()
        } else {
            Qt.callLater(startMobileScanner)
        }
    }

    /**
     * StackView often instantiates the page already visible — onVisibleChanged(true) may never run, so
     * startMobileScanner (and native overlay present) would be skipped; only stop/dismiss runs. Same pattern as
     * PageSetupWizardApiQrPairingReceive (Component.onCompleted + visible).
     */
    Component.onCompleted: {
        if (GC.isMobile() && root.visible && pairingWizardStep === 0) {
            console.warn("[PairingQrSend] Component.onCompleted: schedule startMobileScanner (page created visible)")
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
            if (!root.useIosStyleNativeQrReader || root.pairingWizardStep !== 0
                || !PairingUiController.isPairingCameraAccessGranted()) {
                return
            }
            if (root.useIosNativePairingQrOverlay) {
                Qt.callLater(function () {
                    if (!root.visible || root.pairingWizardStep !== 0 || !GC.isMobile()) {
                        return
                    }
                    PairingUiController.restartIosPairingQrNativeOverlayCapture()
                })
                return
            }
            /**
             * No fixed ms delay: f1 reapply UIView transparency; f2 restart AVCapture; f3 refresh again —
             * after restartPairingIosCamera, QUIMetalView / render thread often rebuilds opaque layers (same bug
             * as status-bar-only camera) until underlay is reapplied once more.
             */
            Qt.callLater(function () {
                if (!root.visible || root.pairingWizardStep !== 0 || !GC.isMobile()) {
                    return
                }
                console.warn("[PairingQrResume] ApplicationActive f1 underlay")
                PairingUiController.embeddedPairingQrCameraActive = true
                PairingUiController.refreshIosEmbeddedPairingQrChrome()
                Qt.callLater(function () {
                    if (!root.visible || root.pairingWizardStep !== 0) {
                        return
                    }
                    console.warn("[PairingQrResume] ApplicationActive f2 restart camera")
                    root.restartPairingIosCamera()
                    Qt.callLater(function () {
                        if (!root.visible || root.pairingWizardStep !== 0) {
                            return
                        }
                        console.warn("[PairingQrResume] ApplicationActive f3 underlay post-camera")
                        PairingUiController.refreshIosEmbeddedPairingQrChrome()
                    })
                })
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

        Item {
            id: scanStep
            anchors.fill: parent
            visible: pairingWizardStep === 0

            readonly property real sqSz: Math.floor(Math.min(width, height) * 0.72)
            readonly property real sqX: (width - sqSz) / 2
            readonly property real sqY: (height - sqSz) / 2 - height * 0.06
            readonly property real dimAlpha: 0.55
            readonly property color dimTopDebug: "#aa3333"
            readonly property color dimBottomDebug: "#33aaff"
            readonly property color dimLeftDebug: "#3333ff"
            readonly property color dimRightDebug: "#ffaa33"
            readonly property int bracketThick: 5
            readonly property int bracketLen: Math.max(28, Math.floor(sqSz * 0.13))
            readonly property real bracketRadius: bracketThick * 0.5

            Rectangle {
                anchors.fill: parent
                color: AmneziaStyle.color.midnightBlack
                visible: !GC.isMobile()
            }

            Label {
                anchors.centerIn: parent
                width: parent.width - 48
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                visible: !GC.isMobile()
                color: AmneziaStyle.color.mutedGray
                font.pixelSize: 15
                text: qsTr("QR pairing is available in the mobile app.")
            }

            Item {
                id: dimLayer
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.topMargin: -root.scanDimBleedTop
                anchors.bottom: parent.bottom
                anchors.bottomMargin: -root.scanDimBleedBottomForDimLayer
                visible: GC.isMobile()
                z: 0

                readonly property real holeTop: root.scanDimBleedTop + scanStep.sqY
                readonly property real holeBottom: holeTop + scanStep.sqSz

                Rectangle {
                    x: 0
                    y: 0
                    width: parent.width
                    height: Math.max(0, dimLayer.holeTop)
                    color: root.pairingQrChromeDebug ? scanStep.dimTopDebug : Qt.rgba(0, 0, 0, scanStep.dimAlpha)
                }
                Rectangle {
                    x: 0
                    y: dimLayer.holeBottom
                    width: parent.width
                    height: Math.max(0, dimLayer.height - dimLayer.holeBottom)
                    color: root.pairingQrChromeDebug ? scanStep.dimBottomDebug : Qt.rgba(0, 0, 0, scanStep.dimAlpha)
                }
                Rectangle {
                    x: 0
                    y: dimLayer.holeTop
                    width: Math.max(0, scanStep.sqX)
                    height: scanStep.sqSz
                    color: root.pairingQrChromeDebug ? scanStep.dimLeftDebug : Qt.rgba(0, 0, 0, scanStep.dimAlpha)
                }
                Rectangle {
                    x: scanStep.sqX + scanStep.sqSz
                    y: dimLayer.holeTop
                    width: Math.max(0, parent.width - (scanStep.sqX + scanStep.sqSz))
                    height: scanStep.sqSz
                    color: root.pairingQrChromeDebug ? scanStep.dimRightDebug : Qt.rgba(0, 0, 0, scanStep.dimAlpha)
                }
            }

            /** Same onyx as tab bar: bridges dim/camera to TabBar sibling so the seam is not only TabBar.background overlap. */
            Rectangle {
                id: pairingIosStackBottomChromeBridge
                objectName: "pairingIosStackBottomChromeBridge"
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 22
                visible: GC.isMobile() && root.useIosStyleNativeQrReader && PairingUiController.embeddedPairingQrCameraActive
                color: root.pairingQrChromeDebug ? "#8844ff" : AmneziaStyle.color.onyxBlack
                z: 1
            }

            Item {
                x: scanStep.sqX
                y: scanStep.sqY
                width: scanStep.sqSz
                height: scanStep.sqSz
                visible: GC.isMobile()

                Rectangle {
                    x: 0
                    y: 0
                    width: scanStep.bracketLen
                    height: scanStep.bracketThick
                    radius: scanStep.bracketRadius
                    color: AmneziaStyle.color.paleGray
                }
                Rectangle {
                    x: 0
                    y: 0
                    width: scanStep.bracketThick
                    height: scanStep.bracketLen
                    radius: scanStep.bracketRadius
                    color: AmneziaStyle.color.paleGray
                }

                Rectangle {
                    x: scanStep.sqSz - scanStep.bracketLen
                    y: 0
                    width: scanStep.bracketLen
                    height: scanStep.bracketThick
                    radius: scanStep.bracketRadius
                    color: AmneziaStyle.color.paleGray
                }
                Rectangle {
                    x: scanStep.sqSz - scanStep.bracketThick
                    y: 0
                    width: scanStep.bracketThick
                    height: scanStep.bracketLen
                    radius: scanStep.bracketRadius
                    color: AmneziaStyle.color.paleGray
                }

                Rectangle {
                    x: 0
                    y: scanStep.sqSz - scanStep.bracketThick
                    width: scanStep.bracketLen
                    height: scanStep.bracketThick
                    radius: scanStep.bracketRadius
                    color: AmneziaStyle.color.paleGray
                }
                Rectangle {
                    x: 0
                    y: scanStep.sqSz - scanStep.bracketLen
                    width: scanStep.bracketThick
                    height: scanStep.bracketLen
                    radius: scanStep.bracketRadius
                    color: AmneziaStyle.color.paleGray
                }

                Rectangle {
                    x: scanStep.sqSz - scanStep.bracketLen
                    y: scanStep.sqSz - scanStep.bracketThick
                    width: scanStep.bracketLen
                    height: scanStep.bracketThick
                    radius: scanStep.bracketRadius
                    color: AmneziaStyle.color.paleGray
                }
                Rectangle {
                    x: scanStep.sqSz - scanStep.bracketThick
                    y: scanStep.sqSz - scanStep.bracketLen
                    width: scanStep.bracketThick
                    height: scanStep.bracketLen
                    radius: scanStep.bracketRadius
                    color: AmneziaStyle.color.paleGray
                }
            }

            Column {
                id: headerBlock
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.topMargin: 8 + PageController.safeAreaTopMargin
                spacing: 10
                z: 2
                /** Native iOS overlay draws its own header. */
                visible: !GC.isMobile() || !root.useIosNativePairingQrOverlay

                BackButtonType {
                    width: parent.width
                    backButtonFunction: function() {
                        PageController.closePage()
                    }
                }

                Label {
                    width: parent.width - 32
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("Add device via QR")
                    font.pixelSize: 28
                    font.bold: true
                    color: AmneziaStyle.color.paleGray
                    wrapMode: Text.Wrap
                }

                ParagraphTextType {
                    width: parent.width - 32
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("Scan the session QR shown on the device you want to add. You will confirm before the subscription is sent.")
                    wrapMode: Text.Wrap
                }
            }

            Item {
                z: 2
                width: 56
                height: 56
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottomMargin: 28 + PageController.safeAreaBottomMargin
                visible: GC.isMobile() && !root.useIosNativePairingQrOverlay

                Rectangle {
                    anchors.fill: parent
                    radius: 28
                    color: Qt.rgba(1, 1, 1, root.torchOn ? 0.42 : 0.22)
                    border.width: root.torchOn ? 2 : 0
                    border.color: AmneziaStyle.color.goldenApricot
                }
                Text {
                    anchors.centerIn: parent
                    text: "🔦"
                    font.pixelSize: 26
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        root.torchOn = !root.torchOn
                        if (Qt.platform.os === "android") {
                            PairingUiController.setPairingQrTorchEnabled(root.torchOn)
                        } else if (root.useIosNativePairingQrOverlay) {
                            PairingUiController.setPairingQrTorchEnabled(root.torchOn)
                        } else if (root.useIosStyleNativeQrReader) {
                            pairingQrReader.setTorchEnabled(root.torchOn)
                        }
                    }
                }
            }

            ParagraphTextType {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottomMargin: 100 + PageController.safeAreaBottomMargin
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                visible: PairingUiController.phoneStatusMessage.length > 0
                text: PairingUiController.phoneStatusMessage
                wrapMode: Text.Wrap
                z: 2
            }

            Item {
                width: 0
                height: 0
                visible: false

                QRCodeReader {
                    id: pairingQrReader

                    // Same idea as PageSetupWizardQrReader: ensure startReading runs even if
                    // StackView/onVisible timing skips startMobileScanner once.
                    Component.onCompleted: {
                        if (root.useIosNativePairingQrOverlay) {
                            return
                        }
                        if (!root.useIosStyleNativeQrReader || root.pairingWizardStep !== 0) {
                            return
                        }
                        Qt.callLater(function () {
                            if (root.pairingWizardStep !== 0 || !PairingUiController.isPairingCameraAccessGranted()) {
                                return
                            }
                            PairingUiController.embeddedPairingQrCameraActive = true
                            pairingQrReader.stopReading()
                            pairingQrReader.startReading()
                        })
                    }

                    onCodeReaded: function(code) {
                        if (addDeviceConfirmNavigationScheduled) {
                            return
                        }
                        if (PairingUiController.applyScannedTextAsPairingUuid(code)) {
                            addDeviceConfirmNavigationScheduled = true
                            stopMobileScanner()
                        } else {
                            const now = new Date().getTime()
                            if (now - lastInvalidPairingQrToastClockMs >= 2200) {
                                lastInvalidPairingQrToastClockMs = now
                                PageController.showNotificationMessage(
                                            qsTr("This QR code is not a pairing session. Show the code from the other device’s “receive config” screen."))
                            }
                        }
                    }
                }
            }
        }

        ColumnLayout {
            id: confirmStep
            anchors.fill: parent
            anchors.leftMargin: 0
            anchors.rightMargin: 0
            visible: pairingWizardStep === 1
            spacing: 16

            BackButtonType {
                Layout.topMargin: 20 + PageController.safeAreaTopMargin
                Layout.leftMargin: 0
                backButtonFunction: function() {
                    PairingUiController.cancelAllPairingActivity()
                    pairingWizardStep = 0
                    addDeviceConfirmNavigationScheduled = false
                    Qt.callLater(startMobileScanner)
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
                    PairingUiController.cancelAllPairingActivity()
                    pairingWizardStep = 0
                    addDeviceConfirmNavigationScheduled = false
                    Qt.callLater(startMobileScanner)
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
                startMobileScanner()
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
            Qt.callLater(function() {
                pairingWizardStep = 1
            })
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
    }
}

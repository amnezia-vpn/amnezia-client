import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes
import Qt5Compat.GraphicalEffects

import ConnectionState 1.0
import PageEnum 1.0
import Style 1.0

Button {
    id: root

    property string defaultButtonColor: "#52525B"
    property string progressButtonColor: "#EAB308"
    property string connectedButtonColor: "#10B981"
    property bool buttonActiveFocus: activeFocus && (Qt.platform.os !== "android" || SettingsController.isOnTv())

    property bool isFocusable: true

    // ── Auth / subscription gate ─────────────────────────────────
    // Block connect when not logged in, or logged in without active subscription
    readonly property bool isNotLoggedIn: !FBLinkController.isLoggedIn
    readonly property bool isSubscriptionRequired:
        !FBLinkController.isLoggedIn || !FBLinkController.isSubscribed

    readonly property string lockedButtonColor: "#6B7280"

    // ── Shake animation for errors / locked tap ──────────────────
    property bool isErrorState: false
    Timer {
        id: errorResetTimer
        interval: 1500
        onTriggered: root.isErrorState = false
    }

    SequentialAnimation {
        id: errorShakeAnim
        NumberAnimation { target: shakeTranslate; property: "x"; from: 0; to: -15; duration: 60; easing.type: Easing.OutSine }
        NumberAnimation { target: shakeTranslate; property: "x"; from: -15; to: 15; duration: 60; easing.type: Easing.InOutSine }
        NumberAnimation { target: shakeTranslate; property: "x"; from: 15; to: -15; duration: 60; easing.type: Easing.InOutSine }
        NumberAnimation { target: shakeTranslate; property: "x"; from: -15; to: 15; duration: 60; easing.type: Easing.InOutSine }
        NumberAnimation { target: shakeTranslate; property: "x"; from: 15; to: 0; duration: 60; easing.type: Easing.InSine }
    }

    Keys.onTabPressed:      FocusController.nextKeyTabItem()
    Keys.onBacktabPressed:  FocusController.previousKeyTabItem()
    Keys.onUpPressed:       FocusController.nextKeyUpItem()
    Keys.onDownPressed:     FocusController.nextKeyDownItem()
    Keys.onLeftPressed:     FocusController.nextKeyLeftItem()
    Keys.onRightPressed:    FocusController.nextKeyRightItem()

    implicitWidth: Qt.platform.os === "android" || Qt.platform.os === "ios" ? 164 : 188
    implicitHeight: Qt.platform.os === "android" || Qt.platform.os === "ios" ? 164 : 188
    padding: 0

    text: root.isNotLoggedIn
        ? qsTr("Войти")
        : (root.isSubscriptionRequired ? qsTr("Премиум") : ConnectionController.connectionStateText)

    Connections {
        target: ConnectionController

        function onPreparingConfig() {
            PageController.showNotificationMessage(qsTr("Невозможно отключиться во время подготовки конфигурации"))
        }

        function onConnectionErrorOccurred(errorCode) {
            root.isErrorState = true
            errorResetTimer.restart()
            errorShakeAnim.restart()
        }
    }

    // ── Resolved accent color (single source of truth) ───────────
    readonly property color accentColor: {
        if (root.isSubscriptionRequired) return root.lockedButtonColor
        if (root.isErrorState)           return "#EF4444"
        if (ConnectionController.isConnectionInProgress) return root.progressButtonColor
        if (ConnectionController.isConnected)            return root.connectedButtonColor
        return root.defaultButtonColor
    }

    background: Item {
        id: bgItem
        transform: Translate { id: shakeTranslate }
        implicitWidth: parent.width
        implicitHeight: parent.height
        transformOrigin: Item.Center

        Rectangle {
            anchors.centerIn: parent
            width: parent.width + 10
            height: parent.height + 10
            radius: width / 2
            color: root.accentColor
            opacity: ConnectionController.isConnectionInProgress ? 0.16 : 0.08

            Behavior on opacity { NumberAnimation { duration: 150 } }
        }

        Rectangle {
            id: mainCircle
            anchors.fill: parent
            radius: width / 2
            color: "#121212"
            border.width: root.buttonActiveFocus ? 2 : 1
            border.color: root.accentColor

            Rectangle {
                anchors.fill: parent
                radius: parent.radius
                color: root.accentColor
                opacity: ConnectionController.isConnected ? 0.12 : (ConnectionController.isConnectionInProgress ? 0.10 : 0.04)
            }

            Rectangle {
                id: ripple
                anchors.centerIn: parent
                width: 0
                height: 0
                radius: width / 2
                color: Qt.rgba(1, 1, 1, 0.18)
                opacity: 0
            }

            ParallelAnimation {
                id: rippleAnim
                NumberAnimation { target: ripple; property: "width"; from: 0; to: mainCircle.width * 1.25; duration: 330; easing.type: Easing.OutQuad }
                NumberAnimation { target: ripple; property: "height"; from: 0; to: mainCircle.height * 1.25; duration: 330; easing.type: Easing.OutQuad }
                NumberAnimation { target: ripple; property: "opacity"; from: 0.45; to: 0; duration: 330 }
            }

            MouseArea {
                id: buttonMouseArea
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                hoverEnabled: true
                onPressed: function(mouse) { mouse.accepted = false }
            }

            scale: (root.hovered || buttonMouseArea.containsMouse) && !root.pressed ? 1.015 : (root.pressed ? 0.96 : 1.0)
            Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
        }

        Rectangle {
            id: lockOverlay
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.bottomMargin: 10
            anchors.rightMargin: 10
            width: 34
            height: 34
            radius: 17
            color: "#1C1D21"
            border.color: root.lockedButtonColor
            border.width: 1
            visible: root.isSubscriptionRequired

            opacity: root.isSubscriptionRequired ? 1.0 : 0.0
            Behavior on opacity { NumberAnimation { duration: 220 } }

            Image {
                anchors.centerIn: parent
                source: "qrc:/images/controls/info.svg"
                sourceSize: Qt.size(16, 16)
                layer.enabled: true
                layer.effect: ColorOverlay { color: root.lockedButtonColor }
            }
        }

        Shape {
            id: shape
            width: parent.implicitWidth
            height: parent.implicitHeight
            anchors.centerIn: parent
            layer.enabled: true
            layer.samples: 4
            visible: ConnectionController.isConnectionInProgress

            ShapePath {
                fillColor: "transparent"
                strokeColor: root.progressButtonColor
                strokeWidth: 4
                capStyle: ShapePath.RoundCap

                PathAngleArc {
                    centerX: shape.width / 2
                    centerY: shape.height / 2
                    radiusX: 92
                    radiusY: 92
                    startAngle: -90
                    sweepAngle: 86
                }
            }

            RotationAnimator {
                target: shape
                running: ConnectionController.isConnectionInProgress
                from: 0
                to: 360
                loops: Animation.Infinite
                duration: 900
            }
        }
    }

    contentItem: Item {
        Column {
            spacing: 6
            anchors.centerIn: parent

            Item {
                width: 48
                height: 48
                anchors.horizontalCenter: parent.horizontalCenter
                visible: !root.isSubscriptionRequired

                Image {
                    id: stateIcon
                    anchors.centerIn: parent
                    source: ConnectionController.isConnected
                        ? "qrc:/images/controls/shield-tick.svg"
                        : (ConnectionController.isConnectionInProgress
                            ? "qrc:/images/controls/refresh-cw.svg"
                            : "qrc:/images/controls/shield.svg")
                    sourceSize: Qt.size(42, 42)
                    layer.enabled: true
                    layer.effect: ColorOverlay {
                        color: ConnectionController.isConnected
                            ? root.connectedButtonColor
                            : (ConnectionController.isConnectionInProgress ? root.progressButtonColor : "#A1A1AA")
                    }

                    RotationAnimator {
                        target: stateIcon
                        running: ConnectionController.isConnectionInProgress
                        from: 0
                        to: 360
                        loops: Animation.Infinite
                        duration: 1200
                        onRunningChanged: {
                            if (!running) {
                                stateIcon.rotation = 0
                            }
                        }
                    }
                }
            }

            Text {
                width: root.implicitWidth - 40
                anchors.horizontalCenter: parent.horizontalCenter

                font.family: "PT Root UI VF"
                font.weight: 700
                font.pixelSize: root.isSubscriptionRequired ? 15 : 22

                transform: Translate { x: shakeTranslate.x }

                color: root.isSubscriptionRequired
                    ? root.lockedButtonColor
                    : (root.isErrorState ? "#EF4444" : (ConnectionController.isConnected ? root.connectedButtonColor : (ConnectionController.isConnectionInProgress ? root.progressButtonColor : "#E5E7EB")))

                text: root.text

                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: !root.isSubscriptionRequired
                text: ConnectionController.isConnected
                    ? qsTr("Нажмите для отключения")
                    : qsTr("Нажмите для подключения")
                font.family: "PT Root UI VF"
                font.pixelSize: 12
                color: "#A1A1AA"
                horizontalAlignment: Text.AlignHCenter
            }

            // Sub-label when locked
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: root.isSubscriptionRequired
                text: root.isNotLoggedIn ? qsTr("Требуется вход") : qsTr("Нужна подписка")
                font.family: "PT Root UI VF"
                font.pixelSize: 11
                color: Qt.rgba(107/255, 114/255, 128/255, 0.8)
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }

    function handleConnectClick() {
        if (root.isNotLoggedIn) {
            errorShakeAnim.restart()
            PageController.showNotificationMessage(qsTr("Войдите в аккаунт для подключения"))
            PageController.goToPage(PageEnum.PageFBLinkLogin)
            return
        }
        if (root.isSubscriptionRequired) {
            errorShakeAnim.restart()
            PageController.showNotificationMessage(
                qsTr("Требуется активная подписка для подключения"))
            PageController.goToPage(PageEnum.PageFBLinkSubscription)
            return
        }
        ServersModel.setProcessedServerIndex(ServersModel.defaultIndex)
        ConnectionController.connectButtonClicked()
    }

    onClicked: {
        rippleAnim.restart()
        handleConnectClick()
    }

    Keys.onEnterPressed:  { rippleAnim.restart(); handleConnectClick() }
    Keys.onReturnPressed: { rippleAnim.restart(); handleConnectClick() }
}

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

    property string defaultButtonColor: "#00C8FF"
    property string progressButtonColor: "#33D4FF"
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

        // ── Outer glow ───────────────────────────────────────────
        Rectangle {
            id: outerGlow
            anchors.centerIn: parent
            width: parent.width
            height: parent.height
            radius: width / 2
            color: "transparent"

            layer.enabled: true
            layer.effect: DropShadow {
                transparentBorder: true
                color: root.accentColor
                radius: 30
                samples: 61
                spread: 0.1
                opacity: ConnectionController.isConnectionInProgress ? 0.7 : 0.3
            }

            SequentialAnimation on scale {
                running: ConnectionController.isConnectionInProgress
                loops: Animation.Infinite
                PropertyAnimation { to: 1.05; duration: 1000; easing.type: Easing.InOutSine }
                PropertyAnimation { to: 0.98; duration: 1000; easing.type: Easing.InOutSine }
            }
        }

        // ── Main circle ──────────────────────────────────────────
        Rectangle {
            id: mainCircle
            anchors.fill: parent
            radius: width / 2

            gradient: Gradient {
                GradientStop {
                    position: 0.0
                    color: Qt.lighter(root.accentColor, 1.1)
                }
                GradientStop { position: 1.0; color: FBLinkStyle.color.midnightBlack }
            }

            border.width: root.buttonActiveFocus ? 2 : 1
            border.color: root.accentColor

            // ── Ripple ───────────────────────────────────────────
            Rectangle {
                id: ripple
                anchors.centerIn: parent
                width: 0; height: 0
                radius: width / 2
                color: Qt.rgba(255, 255, 255, 0.2)
                opacity: 0
            }

            ParallelAnimation {
                id: rippleAnim
                NumberAnimation { target: ripple; property: "width";   from: 0; to: mainCircle.width  * 1.5; duration: 400; easing.type: Easing.OutQuad }
                NumberAnimation { target: ripple; property: "height";  from: 0; to: mainCircle.height * 1.5; duration: 400; easing.type: Easing.OutQuad }
                SequentialAnimation {
                    NumberAnimation { target: ripple; property: "opacity"; from: 0.5; to: 0; duration: 400 }
                }
            }

            MouseArea {
                id: buttonMouseArea
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                hoverEnabled: true

                // Let Button's AbstractButton handle the actual click;
                // this MouseArea only provides hover state for visual effects.
                onPressed: function(mouse) { mouse.accepted = false }
            }

            scale: (root.hovered || buttonMouseArea.containsMouse) && !root.pressed ? 1.02 : (root.pressed ? 0.95 : 1.0)
            Behavior on scale {
                NumberAnimation { duration: 150; easing.type: Easing.OutCubic }
            }
        }

        // ── Lock icon overlay (when subscription required) ───────
        Rectangle {
            id: lockOverlay
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.bottomMargin: 10
            anchors.rightMargin: 10
            width: 36; height: 36
            radius: 18
            color: "#1C1D21"
            border.color: root.lockedButtonColor
            border.width: 1.5
            visible: root.isSubscriptionRequired

            opacity: root.isSubscriptionRequired ? 1.0 : 0.0
            Behavior on opacity { NumberAnimation { duration: 250 } }

            Image {
                anchors.centerIn: parent
                source: "qrc:/images/controls/info.svg"
                sourceSize: Qt.size(18, 18)
                layer.enabled: true
                layer.effect: ColorOverlay { color: root.lockedButtonColor }
            }
        }

        // ── Spinner (connecting) ─────────────────────────────────
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
                    radiusX: 93; radiusY: 93
                    startAngle: 0; sweepAngle: 90
                }
            }

            RotationAnimator {
                target: shape
                running: ConnectionController.isConnectionInProgress
                from: 0; to: 360
                loops: Animation.Infinite
                duration: 1000
            }
        }
    }

    contentItem: Item {
        Column {
            spacing: 4
            anchors.centerIn: parent

            // ── Label ─────────────────────────────────────────────────
            Text {
                width: root.implicitWidth - 40
                anchors.horizontalCenter: parent.horizontalCenter

                font.family: "PT Root UI VF"
                font.weight: 700
                font.pixelSize: root.isSubscriptionRequired ? 15 : 19

                transform: Translate { x: shakeTranslate.x }

                color: root.isSubscriptionRequired
                    ? root.lockedButtonColor
                    : (root.isErrorState ? "#EF4444" : (ConnectionController.isConnected ? root.connectedButtonColor : root.defaultButtonColor))

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
                font.pixelSize: 11
                color: Qt.rgba(1, 1, 1, 0.65)
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

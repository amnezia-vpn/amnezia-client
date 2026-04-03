import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

import PageEnum 1.0
import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"

// Subscription banner – shown on the home screen when the user
// is not subscribed.  Slides up from the bottom over the drawer.
Item {
    id: root

    // Show when not logged in OR logged in but no active subscription
    readonly property bool shouldShow: !FBLinkController.isLoggedIn || !FBLinkController.isSubscribed

    // Allow parent to dismiss permanently for this session
    property bool dismissed: false
    readonly property bool visible_: shouldShow && !dismissed

    // Height of the visible card
    readonly property real cardHeight: bannerCard.implicitHeight + 16 + SettingsController.safeAreaBottomMargin

    implicitHeight: visible_ ? cardHeight : 0
    clip: true

    Behavior on implicitHeight {
        NumberAnimation { duration: 320; easing.type: Easing.OutCubic }
    }

    // ── Card ──────────────────────────────────────────────────────
    Rectangle {
        id: bannerCard

        anchors {
            left: parent.left; right: parent.right
            bottom: parent.bottom
            bottomMargin: SettingsController.safeAreaBottomMargin
        }
        anchors.leftMargin: 12
        anchors.rightMargin: 12

        implicitHeight: cardRow.implicitHeight + 28
        radius: 20

        // Dark navy gradient (from icon background)
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(7/255, 30/255, 50/255, 0.97) }
            GradientStop { position: 1.0; color: Qt.rgba(4/255, 18/255, 32/255, 0.97) }
        }

        border.color: Qt.rgba(234/255, 179/255, 8/255, 0.45)
        border.width: 1

        // Amber ambient glow
        layer.enabled: true
        layer.effect: DropShadow {
            color: "#EAB308"
            radius: 24
            samples: 49
            spread: 0.0
            transparentBorder: true
        }

        // ── Dismiss button ───────────────────────────────────────
        Rectangle {
            id: closeBtn
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.topMargin: 10
            anchors.rightMargin: 10
            width: 26; height: 26
            radius: 13
            color: closeMouse.containsMouse ? Qt.rgba(1,1,1,0.15) : Qt.rgba(1,1,1,0.08)

            Behavior on color { ColorAnimation { duration: 120 } }

            Image {
                anchors.centerIn: parent
                source: "qrc:/images/controls/close.svg"
                sourceSize: Qt.size(12, 12)
                layer.enabled: true
                layer.effect: ColorOverlay { color: Qt.rgba(1,1,1,0.6) }
            }

            MouseArea {
                id: closeMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.dismissed = true
            }
        }

        // ── Content ───────────────────────────────────────────────
        RowLayout {
            id: cardRow
            anchors {
                left: parent.left; right: closeBtn.left
                verticalCenter: parent.verticalCenter
            }
            anchors.leftMargin: 16
            anchors.rightMargin: 8
            spacing: 14

            // Shield icon with glow
            Item {
                width: 46; height: 46

                Rectangle {
                    anchors.fill: parent
                    radius: 23
                    color: Qt.rgba(234/255, 179/255, 8/255, 0.18)
                }

                Image {
                    anchors.centerIn: parent
                    source: "qrc:/images/controls/shield-tick.svg"
                    sourceSize: Qt.size(24, 24)
                    layer.enabled: true
                    layer.effect: ColorOverlay { color: "#FDE68A" }

                    SequentialAnimation on scale {
                        loops: Animation.Infinite
                        PropertyAnimation { to: 1.08; duration: 1600; easing.type: Easing.InOutSine }
                        PropertyAnimation { to: 1.0;  duration: 1600; easing.type: Easing.InOutSine }
                    }
                }
            }

            // Text block
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 3

                LabelTextType {
                    text: FBLinkController.isLoggedIn
                        ? qsTr("Активируйте Premium")
                        : qsTr("Войдите или создайте аккаунт")
                    font.pixelSize: 15
                    font.weight: 700
                    color: "#FFFFFF"
                }

                LabelTextType {
                    text: FBLinkController.isLoggedIn
                        ? qsTr("Безлимитный доступ ко всем серверам от 199 ₽/мес")
                        : qsTr("Получите VPN-доступ к 10+ странам за 199 ₽/мес")
                    font.pixelSize: 12
                    color: Qt.rgba(1, 1, 1, 0.65)
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                // CTA button
                Rectangle {
                    Layout.topMargin: 8
                    height: 36
                    width: ctaText.implicitWidth + 32
                    radius: 18

                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: "#FACC15" }
                        GradientStop { position: 1.0; color: "#CA8A04" }
                    }

                    scale: ctaMouse.pressed ? 0.95 : (ctaMouse.containsMouse ? 1.03 : 1.0)
                    Behavior on scale { NumberAnimation { duration: 120 } }

                    LabelTextType {
                        id: ctaText
                        anchors.centerIn: parent
                        text: FBLinkController.isLoggedIn
                            ? qsTr("Подключить Premium")
                            : qsTr("Войти / Зарегистрироваться")
                        font.pixelSize: 13
                        font.weight: 700
                        color: "#FFFFFF"
                    }

                    MouseArea {
                        id: ctaMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (FBLinkController.isLoggedIn) {
                                PageController.goToPage(PageEnum.PageFBLinkSubscription)
                            } else {
                                PageController.goToPage(PageEnum.PageFBLinkLogin)
                            }
                        }
                    }
                }
            }
        }
    }

    // Slide-in animation on first show
    transform: Translate {
        id: slideTranslate
        y: root.visible_ ? 0 : root.cardHeight
        Behavior on y { NumberAnimation { duration: 350; easing.type: Easing.OutBack; easing.overshoot: 0.6 } }
    }

    // Reset dismiss if user logs in/out
    Connections {
        target: FBLinkController
        function onLoginStateChanged() { root.dismissed = false }
        function onSubscriptionChanged() { root.dismissed = false }
    }
}

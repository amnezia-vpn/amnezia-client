import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../Components"

// FBLink VPN — TV subscription / account page.
//
// Read-only view of the subscription state with a back button. Payment flows
// stay on phones/desktops (the user only needs to scan a QR code), so the TV
// surface deliberately stays simple: status, plan, expiry, refresh.
FocusScope {
    id: root

    width: parent ? parent.width : 1920
    height: parent ? parent.height : 1080
    focus: true
    clip: true

    function isOkKey(event) {
        return event.key === Qt.Key_Return
            || event.key === Qt.Key_Enter
            || event.key === Qt.Key_Select
    }

    function goBack() {
        if (StackView.view) {
            StackView.view.pop()
        }
    }

    function formatEndDate() {
        const raw = FBLinkController.subscriptionEndDate || ""
        if (raw === "") {
            return "—"
        }
        // Strip the nanosecond component Go's RFC3339 emits — see CLAUDE.md.
        const isoDate = raw.slice(0, 10)
        const d = new Date(isoDate)
        if (isNaN(d.getTime())) {
            return raw
        }
        return Qt.formatDate(d, "d MMMM yyyy")
    }

    Component.onCompleted: {
        console.log("PageTvSubscription loaded")
        backButton.forceActiveFocus()
        if (FBLinkController.isLoggedIn) {
            FBLinkController.fetchSubscription()
        }
    }

    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Back || event.key === Qt.Key_Escape) {
            root.goBack()
            event.accepted = true
        }
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#08090B" }
            GradientStop { position: 1.0; color: "#0F0A02" }
        }
    }

    Item {
        id: stage
        width: 1920
        height: 1080
        scale: Math.min(1, root.width / width, root.height / height) * 0.94
        anchors.centerIn: parent

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 82
            spacing: 28

            RowLayout {
                Layout.fillWidth: true
                spacing: 22

                Label {
                    Layout.fillWidth: true
                    text: qsTr("Подписка")
                    color: "#F8FAFC"
                    font.pixelSize: 54
                    font.bold: true
                }

                TvButton {
                    id: backButton
                    Layout.preferredWidth: 180
                    Layout.preferredHeight: 76
                    text: qsTr("Назад")
                    tvFontPixelSize: 26
                    KeyNavigation.down: refreshButton
                    onClicked: root.goBack()
                    Keys.onPressed: function(event) {
                        if (root.isOkKey(event)) {
                            clicked()
                            event.accepted = true
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 320
                radius: 28
                color: "#101013"
                border.width: 2
                border.color: FBLinkController.isSubscribed ? "#4ADE80" : "#27272A"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 42
                    spacing: 18

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 18

                        Rectangle {
                            Layout.preferredWidth: 18
                            Layout.preferredHeight: 18
                            radius: 9
                            color: FBLinkController.isSubscribed ? "#4ADE80" : "#A1A1AA"
                        }

                        Label {
                            Layout.fillWidth: true
                            text: FBLinkController.isSubscribed
                                    ? qsTr("Активна")
                                    : qsTr("Не активна")
                            color: FBLinkController.isSubscribed ? "#4ADE80" : "#F87171"
                            font.pixelSize: 36
                            font.bold: true
                        }
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        rowSpacing: 12
                        columnSpacing: 28

                        Label {
                            text: qsTr("Тариф")
                            color: "#A1A1AA"
                            font.pixelSize: 24
                        }
                        Label {
                            Layout.fillWidth: true
                            text: FBLinkController.subscriptionPlan !== ""
                                    ? FBLinkController.subscriptionPlan
                                    : "—"
                            color: "#F8FAFC"
                            font.pixelSize: 24
                            elide: Text.ElideRight
                        }

                        Label {
                            text: qsTr("Email")
                            color: "#A1A1AA"
                            font.pixelSize: 24
                        }
                        Label {
                            Layout.fillWidth: true
                            text: FBLinkController.userEmail !== ""
                                    ? FBLinkController.userEmail
                                    : "—"
                            color: "#F8FAFC"
                            font.pixelSize: 24
                            elide: Text.ElideRight
                        }

                        Label {
                            text: qsTr("Действует до")
                            color: "#A1A1AA"
                            font.pixelSize: 24
                        }
                        Label {
                            Layout.fillWidth: true
                            text: root.formatEndDate()
                            color: "#F8FAFC"
                            font.pixelSize: 24
                            elide: Text.ElideRight
                        }

                        Label {
                            text: qsTr("Автопродление")
                            color: "#A1A1AA"
                            font.pixelSize: 24
                        }
                        Label {
                            Layout.fillWidth: true
                            text: FBLinkController.autoRenew
                                    ? qsTr("включено")
                                    : qsTr("выключено")
                            color: "#F8FAFC"
                            font.pixelSize: 24
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 220
                radius: 24
                color: "#0F0F11"
                border.width: 1
                border.color: "#27272A"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 28
                    spacing: 8

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Управление подпиской")
                        color: "#F8FAFC"
                        font.pixelSize: 28
                        font.bold: true
                    }

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Оплата и смена тарифа доступны в мобильном или десктоп-приложении FBLink VPN, а также в личном кабинете на сайте. После оплаты нажмите «Обновить», чтобы синхронизировать статус подписки на этом ТВ.")
                        color: "#A1A1AA"
                        font.pixelSize: 22
                        wrapMode: Text.WordWrap
                    }
                }
            }

            Item { Layout.fillHeight: true }

            RowLayout {
                Layout.fillWidth: true
                spacing: 22

                TvButton {
                    id: refreshButton
                    Layout.fillWidth: true
                    Layout.preferredHeight: 90
                    text: FBLinkController.isLoading
                            ? qsTr("Обновление...")
                            : qsTr("Обновить статус подписки")
                    tvFontPixelSize: 28
                    enabled: !FBLinkController.isLoading
                    KeyNavigation.up: backButton
                    KeyNavigation.right: signOutButton
                    onClicked: FBLinkController.fetchSubscription()
                    Keys.onPressed: function(event) {
                        if (root.isOkKey(event)) {
                            clicked()
                            event.accepted = true
                        }
                    }
                }

                TvButton {
                    id: signOutButton
                    Layout.fillWidth: true
                    Layout.preferredHeight: 90
                    text: qsTr("Выйти из аккаунта")
                    tvFontPixelSize: 28
                    KeyNavigation.up: backButton
                    KeyNavigation.left: refreshButton
                    onClicked: FBLinkController.logout()
                    Keys.onPressed: function(event) {
                        if (root.isOkKey(event)) {
                            clicked()
                            event.accepted = true
                        }
                    }
                }
            }
        }
    }
}

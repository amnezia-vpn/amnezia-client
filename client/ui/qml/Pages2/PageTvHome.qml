import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../Components"

// FBLink VPN — TV home screen.
//
// Layout:
//   * Top row — branding, connection status, subscription / logout buttons.
//   * Middle — currently selected server card (or a hint to pick one).
//   * Bottom — primary actions: connect / locations / refresh.
//
// All buttons participate in an explicit D-pad chain so the focus is never
// "lost" between rows. Up from the action row goes back to the header row;
// Down from the header row enters the action row.
FocusScope {
    id: root

    width: parent ? parent.width : 1920
    height: parent ? parent.height : 1080
    focus: true
    clip: true

    readonly property string selectedName:
        ServersModel.defaultServerName !== ""
            ? ServersModel.defaultServerName
            : qsTr("Локация не выбрана")
    readonly property string stateText:
        ConnectionController.isConnected
            ? qsTr("Подключено")
            : (ConnectionController.isConnectionInProgress
                ? qsTr("Подключение...")
                : qsTr("Готово к подключению"))
    readonly property color stateColor:
        ConnectionController.isConnected
            ? "#4ADE80"
            : (ConnectionController.isConnectionInProgress ? "#FACC15" : "#A1A1AA")

    readonly property bool hasSubscriptionInfo:
        FBLinkController.subscriptionPlan !== "" || FBLinkController.isSubscribed
    readonly property string subscriptionText:
        FBLinkController.isSubscribed
            ? (FBLinkController.subscriptionPlan !== ""
                ? qsTr("Подписка: ") + FBLinkController.subscriptionPlan
                : qsTr("Подписка активна"))
            : qsTr("Подписка не активна")

    function isOkKey(event) {
        return event.key === Qt.Key_Return
            || event.key === Qt.Key_Enter
            || event.key === Qt.Key_Select
    }

    function openServers() {
        if (StackView.view) {
            StackView.view.push("PageTvServers.qml")
        }
    }

    function openSubscription() {
        if (StackView.view) {
            StackView.view.push("PageTvSubscription.qml")
        }
    }

    Component.onCompleted: {
        console.log("PageTvHome loaded")
        connectButton.forceActiveFocus()
        if (FBLinkController.isLoggedIn) {
            FBLinkController.fetchSubscription()
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
            anchors.margins: 86
            spacing: 32

            // Header row.
            RowLayout {
                Layout.fillWidth: true
                spacing: 24

                Image {
                    Layout.preferredWidth: 104
                    Layout.preferredHeight: 104
                    source: "qrc:/images/fblink_logo.png"
                    fillMode: Image.PreserveAspectFit
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Label {
                        text: "FBLink VPN"
                        color: "#F8FAFC"
                        font.pixelSize: 54
                        font.bold: true
                    }

                    Label {
                        text: root.stateText
                        color: root.stateColor
                        font.pixelSize: 28
                        font.bold: true
                    }
                }

                TvButton {
                    id: subscriptionButton
                    Layout.preferredWidth: 240
                    Layout.preferredHeight: 76
                    text: qsTr("Подписка")
                    tvFontPixelSize: 24
                    KeyNavigation.right: logoutButton
                    KeyNavigation.down: connectButton
                    onClicked: root.openSubscription()
                    Keys.onPressed: function(event) {
                        if (root.isOkKey(event)) {
                            clicked()
                            event.accepted = true
                        }
                    }
                }

                TvButton {
                    id: logoutButton
                    Layout.preferredWidth: 180
                    Layout.preferredHeight: 76
                    text: qsTr("Выйти")
                    tvFontPixelSize: 24
                    KeyNavigation.left: subscriptionButton
                    KeyNavigation.down: refreshButton
                    onClicked: FBLinkController.logout()
                    Keys.onPressed: function(event) {
                        if (root.isOkKey(event)) {
                            clicked()
                            event.accepted = true
                        }
                    }
                }
            }

            // Selected server card.
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 280
                radius: 28
                color: "#101013"
                border.width: 2
                border.color: ServersModel.defaultServerIsVipOnly ? "#FACC15" : "#27272A"

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 42
                    spacing: 36

                    Rectangle {
                        Layout.preferredWidth: 122
                        Layout.preferredHeight: 122
                        radius: 61
                        color: "#201A05"
                        border.width: 2
                        border.color: "#FACC15"

                        Image {
                            anchors.centerIn: parent
                            width: 62
                            height: 62
                            source: "qrc:/images/controls/map-pin.svg"
                            fillMode: Image.PreserveAspectFit
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 14

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 14

                            Label {
                                Layout.fillWidth: true
                                text: root.selectedName
                                color: "#F8FAFC"
                                font.pixelSize: 46
                                font.bold: true
                                elide: Text.ElideRight
                            }

                            Rectangle {
                                visible: ServersModel.defaultServerIsVipOnly
                                Layout.preferredHeight: 48
                                Layout.preferredWidth: 148
                                radius: 24
                                color: "#2A2104"
                                border.width: 1
                                border.color: "#FACC15"

                                Label {
                                    anchors.centerIn: parent
                                    text: qsTr("VIP-сервер")
                                    color: "#FACC15"
                                    font.pixelSize: 21
                                    font.bold: true
                                }
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            text: ServersModel.defaultServerEndpointHost
                            color: "#A1A1AA"
                            font.pixelSize: 24
                            elide: Text.ElideRight
                        }

                        Label {
                            Layout.fillWidth: true
                            text: root.subscriptionText
                            color: FBLinkController.isSubscribed ? "#4ADE80" : "#A1A1AA"
                            font.pixelSize: 22
                        }
                    }
                }
            }

            // Action row.
            RowLayout {
                Layout.fillWidth: true
                spacing: 22

                TvButton {
                    id: connectButton
                    Layout.fillWidth: true
                    Layout.preferredHeight: 110
                    text: ConnectionController.isConnected
                            ? qsTr("Отключиться")
                            : qsTr("Подключиться")
                    tvFontPixelSize: 36
                    KeyNavigation.up: subscriptionButton
                    KeyNavigation.right: locationsButton
                    onClicked: ConnectionController.toggleConnection()
                    Keys.onPressed: function(event) {
                        if (root.isOkKey(event)) {
                            clicked()
                            event.accepted = true
                        }
                    }
                }

                TvButton {
                    id: locationsButton
                    Layout.fillWidth: true
                    Layout.preferredHeight: 110
                    text: qsTr("Локации")
                    tvFontPixelSize: 36
                    KeyNavigation.up: subscriptionButton
                    KeyNavigation.left: connectButton
                    KeyNavigation.right: refreshButton
                    onClicked: root.openServers()
                    Keys.onPressed: function(event) {
                        if (root.isOkKey(event)) {
                            clicked()
                            event.accepted = true
                        }
                    }
                }

                TvButton {
                    id: refreshButton
                    Layout.fillWidth: true
                    Layout.preferredHeight: 110
                    text: FBLinkController.isConfigSyncing
                            ? qsTr("Обновление...")
                            : qsTr("Обновить")
                    tvFontPixelSize: 36
                    enabled: !FBLinkController.isConfigSyncing
                    KeyNavigation.up: logoutButton
                    KeyNavigation.left: locationsButton
                    onClicked: FBLinkController.syncAll()
                    Keys.onPressed: function(event) {
                        if (root.isOkKey(event)) {
                            clicked()
                            event.accepted = true
                        }
                    }
                }
            }

            Item { Layout.fillHeight: true }
        }
    }
}

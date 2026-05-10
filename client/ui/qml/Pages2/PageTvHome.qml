import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

FocusScope {
    id: root

    width: parent ? parent.width : 1920
    height: parent ? parent.height : 1080
    focus: true
    clip: true

    readonly property string selectedName: ServersModel.defaultServerName !== "" ? ServersModel.defaultServerName : "No location selected"
    readonly property string stateText: ConnectionController.isConnected
        ? "Connected"
        : (ConnectionController.isConnectionInProgress ? "Connecting..." : "Ready")

    function isOkKey(event) {
        return event.key === Qt.Key_Return
            || event.key === Qt.Key_Enter
            || event.key === Qt.Key_Select
            || event.key === Qt.Key_Space
    }

    function openServers() {
        if (StackView.view) {
            StackView.view.push("PageTvServers.qml")
        }
    }

    Component.onCompleted: {
        console.log("PageTvHome loaded")
        connectButton.forceActiveFocus()
    }

    Rectangle {
        anchors.fill: parent
        color: "#070707"
    }

    Item {
        id: stage
        width: 1920
        height: 1080
        scale: Math.min(1, root.width / width, root.height / height) * 0.92
        anchors.centerIn: parent

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 86
            spacing: 36

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
                        color: ConnectionController.isConnected ? "#4ADE80" : "#FACC15"
                        font.pixelSize: 30
                        font.bold: true
                    }
                }

                Button {
                    id: logoutButton
                    text: "Logout"
                    font.pixelSize: 26
                    padding: 20
                    highlighted: activeFocus
                    KeyNavigation.down: locationsButton
                    onClicked: FBLinkController.logout()
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
                Layout.preferredHeight: 300
                radius: 28
                color: "#121212"
                border.width: 3
                border.color: ServersModel.defaultServerIsVipOnly ? "#FACC15" : "#2F2F2F"

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
                        spacing: 16

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
                                    text: "VIP server"
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
                            font.pixelSize: 26
                            elide: Text.ElideRight
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 22

                Button {
                    id: connectButton
                    Layout.fillWidth: true
                    Layout.preferredHeight: 96
                    text: ConnectionController.isConnected ? "Disconnect" : "Connect"
                    font.pixelSize: 34
                    highlighted: activeFocus
                    KeyNavigation.up: logoutButton
                    KeyNavigation.right: locationsButton
                    onClicked: ConnectionController.toggleConnection()
                    Keys.onPressed: function(event) {
                        if (root.isOkKey(event)) {
                            clicked()
                            event.accepted = true
                        }
                    }
                }

                Button {
                    id: locationsButton
                    Layout.fillWidth: true
                    Layout.preferredHeight: 96
                    text: "Locations"
                    font.pixelSize: 34
                    highlighted: activeFocus
                    KeyNavigation.up: logoutButton
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

                Button {
                    id: refreshButton
                    Layout.fillWidth: true
                    Layout.preferredHeight: 96
                    text: FBLinkController.isConfigSyncing ? "Refreshing..." : "Refresh"
                    font.pixelSize: 34
                    enabled: !FBLinkController.isConfigSyncing
                    highlighted: activeFocus
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

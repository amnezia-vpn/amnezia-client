import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

Item {
    id: root

    anchors.fill: parent
    focus: true

    readonly property string selectedName: ServersModel.defaultServerName !== "" ? ServersModel.defaultServerName : "No location selected"
    readonly property string stateText: ConnectionController.isConnected
        ? "Connected"
        : (ConnectionController.isConnectionInProgress ? "Connecting..." : "Ready")

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
        color: "#08090B"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 64
        spacing: 34

        RowLayout {
            Layout.fillWidth: true

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8

                Label {
                    text: "FBLink VPN"
                    color: "#F8FAFC"
                    font.pixelSize: 44
                    font.bold: true
                }

                Label {
                    text: root.stateText
                    color: ConnectionController.isConnected ? "#4ADE80" : "#A1A1AA"
                    font.pixelSize: 26
                }
            }

            Button {
                id: logoutButton
                text: "Logout"
                font.pixelSize: 22
                padding: 16
                KeyNavigation.down: connectButton
                onClicked: FBLinkController.logout()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 230
            radius: 24
            color: "#121318"
            border.width: 2
            border.color: ServersModel.defaultServerIsVipOnly ? "#EAB308" : "#30333B"

            RowLayout {
                anchors.fill: parent
                anchors.margins: 34
                spacing: 28

                Rectangle {
                    Layout.preferredWidth: 88
                    Layout.preferredHeight: 88
                    radius: 44
                    color: "#1F2937"

                    Label {
                        anchors.centerIn: parent
                        text: "VPN"
                        color: "#FACC15"
                        font.pixelSize: 24
                        font.bold: true
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 14

                        Label {
                            Layout.fillWidth: true
                            text: root.selectedName
                            color: "#F8FAFC"
                            font.pixelSize: 38
                            font.bold: true
                            elide: Text.ElideRight
                        }

                        Rectangle {
                            visible: ServersModel.defaultServerIsVipOnly
                            Layout.preferredHeight: 40
                            Layout.preferredWidth: 88
                            radius: 20
                            color: "#3B2F05"
                            border.color: "#EAB308"

                            Label {
                                anchors.centerIn: parent
                                text: "VIP"
                                color: "#FACC15"
                                font.pixelSize: 20
                                font.bold: true
                            }
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: ServersModel.defaultServerEndpointHost
                        color: "#A1A1AA"
                        font.pixelSize: 22
                        elide: Text.ElideRight
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 18

            Button {
                id: connectButton
                Layout.fillWidth: true
                Layout.preferredHeight: 86
                text: ConnectionController.isConnected ? "Disconnect" : "Connect"
                font.pixelSize: 30
                highlighted: true
                KeyNavigation.up: logoutButton
                KeyNavigation.right: locationsButton
                onClicked: ConnectionController.toggleConnection()
            }

            Button {
                id: locationsButton
                Layout.fillWidth: true
                Layout.preferredHeight: 86
                text: "Locations"
                font.pixelSize: 30
                KeyNavigation.up: logoutButton
                KeyNavigation.left: connectButton
                KeyNavigation.right: refreshButton
                onClicked: root.openServers()
            }

            Button {
                id: refreshButton
                Layout.fillWidth: true
                Layout.preferredHeight: 86
                text: FBLinkController.isLoading ? "Refreshing..." : "Refresh"
                font.pixelSize: 30
                enabled: !FBLinkController.isLoading
                KeyNavigation.up: logoutButton
                KeyNavigation.left: locationsButton
                onClicked: FBLinkController.syncAll()
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}

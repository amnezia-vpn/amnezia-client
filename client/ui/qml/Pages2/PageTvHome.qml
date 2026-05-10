import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

FocusScope {
    id: root

    anchors.fill: parent
    focus: true

    readonly property string selectedName: ServersModel.defaultServerName !== "" ? ServersModel.defaultServerName : "Локация не выбрана"
    readonly property string stateText: ConnectionController.isConnected
        ? "Подключено"
        : (ConnectionController.isConnectionInProgress ? "Подключение..." : "Готово")

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

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 64
        spacing: 34

        RowLayout {
            Layout.fillWidth: true
            spacing: 24

            Image {
                Layout.preferredWidth: 92
                Layout.preferredHeight: 92
                source: "qrc:/images/fblink_logo.png"
                fillMode: Image.PreserveAspectFit
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6

                Label {
                    text: "FBLink VPN"
                    color: "#F8FAFC"
                    font.pixelSize: 46
                    font.bold: true
                }

                Label {
                    text: root.stateText
                    color: ConnectionController.isConnected ? "#4ADE80" : "#FACC15"
                    font.pixelSize: 28
                    font.bold: true
                }
            }

            Button {
                id: logoutButton
                text: "Выйти"
                font.pixelSize: 24
                padding: 18
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
            Layout.preferredHeight: 280
            radius: 28
            color: "#121212"
            border.width: 3
            border.color: ServersModel.defaultServerIsVipOnly ? "#FACC15" : "#2F2F2F"

            RowLayout {
                anchors.fill: parent
                anchors.margins: 38
                spacing: 34

                Rectangle {
                    Layout.preferredWidth: 116
                    Layout.preferredHeight: 116
                    radius: 58
                    color: "#201A05"
                    border.width: 2
                    border.color: "#FACC15"

                    Image {
                        anchors.centerIn: parent
                        width: 58
                        height: 58
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
                            font.pixelSize: 42
                            font.bold: true
                            elide: Text.ElideRight
                        }

                        Rectangle {
                            visible: ServersModel.defaultServerIsVipOnly
                            Layout.preferredHeight: 44
                            Layout.preferredWidth: 138
                            radius: 22
                            color: "#2A2104"
                            border.width: 1
                            border.color: "#FACC15"

                            Label {
                                anchors.centerIn: parent
                                text: "VIP server"
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
                        font.pixelSize: 24
                        elide: Text.ElideRight
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 20

            Button {
                id: connectButton
                Layout.fillWidth: true
                Layout.preferredHeight: 92
                text: ConnectionController.isConnected ? "Отключить" : "Подключить"
                font.pixelSize: 32
                highlighted: true
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
                Layout.preferredHeight: 92
                text: "Локации"
                font.pixelSize: 32
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
                Layout.preferredHeight: 92
                text: FBLinkController.isConfigSyncing ? "Обновляем..." : "Обновить"
                font.pixelSize: 32
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

        Item {
            Layout.fillHeight: true
        }
    }
}

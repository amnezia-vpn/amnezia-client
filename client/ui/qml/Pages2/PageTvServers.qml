import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

FocusScope {
    id: root

    anchors.fill: parent
    focus: true

    function isOkKey(event) {
        return event.key === Qt.Key_Return
            || event.key === Qt.Key_Enter
            || event.key === Qt.Key_Select
            || event.key === Qt.Key_Space
    }

    function goBack() {
        if (StackView.view) {
            StackView.view.pop()
        }
    }

    Component.onCompleted: {
        console.log("PageTvServers loaded")
        serverList.currentIndex = Math.max(0, ServersModel.defaultIndex)
        serverList.forceActiveFocus()
        if (FBLinkController.isLoggedIn) {
            FBLinkController.syncAll()
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "#070707"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 58
        spacing: 26

        RowLayout {
            Layout.fillWidth: true
            spacing: 22

            Label {
                Layout.fillWidth: true
                text: "Локации"
                color: "#F8FAFC"
                font.pixelSize: 46
                font.bold: true
            }

            Button {
                id: backButton
                text: "Назад"
                font.pixelSize: 24
                padding: 18
                highlighted: activeFocus
                KeyNavigation.down: serverList
                onClicked: root.goBack()
                Keys.onPressed: function(event) {
                    if (root.isOkKey(event)) {
                        clicked()
                        event.accepted = true
                    }
                }
            }
        }

        ListView {
            id: serverList
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: ServersModel
            spacing: 14
            clip: true
            focus: true
            KeyNavigation.up: backButton

            Keys.onPressed: function(event) {
                if (event.key === Qt.Key_Down) {
                    currentIndex = Math.min(count - 1, currentIndex + 1)
                    positionViewAtIndex(currentIndex, ListView.Contain)
                    event.accepted = true
                } else if (event.key === Qt.Key_Up) {
                    if (currentIndex <= 0) {
                        backButton.forceActiveFocus()
                    } else {
                        currentIndex = currentIndex - 1
                        positionViewAtIndex(currentIndex, ListView.Contain)
                    }
                    event.accepted = true
                } else if (root.isOkKey(event)) {
                    selectCurrent()
                    event.accepted = true
                } else if (event.key === Qt.Key_Back || event.key === Qt.Key_Escape) {
                    root.goBack()
                    event.accepted = true
                }
            }

            function selectCurrent() {
                if (currentIndex < 0 || currentIndex >= count) {
                    return
                }
                const item = itemAtIndex(currentIndex)
                if (item) {
                    item.selectServer()
                }
            }

            delegate: Rectangle {
                id: row

                required property int index
                required property string name
                required property string serverDescription
                required property string apiServerCountryCode
                required property bool isVipOnly
                required property bool isAvailableForCurrentPlan

                readonly property bool selected: serverList.currentIndex === index
                readonly property bool locked: isVipOnly && !isAvailableForCurrentPlan

                width: serverList.width
                height: 104
                radius: 20
                color: selected ? "#251F07" : "#121212"
                border.width: selected ? 3 : 1
                border.color: selected ? "#FACC15" : "#2F2F2F"
                opacity: locked ? 0.55 : 1.0

                function selectServer() {
                    if (locked) {
                        PageController.showNotificationMessage("Этот сервер доступен только VIP")
                        return
                    }
                    if (ConnectionController.isConnected) {
                        PageController.showNotificationMessage("Сначала отключите VPN")
                        return
                    }
                    ServersModel.defaultIndex = index
                    ServersModel.processedIndex = index
                    root.goBack()
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        serverList.currentIndex = index
                        row.selectServer()
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 22
                    spacing: 20

                    Rectangle {
                        Layout.preferredWidth: 62
                        Layout.preferredHeight: 62
                        radius: 31
                        color: "#1A1A1A"
                        border.width: 1
                        border.color: row.selected ? "#FACC15" : "#343434"

                        Image {
                            anchors.centerIn: parent
                            width: 42
                            height: 30
                            source: apiServerCountryCode !== ""
                                ? "qrc:/countriesFlags/images/flagKit/" + apiServerCountryCode + ".svg"
                                : "qrc:/images/controls/map-pin.svg"
                            fillMode: Image.PreserveAspectFit
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Label {
                            Layout.fillWidth: true
                            text: name
                            color: "#F8FAFC"
                            font.pixelSize: 28
                            font.bold: row.selected
                            elide: Text.ElideRight
                        }

                        Label {
                            Layout.fillWidth: true
                            text: row.locked ? "Только для VIP" : serverDescription
                            color: row.locked ? "#FACC15" : "#A1A1AA"
                            font.pixelSize: 20
                            elide: Text.ElideRight
                        }
                    }

                    Rectangle {
                        visible: isVipOnly
                        Layout.preferredWidth: 94
                        Layout.preferredHeight: 38
                        radius: 19
                        color: "#2A2104"
                        border.width: 1
                        border.color: "#FACC15"

                        Label {
                            anchors.centerIn: parent
                            text: row.locked ? "LOCK" : "VIP"
                            color: "#FACC15"
                            font.pixelSize: 18
                            font.bold: true
                        }
                    }

                    Image {
                        visible: ServersModel.defaultIndex === index
                        Layout.preferredWidth: 32
                        Layout.preferredHeight: 32
                        source: "qrc:/images/controls/check.svg"
                        fillMode: Image.PreserveAspectFit
                    }
                }
            }
        }
    }
}

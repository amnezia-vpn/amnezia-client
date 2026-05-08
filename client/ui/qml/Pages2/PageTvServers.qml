import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    anchors.fill: parent
    focus: true

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
        color: "#08090B"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 54
        spacing: 24

        RowLayout {
            Layout.fillWidth: true

            Label {
                Layout.fillWidth: true
                text: "Locations"
                color: "#F8FAFC"
                font.pixelSize: 44
                font.bold: true
            }

            Button {
                text: "Back"
                font.pixelSize: 22
                padding: 16
                onClicked: root.goBack()
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

            Keys.onPressed: function(event) {
                if (event.key === Qt.Key_Down) {
                    currentIndex = Math.min(count - 1, currentIndex + 1)
                    positionViewAtIndex(currentIndex, ListView.Contain)
                    event.accepted = true
                } else if (event.key === Qt.Key_Up) {
                    currentIndex = Math.max(0, currentIndex - 1)
                    positionViewAtIndex(currentIndex, ListView.Contain)
                    event.accepted = true
                } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_Space) {
                    selectCurrent()
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

                width: serverList.width
                height: 92
                radius: 18
                color: serverList.currentIndex === index ? "#242712" : "#121318"
                border.width: serverList.currentIndex === index ? 3 : 1
                border.color: serverList.currentIndex === index ? "#EAB308" : "#30333B"
                opacity: isVipOnly && !isAvailableForCurrentPlan ? 0.58 : 1.0

                function selectServer() {
                    if (isVipOnly && !isAvailableForCurrentPlan) {
                        PageController.showNotificationMessage("VIP only server")
                        return
                    }
                    if (ConnectionController.isConnected) {
                        PageController.showNotificationMessage("Disconnect before changing location")
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
                    anchors.margins: 20
                    spacing: 18

                    Image {
                        Layout.preferredWidth: 46
                        Layout.preferredHeight: 32
                        source: apiServerCountryCode !== ""
                            ? "qrc:/countriesFlags/images/flagKit/" + apiServerCountryCode + ".svg"
                            : "qrc:/images/controls/map-pin.svg"
                        fillMode: Image.PreserveAspectFit
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        Label {
                            Layout.fillWidth: true
                            text: name
                            color: "#F8FAFC"
                            font.pixelSize: 26
                            font.bold: serverList.currentIndex === index
                            elide: Text.ElideRight
                        }

                        Label {
                            Layout.fillWidth: true
                            text: isVipOnly && !isAvailableForCurrentPlan ? "VIP only" : serverDescription
                            color: "#A1A1AA"
                            font.pixelSize: 18
                            elide: Text.ElideRight
                        }
                    }

                    Rectangle {
                        visible: isVipOnly
                        Layout.preferredWidth: 78
                        Layout.preferredHeight: 36
                        radius: 18
                        color: "#3B2F05"
                        border.color: "#EAB308"

                        Label {
                            anchors.centerIn: parent
                            text: "VIP"
                            color: "#FACC15"
                            font.pixelSize: 18
                            font.bold: true
                        }
                    }

                    Label {
                        visible: ServersModel.defaultIndex === index
                        text: "OK"
                        color: "#FACC15"
                        font.pixelSize: 22
                        font.bold: true
                    }
                }
            }
        }
    }
}

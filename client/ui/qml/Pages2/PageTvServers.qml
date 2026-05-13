import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../Components"

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

    Component.onCompleted: {
        console.log("PageTvServers loaded")
        serverList.currentIndex = Math.max(0, ServersModel.defaultIndex)
        serverList.forceActiveFocus()
        if (FBLinkController.isLoggedIn) {
            FBLinkController.syncAll()
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
        scale: Math.min(1, root.width / width, root.height / height) * 0.92
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
                    text: qsTr("Локации")
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
                    height: 112
                    radius: 20
                    color: selected ? "#251F07" : "#121212"
                    border.width: selected ? 3 : 1
                    border.color: selected ? "#FACC15" : "#2F2F2F"
                    opacity: locked ? 0.55 : 1.0

                    function selectServer() {
                        if (locked) {
                            PageController.showNotificationMessage(qsTr("Доступно только с VIP-подпиской"))
                            return
                        }
                        if (ConnectionController.isConnected) {
                            PageController.showNotificationMessage(qsTr("Сначала отключите VPN"))
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
                        anchors.margins: 24
                        spacing: 20

                        Rectangle {
                            Layout.preferredWidth: 66
                            Layout.preferredHeight: 66
                            radius: 33
                            color: "#1A1A1A"
                            border.width: 1
                            border.color: row.selected ? "#FACC15" : "#343434"

                            Image {
                                anchors.centerIn: parent
                                width: 44
                                height: 32
                                source: apiServerCountryCode !== ""
                                    ? "qrc:/countriesFlags/images/flagKit/" + apiServerCountryCode + ".svg"
                                    : "qrc:/images/controls/map-pin.svg"
                                fillMode: Image.PreserveAspectFit
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 7

                            Label {
                                Layout.fillWidth: true
                                text: name
                                color: "#F8FAFC"
                                font.pixelSize: 30
                                font.bold: row.selected
                                elide: Text.ElideRight
                            }

                            Label {
                                Layout.fillWidth: true
                                text: row.locked ? qsTr("Только для VIP") : serverDescription
                                color: row.locked ? "#FACC15" : "#A1A1AA"
                                font.pixelSize: 22
                                elide: Text.ElideRight
                            }
                        }

                        Rectangle {
                            visible: isVipOnly
                            Layout.preferredWidth: 96
                            Layout.preferredHeight: 40
                            radius: 20
                            color: "#2A2104"
                            border.width: 1
                            border.color: "#FACC15"

                            Label {
                                anchors.centerIn: parent
                                text: row.locked ? qsTr("LOCK") : "VIP"
                                color: "#FACC15"
                                font.pixelSize: 18
                                font.bold: true
                            }
                        }

                        Image {
                            visible: ServersModel.defaultIndex === index
                            Layout.preferredWidth: 34
                            Layout.preferredHeight: 34
                            source: "qrc:/images/controls/check.svg"
                            fillMode: Image.PreserveAspectFit
                        }
                    }
                }
            }
        }
    }
}

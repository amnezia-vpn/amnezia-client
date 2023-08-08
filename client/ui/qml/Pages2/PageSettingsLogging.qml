import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0

import "./"
import "../Controls2"
import "../Config"
import "../Controls2/TextTypes"

PageType {
    id: root

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20
    }

    FlickableType {
        id: fl
        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        contentHeight: content.height

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 16
            anchors.rightMargin: 16

            spacing: 16

            HeaderType {
                Layout.fillWidth: true

                headerText: qsTr("Logging")
            }

            SwitcherType {
                Layout.fillWidth: true
                Layout.topMargin: 16

                text: qsTr("Save logs")

                checked: SettingsController.isLoggingEnabled
                onCheckedChanged: {
                    if (checked !== SettingsController.isLoggingEnabled) {
                        SettingsController.isLoggingEnabled = checked
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true

                ColumnLayout {
                    Layout.alignment: Qt.AlignBaseline
                    Layout.preferredWidth: root.width / 3

                    ImageButtonType {
                        Layout.alignment: Qt.AlignHCenter

                        implicitWidth: 56
                        implicitHeight: 56

                        image: "qrc:/images/controls/folder-open.svg"

                        onClicked: SettingsController.openLogsFolder()
                    }

                    CaptionTextType {
                        horizontalAlignment: Text.AlignHCenter
                        Layout.fillWidth: true

                        text: qsTr("Open folder with logs")
                        color: "#D7D8DB"
                    }
                }

                ColumnLayout {
                    Layout.alignment: Qt.AlignBaseline
                    Layout.preferredWidth: root.width / 3

                    ImageButtonType {
                        Layout.alignment: Qt.AlignHCenter

                        implicitWidth: 56
                        implicitHeight: 56

                        image: "qrc:/images/controls/save.svg"

                        onClicked: SettingsController.exportLogsFile()
                    }

                    CaptionTextType {
                        horizontalAlignment: Text.AlignHCenter
                        Layout.fillWidth: true

                        text: qsTr("Save logs to file")
                        color: "#D7D8DB"
                    }
                }

                ColumnLayout {
                    Layout.alignment: Qt.AlignBaseline
                    Layout.preferredWidth: root.width / 3

                    ImageButtonType {
                        Layout.alignment: Qt.AlignHCenter

                        implicitWidth: 56
                        implicitHeight: 56

                        image: "qrc:/images/controls/delete.svg"

                        onClicked: SettingsController.clearLogs()
                    }

                    CaptionTextType {
                        horizontalAlignment: Text.AlignHCenter
                        Layout.fillWidth: true

                        text: qsTr("Clear logs")
                        color: "#D7D8DB"
                    }
                }
            }
        }
    }
}

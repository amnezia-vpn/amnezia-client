pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Config 1.0

import "../Components"
import "../Controls"
import "../Controls/TextTypes"

Page {
    id: root

    Connections {
        target: ImportController

        function onImportErrorOccurred(error, goToPageHome) {
            PageController.showErrorMessage(error)
        }

        function onImportFinished() {
            if (!ConnectionController.isConnected) {
                ServersModel.setDefaultServerIndex(ServersModel.getServersCount() - 1);
                ServersModel.processedIndex = ServersModel.defaultIndex
            }

            PageController.goToStartPage()
        }
    }

    ColumnLayout {
        anchors.fill: parent

        spacing: 0

        RowLayout {
            Layout.leftMargin: 8
            Layout.rightMargin: 8
            Layout.topMargin: 8

            WhiteButtonNoBorder {
                id: backButton
                imageSource: "qrc:/images/controls/arrow-left.svg"

                onClicked: PageController.closePage()
            }

            Item {
                Layout.fillWidth: true
            }
        }

        Header1TextType {
            id: header

            Layout.topMargin: 8
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.bottomMargin: 24
            Layout.fillWidth: true

            text: qsTr("Adding a server to connect to")

            horizontalAlignment: Qt.AlignLeft
            verticalAlignment: Qt.AlignVCenter
        }

        XSmallTextType {
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.bottomMargin: 8
            Layout.fillWidth: true

            text: qsTr("Key")
        }

        InputType {
            id: textKey

            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.fillWidth: true
            Layout.preferredHeight: 240

            placeholderText: qsTr("VPN://")
        }

        WhiteButtonWithBorder {
            Layout.topMargin: 12
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.fillWidth: true

            text: qsTr("Insert")

            onClicked: function() {
                textKey.text = ""
                textKey.paste()
            }
        }

        BlueButtonNoBorder {
            Layout.topMargin: 12
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.fillWidth: true

            text: qsTr("Add")

            onClicked: function() {
                if (ImportController.extractConfigFromData(textKey.text)) {
                    ImportController.importConfig()
                } else {
                    PageController.showErrorMessage(qsTr("Unsupported config file"))
                }
            }
        }

        BlackButtonWithBorder {
            id: importConfigButton
            
            Layout.topMargin: 12
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.fillWidth: true

            text: qsTr("Import config from file")

            onClicked: function() {
                var nameFilter = !ServersModel.getServersCount() ? "Config or backup files (*.vpn *.ovpn *.conf *.json *.backup)" :
                                                               "Config files (*.vpn *.ovpn *.conf *.json)"
                var fileName = SystemController.getFileName(qsTr("Open config file"), nameFilter)
                if (fileName !== "") {
                    if (ImportController.extractConfigFromFile(fileName)) {
                        ImportController.importConfig()
                    }
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}

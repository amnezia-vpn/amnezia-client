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
        target: InstallController

        function onRemoveProcessedServerFinished(finishedMessage) {
            if (!ServersModel.getServersCount()) {
                PageController.goToStartPage()
            } else {
                PageController.closePage()
            }
            PageController.showNotificationMessage(finishedMessage)
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

            text: qsTr("Server settings")

            horizontalAlignment: Qt.AlignLeft
            verticalAlignment: Qt.AlignVCenter
        }

        XSmallTextType {
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.bottomMargin: 8
            Layout.fillWidth: true

            text: qsTr("Name")
        }

        InputType {
            id: textKey

            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.fillWidth: true
        }

        WhiteButtonWithBorder {
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.topMargin: 24
            Layout.fillWidth: true

            text: qsTr("Remove server")

            onClicked: function() {
                PageController.showBusyIndicator(true)
                InstallController.removeProcessedServer()
                PageController.showBusyIndicator(false)
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}

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

            text: ServersModel.getProcessedServerData("name") + " " + qsTr("Server settings")

            horizontalAlignment: Qt.AlignLeft
            verticalAlignment: Qt.AlignVCenter
        }

        XSmallTextType {
            visible: false

            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.bottomMargin: 8
            Layout.fillWidth: true

            text: qsTr("Name")
        }

        InputType {
            visible: false

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

            text: qsTr("Rename server")
            defaultTextColor: Style.color.black
            hoveredTextColor: Style.color.black
            pressedTextColor: Style.color.black

            onClicked: renameServerPopup.open()
        }

        WhiteButtonWithBorder {
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.topMargin: 12
            Layout.fillWidth: true

            text: qsTr("Delete server")
            defaultTextColor: Style.color.error
            hoveredTextColor: Style.color.error
            pressedTextColor: Style.color.error

            onClicked: deleteConfirmationDialog.open()
        }

        Item {
            Layout.fillHeight: true
        }
    }

    Popup {
        id: renameServerPopup

        property string serverName: ServersModel.getProcessedServerData("name")

        modal: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        anchors.centerIn: parent
        width: parent.width - 30
        padding: 24

        background: Rectangle {
            color: Style.color.white
            radius: 20
            border.width: 1
            border.color: Style.color.gray2
        }

        contentItem: ColumnLayout {
            spacing: 24

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 16

                Header3TextType {
                    Layout.fillWidth: true
                    text: qsTr("Server name")
                    horizontalAlignment: Text.AlignVCenter
                }

                InputType {
                    id: serverNameInput
                    Layout.fillWidth: true
                    text: renameServerPopup.serverName
                    placeholderText: qsTr("Enter server name")
                    onAccepted: {
                        if (serverNameInput.text.trim() !== "") {
                            ServersModel.setProcessedServerData("name", serverNameInput.text.trim())
                            PageController.showNotificationMessage(qsTr("Server renamed successfully"))
                            header.text = serverNameInput.text.trim() + " " + qsTr("Server settings")
                        }
                        renameServerPopup.close()
                    }
                }
            }

            BlueButtonNoBorder {
                Layout.fillWidth: true
                text: qsTr("Save")
                onClicked: {
                    if (serverNameInput.text.trim() !== "") {
                        ServersModel.setProcessedServerData("name", serverNameInput.text.trim())
                        PageController.showNotificationMessage(qsTr("Server renamed successfully"))
                        header.text = serverNameInput.text.trim() + " " + qsTr("Server settings")
                    }
                    renameServerPopup.close()
                }
            }
        }

        Overlay.modal: Item {
            anchors.fill: parent

            ShaderEffectSource {
                id: blurSource
                anchors.fill: parent
                sourceItem: renameServerPopup.parent
            }

            Rectangle {
                anchors.fill: parent
                color: Style.color.transparentWhite
            }
        }
    }

    ConfirmationDialog {
        id: deleteConfirmationDialog
        title: qsTr("Are you sure you want to remove the server from the app?")
        description: qsTr("You won't be able to connect to it")
        confirmButtonText: qsTr("Yes, delete anyway")
        cancelButtonText: qsTr("No, keep it")
        
        onConfirm: function() {
            PageController.showBusyIndicator(true)
            InstallController.removeProcessedServer()
            PageController.showBusyIndicator(false)
        }
    }
}

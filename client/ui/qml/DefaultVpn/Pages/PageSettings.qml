import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

import QtCore

import PageEnum 1.0
import Config 1.0

import "../Components"
import "../Controls"
import "../Controls/TextTypes"

Page {
    id: root

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        anchors.leftMargin: 8
        anchors.rightMargin: 8

        RowLayout {
            Layout.topMargin: 8
            WhiteButtonNoBorder {
                id: backButton
                imageSource: "qrc:/images/controls/arrow-left.svg"
                onClicked: PageController.closePage()
            }
        }

        Header1TextType {
            Layout.topMargin: 8
            Layout.leftMargin: 8
            Layout.rightMargin: 8
            Layout.fillWidth: true
            text: qsTr("Settings")
            horizontalAlignment: Qt.AlignLeft
            verticalAlignment: Qt.AlignVCenter
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.topMargin: 24

            SettingsButtonNoBorder {
                buttonText: qsTr("Language")
                onButtonClicked: PageController.goToPage(PageEnum.PageSettingsLanguage)
            }

            SettingsButtonNoBorder {
                buttonText: qsTr("Logging")
                onButtonClicked: PageController.goToPage(PageEnum.PageSettingsLogging)
            }

            SettingsButtonNoBorder {
                buttonText: qsTr("About")
                onButtonClicked: PageController.goToPage(PageEnum.PageAbout)
            }

            SettingsButtonNoBorder {
                buttonText: qsTr("Reset settings and remove all data from the application")
                buttonTextColor: Style.color.error
                showArrow: false
                onButtonClicked: function() {
                    if (ServersModel.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected) {
                        PageController.showNotificationMessage(qsTr("Cannot reset settings during active connection"))
                    } else {
                        resetConfirmationDialog.open()
                    }
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }

    ConfirmationDialog {
        id: resetConfirmationDialog
        title: qsTr("Reset settings and remove all data from the application?")
        description: qsTr("All settings will be reset to default. All installed DefaultVPN services will still remain on the server.")
        confirmButtonText: qsTr("Continue")
        cancelButtonText: qsTr("Cancel")
        onConfirm: function() {
            SettingsController.clearSettings()
            PageController.goToStartPage()
        }
    }
}

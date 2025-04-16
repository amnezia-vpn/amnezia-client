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

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 8
        anchors.bottomMargin: 36
        anchors.leftMargin: 16
        anchors.rightMargin: 16

        spacing: 0

        RowLayout {
            Layout.fillWidth: true
                    
            Text {
                Layout.fillWidth: true
                lineHeight: 68
                lineHeightMode: Text.FixedHeight

                color: Style.color.gray2
                font.pixelSize: 56
                font.weight: 700
                font.family: Style.font

                horizontalAlignment: Qt.AlignLeft

                text: ConnectionController.isConnected ? qsTr("Online") : qsTr("Offline")
            }

            WhiteButtonNoBorder {
                Layout.rightMargin: -8
                Layout.topMargin: -16
                imageSource: "qrc:/images/controls/settings.svg"
                onClicked: PageController.goToPage(PageEnum.PageSettings)
            }
        }

        Item {
            Layout.fillHeight: true
        }

        XSmallTextType {
            text: qsTr("Connection to")

            horizontalAlignment: Qt.AlignLeft
            verticalAlignment: Qt.AlignVCenter
        }

        RowLayout {
            DropDownType {
                id: defaultServerDropDown
                Layout.fillWidth: true

                text: ServersModel.defaultServerName

                onClicked: function() {
                    PageController.goToPage(PageEnum.PageSettingsServersList)
                }
            }

            WhiteButtonWithBorder {
                imageSource: "qrc:/images/controls/plus.svg"

                onClicked: function() {
                    PageController.goToPage(PageEnum.PageSetupWizardConfigSource)
                }
            }
        }

        DropDownType {
            id: countryDropDown
            Layout.fillWidth: false
            Layout.topMargin: 10
            Layout.preferredWidth: defaultServerDropDown.width

            visible: ServersModel.defaultServerImagePathCollapsed !== ""

            text: ServersModel.defaultServerDescriptionCollapsed

            onClicked: function() {
                if (ConnectionController.isConnected) {
                    PageController.showNotificationMessage(qsTr("Unable change server location while there is an active connection"))
                    return
                }
                ServersModel.setProcessedServerIndex(ServersModel.defaultIndex)
                PageController.goToPage(PageEnum.PageCountrySelector)
            }
        }

        Button {
            id: connectButton

            Layout.fillWidth: true
            implicitHeight: 358

            Layout.topMargin: 16

            background: Rectangle {
                anchors.fill: parent

                radius: 16

                color: {
                    if (ConnectionController.isConnectionInProgress) {
                        return Style.color.accent3
                    } else if (ConnectionController.isConnected) {
                        return Style.color.accent1
                    } else {
                        return Style.color.black
                    }
                }

                ColumnLayout {
                    anchors.centerIn: parent

                    Image {
                        Layout.alignment: Qt.AlignCenter

                        source: "qrc:/images/controls/connect-button.svg"
                    }

                    Header3TextType {
                        Layout.alignment: Qt.AlignCenter
                        Layout.topMargin: 24

                        text: ConnectionController.connectionStateText

                        color: Style.color.white
                    }

                    Item {
                        Layout.fillWidth: true
                    }
                }
            }

            onClicked: function() {
                ServersModel.setProcessedServerIndex(ServersModel.defaultIndex)
                ConnectionController.connectButtonClicked()
            }
        }
    }
}

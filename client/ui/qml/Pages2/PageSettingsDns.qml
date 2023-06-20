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
        anchors.rightMargin: 16
        anchors.leftMargin: 16
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

                headerText: qsTr("DNS servers")
            }

            ParagraphTextType {
                text: qsTr("If AmneziaDNS is not used or installed")
            }

            TextFieldWithHeaderType {
                id: primaryDns

                Layout.fillWidth: true
                headerText: "Primary DNS"

                textFieldText: SettingsController.primaryDns
            }

            TextFieldWithHeaderType {
                id: secondaryDns

                Layout.fillWidth: true
                headerText: "Secondary DNS"

                textFieldText: SettingsController.secondaryDns
            }

            BasicButtonType {
                Layout.fillWidth: true

                text: qsTr("Save")

                onClicked: function() {
                    if (primaryDns.textFieldText !== SettingsController.primaryDns) {
                        SettingsController.primaryDns = primaryDns.textFieldText
                    }
                    if (secondaryDns.textFieldText !== SettingsController.secondaryDns) {
                        SettingsController.secondaryDns = secondaryDns.textFieldText
                    }
                }
            }
        }
    }
}

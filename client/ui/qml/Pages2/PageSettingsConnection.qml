import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0

import "./"
import "../Controls2"
import "../Config"

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
        anchors.bottom: root.bottom
        contentHeight: content.height

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 16

            HeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Connection")
            }

            SwitcherType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Use AmnesiaDNS if installed on the server")
                descriptionText: qsTr("Internal IP address 172.29.172.254")

                checked: SettingsController.isAmneziaDnsEnabled()
                onCheckedChanged: {
                    if (checked !== SettingsController.isAmneziaDnsEnabled()) {
                        SettingsController.setAmneziaDns(checked)
                    }
                }
            }

            DividerType {}

            LabelWithButtonType {
                Layout.fillWidth: true

                text: qsTr("DNS servers")
                descriptionText: qsTr("If AmneziaDNS is not used or installed")
                buttonImage: "qrc:/images/controls/chevron-right.svg"

                clickedFunction: function() {
                    goToPage(PageEnum.PageSettingsDns)
                }
            }

            DividerType {}

            LabelWithButtonType {
                Layout.fillWidth: true

                text: qsTr("Split site tunneling")
                descriptionText: qsTr("Allows you to connect to some sites through a secure connection, and to others bypassing it")
                buttonImage: "qrc:/images/controls/chevron-right.svg"

                clickedFunction: function() {
                }
            }

            DividerType {}

            LabelWithButtonType {
                Layout.fillWidth: true

                text: qsTr("Separate application tunneling")
                descriptionText: qsTr("Allows you to use the VPN only for certain applications")
                buttonImage: "qrc:/images/controls/chevron-right.svg"

                clickedFunction: function() {
                }
            }

            DividerType {}
        }
    }
}

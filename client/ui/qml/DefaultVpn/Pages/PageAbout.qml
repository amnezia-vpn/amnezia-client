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
        anchors.leftMargin: 8
        anchors.rightMargin: 8

        spacing: 0

        RowLayout {

            WhiteButtonNoBorder {
                id: backButton
                imageSource: "qrc:/images/controls/arrow-left.svg"

                onClicked: PageController.closePage()
            }

            Item {
                Layout.fillWidth: true
            }
        }

        Image {
            id: image
            source: "qrc:/images/about.png"

            Layout.alignment: Qt.AlignCenter
            Layout.topMargin: 8
            Layout.preferredWidth: 230
            Layout.preferredHeight: 230
        }

        XSmallTextType {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            Layout.topMargin: 24
            text: qsTr("Support group in Telegram")
        }

        MediumTextType {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            Layout.topMargin: 8
            text: "@DefaultVPNSupport"
            color: Style.color.accent1
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: Qt.openUrlExternally("https://t.me/DefaultVPNSupport")
            }
        }

        XSmallTextType {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            Layout.topMargin: 24
            text: qsTr("E-mail for technical support")
        }

        MediumTextType {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            Layout.topMargin: 8
            text: "support@dfvpn.com"
            color: Style.color.accent1
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: Qt.openUrlExternally("mailto:support@dfvpn.com")
            }
        }

        XSmallTextType {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            Layout.topMargin: 24
            text: qsTr("Site")
        }

        MediumTextType {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            Layout.topMargin: 8
            text: "dfvpn.com"
            color: Style.color.accent1
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: Qt.openUrlExternally("https://dfvpn.com/")
            }
        }

        Item {
            Layout.fillHeight: true
        }

        MediumTextType {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("Privacy policy")
            color: Style.color.accent1
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: Qt.openUrlExternally("https://dfvpn.com/privacy")
            }
        }

        XSmallTextType {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            Layout.topMargin: 16
            text: qsTr("v. %1").arg(SettingsController.getAppVersion())
            color: Style.color.gray7
        }
    }
} 

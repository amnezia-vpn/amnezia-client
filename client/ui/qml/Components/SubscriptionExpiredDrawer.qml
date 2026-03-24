pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

import PageEnum 1.0
import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"

DrawerType2 {
    id: root

    expandedStateContent: ColumnLayout {
        id: content

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        spacing: 0

        onImplicitHeightChanged: {
            root.expandedHeight = content.implicitHeight + 32 + SettingsController.safeAreaBottomMargin
        }

        Item {
            Layout.fillWidth: true
            Layout.topMargin: 24
            Layout.rightMargin: 16
            Layout.leftMargin: 16
            implicitHeight: titleText.implicitHeight

            Header2TextType {
                id: titleText
                anchors.left: parent.left
                anchors.right: parent.right

                text: qsTr("Amnezia Premium subscription has expired")
                horizontalAlignment: Text.AlignLeft
            }
        }

        ParagraphTextType {
            Layout.fillWidth: true
            Layout.topMargin: 8
            Layout.rightMargin: 16
            Layout.leftMargin: 16

            text: qsTr("Renew your subscription to continue using VPN")
            horizontalAlignment: Text.AlignLeft
        }

        BasicButtonType {
            Layout.fillWidth: true
            Layout.topMargin: 16
            Layout.rightMargin: 16
            Layout.leftMargin: 16

            text: qsTr("Renew")

            defaultColor: AmneziaStyle.color.paleGray
            hoveredColor: AmneziaStyle.color.lightGray
            pressedColor: AmneziaStyle.color.mutedGray
            textColor: AmneziaStyle.color.midnightBlack

            clickedFunc: function() {
                ApiSettingsController.getRenewalLink()
            }
        }

        BasicButtonType {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 8
            Layout.bottomMargin: 8

            implicitHeight: 25

            defaultColor: AmneziaStyle.color.transparent
            hoveredColor: AmneziaStyle.color.translucentWhite
            pressedColor: AmneziaStyle.color.sheerWhite
            textColor: AmneziaStyle.color.goldenApricot

            text: qsTr("Support")

            clickedFunc: function() {
                root.closeTriggered()
                PageController.goToPage(PageEnum.PageSettingsApiSupport)
            }
        }
    }
}

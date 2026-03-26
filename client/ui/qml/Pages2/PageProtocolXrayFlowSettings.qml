import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    BackButtonType {
        id: backButton
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20 + PageController.safeAreaTopMargin
    }

    ListViewType {
        id: listView
        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        model: XrayConfigModel

        delegate: ColumnLayout {
            width: listView.width
            spacing: 0

            Header2TextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 0
                Layout.bottomMargin: 24
                text: qsTr("Flow")
            }

            VerticalRadioButton {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: qsTr("Empty")
                checked: flow === ""
                onClicked: flow = ""
            }

            DividerType {
            }

            VerticalRadioButton {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: "xtls-rprx-vision"
                checked: flow === "xtls-rprx-vision"
                onClicked: flow = "xtls-rprx-vision"
            }

            DividerType {
            }

            VerticalRadioButton {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: "xtls-rprx-vision-udp443"
                checked: flow === "xtls-rprx-vision-udp443"
                onClicked: flow = "xtls-rprx-vision-udp443"
            }

            DividerType {
            }

            Item {
                Layout.preferredHeight: 16
            }
        }
    }

    //     BasicButtonType {
    //         id: saveButton
    //         anchors.bottom: parent.bottom
    //         anchors.left: parent.left
    //         anchors.right: parent.right
    //         anchors.bottomMargin: 16 + PageController.safeAreaBottomMargin
    //         anchors.leftMargin: 16
    //         anchors.rightMargin: 16
    //         text: qsTr("Save")
    //         onClicked: {
    //             forceActiveFocus()
    //             PageController.closePage()
    //         }
    //         Keys.onEnterPressed: clicked()
    //         Keys.onReturnPressed: clicked()
    //     }
}

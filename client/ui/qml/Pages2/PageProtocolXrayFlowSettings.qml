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

    // Temporary local state — will be replaced by model role
    property int selectedFlow: 1  // 0=Empty, 1=xtls-rprx-vision, 2=xtls-rprx-vision-udp443

    BackButtonType {
        id: backButton
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20 + PageController.safeAreaTopMargin
    }

    FlickableType {
        id: flickable
        anchors.top: backButton.bottom
        anchors.bottom: saveButton.top
        anchors.left: parent.left
        anchors.right: parent.right
        contentHeight: mainColumn.implicitHeight

        ColumnLayout {
            id: mainColumn
            width: flickable.width
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
                checked: root.selectedFlow === 0
                onClicked: root.selectedFlow = 0
            }

            DividerType {
            }

            VerticalRadioButton {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: qsTr("xtls-rprx-vision")
                checked: root.selectedFlow === 1
                onClicked: root.selectedFlow = 1
            }

            DividerType {
            }

            VerticalRadioButton {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: qsTr("xtls-rprx-vision-udp443")
                checked: root.selectedFlow === 2
                onClicked: root.selectedFlow = 2
            }

            DividerType {
            }

            Item {
                Layout.preferredHeight: 16
            }
        }
    }

    BasicButtonType {
        id: saveButton
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottomMargin: 16 + PageController.safeAreaBottomMargin
        anchors.leftMargin: 16
        anchors.rightMargin: 16

        text: qsTr("Save")
        onClicked: {
            forceActiveFocus()
            // XrayConfigModel.setFlow(...)
        }
        Keys.onEnterPressed: clicked()
        Keys.onReturnPressed: clicked()
    }
}

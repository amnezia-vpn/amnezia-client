import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

PageType {
    id: root

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20

        onFocusChanged: {
            if (this.activeFocus) {
                listView.positionViewAtBeginning()
            }
        }
    }

    ListViewType {
        id: listView

        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.left: parent.left

        header: ColumnLayout {
            width: listView.width

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                headerText: qsTr("Connection key")
                descriptionText: qsTr("A line that starts with vpn://...")
            }
        }

        spacing: 16
        model: 1 // fake model to force the ListView to be created without a model

        delegate: ColumnLayout {
            width: listView.width

            TextFieldWithHeaderType {
                id: textKey

                Layout.fillWidth: true
                Layout.topMargin: 32
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                headerText: qsTr("Key")
                textField.placeholderText: "vpn://"
                buttonText: qsTr("Insert")

                clickedFunc: function() {
                    textField.text = ""
                    textField.paste()
                }
            }
        }

        footer: ColumnLayout {
            width: listView.width

            BasicButtonType {
                id: continueButton

                Layout.fillWidth: true
                Layout.rightMargin: 16
                Layout.leftMargin: 16
                Layout.topMargin: 16
                Layout.bottomMargin: 32

                text: qsTr("Continue")

                clickedFunc: function() {
                    if (ImportController.extractConfigFromData(textKey.textField.text)) {
                        PageController.goToPage(PageEnum.PageSetupWizardViewConfig)
                    }
                }
            }
        }
    }
}

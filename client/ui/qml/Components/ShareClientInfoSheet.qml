import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"

SheetType {
    id: root

    property string clientName
    property string creationDate
    property string latestHandshake
    property string dataReceived
    property string dataSent
    property string allowedIps

    signal renameRequested(string newName)
    signal revokeRequested()

    expandedHeight: content.implicitHeight + 16 + PageController.safeAreaBottomMargin

    ColumnLayout {
        id: content

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 16
        anchors.leftMargin: 16
        anchors.rightMargin: 16

        spacing: 0

        H2TextType {
            Layout.fillWidth: true
            Layout.bottomMargin: 8

            text: root.clientName
            maximumLineCount: 2
            wrapMode: Text.Wrap
            elide: Qt.ElideRight
        }

        BodyTextType {
            Layout.fillWidth: true

            visible: root.creationDate !== ""

            maximumLineCount: 2
            wrapMode: Text.Wrap
            elide: Qt.ElideRight

            text: qsTranslate("PageShare", "Creation date: %1").arg(root.creationDate)
        }

        BodyTextType {
            Layout.fillWidth: true

            visible: root.latestHandshake !== ""

            maximumLineCount: 2
            wrapMode: Text.Wrap
            elide: Qt.ElideRight

            text: qsTranslate("PageShare", "Latest handshake: %1").arg(root.latestHandshake)
        }

        BodyTextType {
            Layout.fillWidth: true

            visible: root.dataReceived !== ""

            maximumLineCount: 2
            wrapMode: Text.Wrap
            elide: Qt.ElideRight

            text: qsTranslate("PageShare", "Data received: %1").arg(root.dataReceived)
        }

        BodyTextType {
            Layout.fillWidth: true

            visible: root.dataSent !== ""

            maximumLineCount: 2
            wrapMode: Text.Wrap
            elide: Qt.ElideRight

            text: qsTranslate("PageShare", "Data sent: %1").arg(root.dataSent)
        }

        BodyTextType {
            Layout.fillWidth: true

            visible: root.allowedIps !== ""

            wrapMode: Text.Wrap

            text: qsTranslate("PageShare", "Allowed IPs: %1").arg(root.allowedIps)
        }

        ButtonType {
            id: renameButton

            Layout.fillWidth: true
            Layout.topMargin: 24

            variant: ButtonType.Secondary
            text: qsTranslate("PageShare", "Rename")

            onClicked: {
                clientNameEditor.textField.text = root.clientName
                clientNameEditDrawer.open()
            }
        }

        ButtonType {
            id: revokeButton

            Layout.fillWidth: true
            Layout.topMargin: 12

            variant: ButtonType.Secondary
            text: qsTranslate("PageShare", "Revoke")

            onClicked: root.revokeRequested()
        }
    }

    SheetType {
        id: clientNameEditDrawer

        expandedHeight: editContent.implicitHeight + 32 + PageController.safeAreaBottomMargin

        ColumnLayout {
            id: editContent

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 16
            anchors.leftMargin: 16
            anchors.rightMargin: 16

            spacing: 16

            TextFieldType {
                id: clientNameEditor

                Layout.fillWidth: true

                headerText: qsTranslate("PageShare", "Client name")
                textField.text: root.clientName
                textField.maximumLength: 20
                checkEmptyText: true
            }

            ButtonType {
                id: saveButton

                Layout.fillWidth: true

                text: qsTranslate("PageShare", "Save")

                onClicked: {
                    if (clientNameEditor.textField.text === "") {
                        return
                    }

                    if (clientNameEditor.textField.text !== root.clientName) {
                        root.renameRequested(clientNameEditor.textField.text)
                        clientNameEditDrawer.close()
                    }
                }
            }
        }
    }
}

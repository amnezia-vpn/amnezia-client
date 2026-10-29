import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import QtQuick.Dialogs

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

PageType {
    id: root

    property bool showContent: false

    Connections {
        target: PageController

        function onClosePage() {
            ImportController.clearConfigFileName()
        }
    }

    Connections {
        target: ImportController

        function onImportErrorOccurred(error, goToPageHome) {
            PageController.showBusyIndicator(false)
            if (goToPageHome) {
                PageController.goToStartPage()
            } else {
                PageController.closePage()
            }
        }

        function onImportFinished() {
            PageController.showBusyIndicator(false)
            PageController.goToPageHome()
        }
    }

    FlickableType {
        id: flickable

        anchors.top: parent.top
        anchors.bottom: bottomBar.top

        contentHeight: content.implicitHeight + 16

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0

            NavigationBarType {
                Layout.fillWidth: true

                title: qsTr("New connection")
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                spacing: 12

                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 8
                    Layout.bottomMargin: 8

                    spacing: 8

                    visible: fileName.text !== ""

                    IconImage {
                        Layout.preferredWidth: 24
                        Layout.preferredHeight: 24

                        source: "qrc:/images/controls/file-check-2.svg"
                        sourceSize: Qt.size(24, 24)
                        color: AmneziaStyle.color.iconDefault
                    }

                    H2TextType {
                        id: fileName

                        Layout.fillWidth: true

                        text: ImportController.getConfigFileName()
                        color: AmneziaStyle.color.textSecondary
                        wrapMode: Text.Wrap
                    }
                }

                ButtonType {
                    id: showContentButton

                    Layout.alignment: Qt.AlignLeft

                    variant: ButtonType.Text
                    size: ButtonType.Medium

                    text: root.showContent ? qsTr("Collapse content") : qsTr("Show content")

                    onClicked: {
                        root.showContent = !root.showContent
                    }
                }

                CheckBoxType {
                    id: cloakingCheckBox

                    Layout.fillWidth: true

                    visible: ImportController.isNativeWireGuardConfig

                    text: qsTr("Enable WireGuard obfuscation. It may be useful if WireGuard is blocked on your provider.")
                }

                NotificationType {
                    Layout.fillWidth: true

                    variant: NotificationType.Error
                    iconSource: "qrc:/images/controls/mail-warning.svg"
                    text: ImportController.maliciousWarningText
                    textFormat: Qt.RichText
                    visible: text !== ""
                }

                NotificationType {
                    Layout.fillWidth: true

                    text: qsTr("Use connection codes only from sources you trust. Codes from public sources may have been created to intercept your data.")
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.topMargin: 12

                    implicitHeight: configContent.implicitHeight + 32

                    radius: 16
                    color: AmneziaStyle.color.bgSurface

                    visible: root.showContent

                    BodyTextType {
                        id: configContent

                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 16

                        wrapMode: Text.Wrap

                        text: ImportController.getConfig()
                    }
                }
            }
        }
    }

    BottomBarType {
        id: bottomBar

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        ButtonType {
            id: connectButton

            Layout.fillWidth: true

            text: qsTr("Connect")

            onClicked: {
                if (cloakingCheckBox.checked) {
                    ImportController.processNativeWireGuardConfig()
                }
                PageController.showBusyIndicator(true)
                ImportController.importConfig()
            }
        }
    }
}

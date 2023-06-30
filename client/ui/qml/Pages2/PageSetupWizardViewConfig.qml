import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import PageEnum 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

PageType {
    id: root

    property bool showContent: false

    Connections {
        target: ImportController

        function onImportErrorOccurred(errorMessage) {
            closePage()
            PageController.showErrorMessage(errorMessage)
        }

        function onImportFinished() {
            goToStartPage()
            if (stackView.currentItem.objectName === PageController.getPagePath(PageEnum.PageHome)) {
                PageController.restorePageHomeState()
            } else if (stackView.currentItem.objectName === PageController.getPagePath(PageEnum.PageSettings)) {
                goToPage(PageEnum.PageSettingsServersList, false)
            } else {
                var pagePath = PageController.getPagePath(PageEnum.PageStart)
                stackView.replace(pagePath, { "objectName" : pagePath })
            }
        }
    }

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20
    }

    FlickableType {
        id: fl
        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        contentHeight: content.implicitHeight + connectButton.implicitHeight

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.rightMargin: 16
            anchors.leftMargin: 16

            HeaderType {
                headerText: qsTr("New connection")
            }

            RowLayout {
                Layout.topMargin: 32
                spacing: 8

                visible: fileName.text !== ""

                Image {
                    source: "qrc:/images/controls/file-cog-2.svg"
                }

                Header2TextType {
                    id: fileName

                    Layout.fillWidth: true

                    text: ImportController.getConfigFileName()
                }
            }

            CaptionTextType {
                Layout.fillWidth: true
                Layout.topMargin: 16

                text: qsTr("Do not use connection code from public sources. It could be created to intercept your data.")
                color: "#878B91"
            }

            BasicButtonType {
                defaultColor: "transparent"
                hoveredColor: Qt.rgba(1, 1, 1, 0.08)
                pressedColor: Qt.rgba(1, 1, 1, 0.12)
                disabledColor: "#878B91"
                textColor: "#D7D8DB"

                text: showContent ? qsTr("Collapse content") : qsTr("Show content")

                onClicked: {
                    showContent = !showContent
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.bottomMargin: 16

                implicitHeight: configContent.implicitHeight

                radius: 10
                color: "#1C1D21"

                visible: showContent

                ParagraphTextType {
                    id: configContent

                    anchors.fill: parent
                    anchors.margins: 16

                    text: ImportController.getConfig()
                }
            }
        }
    }

    ColumnLayout {
        id: connectButton

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.rightMargin: 16
        anchors.leftMargin: 16

        BasicButtonType {
            Layout.fillWidth: true
            Layout.bottomMargin: 32

            text: qsTr("Connect")
            onClicked: {
                ImportController.importConfig()
            }
        }
    }
}

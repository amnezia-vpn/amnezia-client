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

        function onImportErrorOccurred(errorMessage, goToPageHome) {
            if (goToPageHome) {
                PageController.goToStartPage()
            } else {
                PageController.closePage()
            }
            PageController.showErrorMessage(errorMessage)
        }

        function onImportFinished() {
            if (!ConnectionController.isConnected) {
                ServersModel.setDefaultServerIndex(ServersModel.getServersCount() - 1);
                ServersModel.processedIndex = ServersModel.defaultIndex
            }

            PageController.goToStartPage()
            if (stackView.currentItem.objectName === PageController.getPagePath(PageEnum.PageSetupWizardStart)) {
                PageController.replaceStartPage()
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
                    wrapMode: Text.Wrap
                }
            }

            CaptionTextType {
                Layout.fillWidth: true
                Layout.topMargin: 16

                text: qsTr("Do not use connection code from public sources. It could be created to intercept your data.")
                color: "#878B91"
            }

            BasicButtonType {
                Layout.topMargin: 16
                Layout.leftMargin: -8
                implicitHeight: 32

                defaultColor: "transparent"
                hoveredColor: Qt.rgba(1, 1, 1, 0.08)
                pressedColor: Qt.rgba(1, 1, 1, 0.12)
                disabledColor: "#878B91"
                textColor: "#FBB26A"

                text: showContent ? qsTr("Collapse content") : qsTr("Show content")

                clickedFunc: function() {
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
            clickedFunc: function() {
                ImportController.importConfig()
            }
        }
    }
}

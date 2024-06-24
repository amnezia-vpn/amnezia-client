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

    defaultActiveFocusItem: focusItem

    Item {
        id: focusItem
        KeyNavigation.tab: backButton
    }

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20

        KeyNavigation.tab: showContentButton
    }

    Connections {
        target: ImportController

        function onImportErrorOccurred(errorMessage, goToPageHome) {
            if (goToPageHome) {
                PageController.goToStartPage()
            } else {
                PageController.closePage()
            }
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
                    source: "qrc:/images/controls/file-check-2.svg"
                }

                Header2TextType {
                    id: fileName

                    Layout.fillWidth: true

                    text: ImportController.getConfigFileName()
                    wrapMode: Text.Wrap
                }
            }

            BasicButtonType {
                id: showContentButton
                Layout.topMargin: 16
                Layout.leftMargin: -8
                implicitHeight: 32

                defaultColor: "transparent"
                hoveredColor: Qt.rgba(1, 1, 1, 0.08)
                pressedColor: Qt.rgba(1, 1, 1, 0.12)
                disabledColor: "#878B91"
                textColor: "#FBB26A"

                text: showContent ? qsTr("Collapse content") : qsTr("View debugging information")
                KeyNavigation.tab: connectButton

                clickedFunc: function() {
                    showContent = !showContent
                }
            }

            CheckBoxType {
                id: cloakingCheckBox

                visible: ImportController.isNativeWireGuardConfig()

                Layout.fillWidth: true
                text: qsTr("Enable WireGuard obfuscation. It may be useful if WireGuard is blocked on your provider.")
            }

            WarningType {
                Layout.topMargin: 16
                Layout.fillWidth: true

                textString: ImportController.getMaliciousWarningText()
                textFormat: Qt.RichText
                visible: textString !== ""

                iconPath: "qrc:/images/controls/alert-circle.svg"

                textColor: "#EB5757"
                imageColor: "#EB5757"
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.bottomMargin: 48

                implicitHeight: configContent.implicitHeight

                radius: 10
                color: "#1C1D21"

                visible: showContent

                ParagraphTextType {
                    id: configContent

                    anchors.fill: parent
                    anchors.margins: 16

                    wrapMode: Text.Wrap

                    text: ImportController.getConfig()
                }
            }
        }
    }

    Rectangle {
        anchors.fill: columnContent
        anchors.bottomMargin: -24
        color: "#0E0E11"
        opacity: 0.8
    }

    ColumnLayout {
        id: columnContent
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.rightMargin: 16
        anchors.leftMargin: 16

        Keys.onTabPressed: lastItemTabClicked(focusItem)

        BasicButtonType {
            id: connectButton
            Layout.fillWidth: true
            Layout.bottomMargin: 32

            text: qsTr("Connect")

            defaultColor: "transparent"
            hoveredColor: "#FFDD51"
            pressedColor: "#FFDD51"
            disabledColor: "#878B91"
            textColor: "#000000" // Set default text color to black

            // Button styling
            background: Rectangle {
                color: parent.hovered ? "#FFDD51" : "transparent"
                border.color: parent.hovered ? "#191919" : "#FFDD51" // Set border color to corner color when hovered
                radius: 10
            }

            // Button text color
            contentItem: Text {
                text: parent.text
                color: parent.hovered ? "#000000" : "#FFDD51" // Change text color when hovered
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 16
            }
            clickedFunc: function() {
                if (cloakingCheckBox.checked) {
                    ImportController.processNativeWireGuardConfig()
                }
                ImportController.importConfig()
            }
        }
    }
}

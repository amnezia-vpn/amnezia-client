import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0

import "./"
import "../Controls2"
import "../Config"
import "../Controls2/TextTypes"
import "../Components"

PageType {
    id: root

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
        contentHeight: content.height

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            HeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Application")
            }

            LabelWithButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 16

                text: qsTr("Language")
                descriptionText: LanguageModel.currentLanguageName
                rightImageSource: "qrc:/images/controls/chevron-right.svg"

                clickedFunction: function() {
                    selectLanguageDrawer.open()
                }
            }

            SelectLanguageDrawer {
                id: selectLanguageDrawer
            }


            DividerType {}

            LabelWithButtonType {
                Layout.fillWidth: true

                text: qsTr("Logging")
                descriptionText: SettingsController.isLoggingEnabled ? qsTr("Enabled") : qsTr("Disabled")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"

                clickedFunction: function() {
                    goToPage(PageEnum.PageSettingsLogging)
                }
            }

            DividerType {}

            LabelWithButtonType {
                Layout.fillWidth: true

                text: qsTr("Reset settings and remove all data from the application")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"

                clickedFunction: function() {
                    questionDrawer.headerText = qsTr("Reset settings and remove all data from the application?")
                    questionDrawer.descriptionText = qsTr("All settings will be reset to default. All installed AmneziaVPN services will still remain on the server.")
                    questionDrawer.yesButtonText = qsTr("Continue")
                    questionDrawer.noButtonText = qsTr("Cancel")

                    questionDrawer.yesButtonFunction = function() {
                        questionDrawer.visible = false
                        SettingsController.clearSettings()
                        PageController.replaceStartPage()
                    }
                    questionDrawer.noButtonFunction = function() {
                        questionDrawer.visible = false
                    }
                    questionDrawer.visible = true
                }
            }

            DividerType {}

            QuestionDrawer {
                id: questionDrawer
            }
        }
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Config 1.0

import "../Components"
import "../Controls"
import "../Controls/TextTypes"

Page {
    id: root

    ColumnLayout {
        anchors.fill: parent

        RowLayout {
            Layout.leftMargin: 8
            Layout.rightMargin: 8
            Layout.topMargin: 8

            WhiteButtonNoBorder {
                id: backButton
                imageSource: "qrc:/images/controls/arrow-left.svg"
                onClicked: PageController.closePage()
            }
        }

        Header1TextType {
            Layout.topMargin: 8
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.fillWidth: true
            text: qsTr("Select Language")
            horizontalAlignment: Qt.AlignLeft
            verticalAlignment: Qt.AlignVCenter
        }

        ButtonGroup {
            id: languageButtonGroup
        }

        ListView {
            id: languageListView

            Layout.topMargin: 16
            Layout.fillHeight: true
            Layout.fillWidth: true

            model: LanguageModel
            currentIndex: LanguageModel.currentLanguageIndex
            spacing: 8

            ScrollBar.vertical: ScrollBar {}

            delegate: Item {
                id: languageItem
                required property string languageName
                required property int languageIndex
                required property int index

                width: languageListView.width
                height: 60

                Rectangle {
                    anchors.fill: parent
                    color: radioButton.checked ? Style.color.gray1 : Style.color.transparent
                    radius: 8

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 8

                        RadioButton {
                            id: radioButton
                            ButtonGroup.group: languageButtonGroup
                            checked: languageIndex === LanguageModel.currentLanguageIndex

                            text: languageName
                            font.pixelSize: 18
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignVCenter

                            onClicked: {
                                if (languageIndex !== LanguageModel.currentLanguageIndex) {
                                    LanguageModel.changeLanguage(languageIndex);
                                    PageController.closePage();
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

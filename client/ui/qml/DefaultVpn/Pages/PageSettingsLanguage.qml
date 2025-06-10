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

            ScrollBar.vertical: ScrollBar {}

            delegate: Item {
                required property string languageName
                required property int languageIndex
                required property int index

                implicitWidth: languageListView.width
                implicitHeight: languageItem.implicitHeight

                visible: languageName === "English" || languageName === "Русский"

                RadioButton {
                    id: languageItem

                    anchors.fill: parent
                    anchors.rightMargin: 16
                    anchors.leftMargin: 16

                    ButtonGroup.group: languageButtonGroup

                    checked: languageIndex === LanguageModel.currentLanguageIndex

                    indicator: Item { }

                    contentItem: Item {
                        id: contentContainer

                        anchors.left: parent.left
                        anchors.right: parent.right

                        implicitHeight: content.implicitHeight

                        Rectangle {
                            anchors.fill: parent
                            radius: 8
                            color: languageItem.checked ? Style.color.gray1 : Style.color.transparent
                        }

                        RowLayout {
                            id: content
                            anchors.fill: parent

                            Header3TextType {
                                Layout.fillWidth: true
                                Layout.leftMargin: 8
                                Layout.topMargin: 19
                                Layout.bottomMargin: 19

                                text: languageName

                                color: languageItem.hovered ? Style.color.gray9 : Style.color.black
                            }

                            Image {
                                Layout.rightMargin: 8
                                width: 24
                                height: 24
                                source: "qrc:/images/controls/check.svg"
                                visible: languageItem.checked
                            }
                        }
                    }

                    onClicked: {
                        if (languageIndex !== LanguageModel.currentLanguageIndex) {
                            LanguageModel.changeLanguage(languageIndex);
                            PageController.closePage();
                        }
                    }

                    MouseArea {
                        anchors.fill: languageItem
                        cursorShape: Qt.PointingHandCursor
                        enabled: false
                    }
                }
            }
        }
    }
}

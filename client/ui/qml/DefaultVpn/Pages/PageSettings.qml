import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

import PageEnum 1.0
import Config 1.0

import "../Components"
import "../Controls"
import "../Controls/TextTypes"

Page {
    id: root

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        anchors.leftMargin: 8
        anchors.rightMargin: 8

        RowLayout {
            Layout.topMargin: 8

            WhiteButtonNoBorder {
                id: backButton
                imageSource: "qrc:/images/controls/arrow-left.svg"
                onClicked: PageController.closePage()
            }
        }

        Header1TextType {
            Layout.topMargin: 8
            Layout.leftMargin: 8
            Layout.rightMargin: 8
            Layout.fillWidth: true

            text: qsTr("Settings")

            horizontalAlignment: Qt.AlignLeft
            verticalAlignment: Qt.AlignVCenter
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.topMargin: 24
            Layout.leftMargin: 8
            Layout.rightMargin: 8
            spacing: 2

            RadioButton {
                id: languageRadioButton
                Layout.fillWidth: true
                background: Rectangle {
                    anchors.fill: parent
                    radius: 8
                    color: languageRadioButton.hovered ? Style.color.gray1 : Style.color.transparent
                }

                indicator: Item { }

                contentItem: RowLayout {
                    id: content

                    anchors.left: parent.left
                    anchors.right: parent.right
                    implicitHeight: content.implicitHeight

                    Header3TextType {
                        Layout.fillWidth: true
                        Layout.leftMargin: 8
                        Layout.topMargin: 19
                        Layout.bottomMargin: 19

                        text: qsTr("Language")
                        color: languageRadioButton.hovered ? Style.color.gray9 : Style.color.black
                    }

                    Image {
                        Layout.rightMargin: 8
                        source: "qrc:/images/controls/chevron-right.svg"

                        layer {
                            enabled: true
                            effect: ColorOverlay {
                                color: Style.color.black
                            }
                        }
                    }
                }

                onClicked: PageController.goToPage(PageEnum.PageSettingsLanguage)

                MouseArea {
                    anchors.fill: languageRadioButton
                    cursorShape: Qt.PointingHandCursor
                    enabled: false
                }
            }

            RadioButton {
                id: loggingRadioButton
                Layout.fillWidth: true
                background: Rectangle {
                    anchors.fill: parent
                    radius: 8
                    color: loggingRadioButton.hovered ? Style.color.gray1 : Style.color.transparent
                }

                indicator: Item { }

                contentItem: RowLayout {
                    id: content2

                    anchors.left: parent.left
                    anchors.right: parent.right
                    implicitHeight: content2.implicitHeight

                    Header3TextType {
                        Layout.fillWidth: true
                        Layout.leftMargin: 8
                        Layout.topMargin: 19
                        Layout.bottomMargin: 19

                        text: qsTr("Logging")
                        color: loggingRadioButton.hovered ? Style.color.gray9 : Style.color.black
                    }

                    Image {
                        Layout.rightMargin: 8
                        source: "qrc:/images/controls/chevron-right.svg"

                        layer {
                            enabled: true
                            effect: ColorOverlay {
                                color: Style.color.black
                            }
                        }
                    }
                }

                onClicked: PageController.goToPage(PageEnum.PageSettingsLogging)

                MouseArea {
                    anchors.fill: loggingRadioButton
                    cursorShape: Qt.PointingHandCursor
                    enabled: false
                }
            }

            RadioButton {
                id: aboutRadioButton
                Layout.fillWidth: true
                background: Rectangle {
                    anchors.fill: parent
                    radius: 8
                    color: aboutRadioButton.hovered ? Style.color.gray1 : Style.color.transparent
                }

                indicator: Item { }

                contentItem: RowLayout {
                    id: content3

                    anchors.left: parent.left
                    anchors.right: parent.right
                    implicitHeight: content.implicitHeight

                    Header3TextType {
                        Layout.fillWidth: true
                        Layout.leftMargin: 8
                        Layout.topMargin: 19
                        Layout.bottomMargin: 19

                        text: qsTr("About")
                        color: aboutRadioButton.hovered ? Style.color.gray9 : Style.color.black
                    }

                    Image {
                        Layout.rightMargin: 8
                        source: "qrc:/images/controls/chevron-right.svg"

                        layer {
                            enabled: true
                            effect: ColorOverlay {
                                color: Style.color.black
                            }
                        }
                    }
                }

                onClicked: PageController.goToPage(PageEnum.PageAbout)

                MouseArea {
                    anchors.fill: aboutRadioButton
                    cursorShape: Qt.PointingHandCursor
                    enabled: false
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
} 

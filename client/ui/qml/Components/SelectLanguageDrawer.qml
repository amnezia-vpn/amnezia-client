import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../Controls2"
import "../Controls2/TextTypes"

DrawerType2 {
    id: root

    expandedContent: Item {
        id: container

        implicitHeight: root.height * 0.9

        Component.onCompleted: {
            root.expandedHeight = container.implicitHeight
        }

        ColumnLayout {
            id: backButton

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 16

            BackButtonType {
                backButtonImage: "qrc:/images/controls/arrow-left.svg"
                backButtonFunction: function() {
                    root.close()
                }
            }
        }

        FlickableType {
            anchors.top: backButton.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            contentHeight: content.implicitHeight

            ColumnLayout {
                id: content

                anchors.fill: parent

                Header2Type {
                    id: header
                    Layout.fillWidth: true
                    Layout.topMargin: 16
                    Layout.rightMargin: 16
                    Layout.leftMargin: 16

                    headerText: qsTr("Choose language")
                }

                ListView {
                    id: listView

                    Layout.fillWidth: true
                    height: listView.contentItem.height

                    clip: true
                    interactive: false

                    model: LanguageModel
                    currentIndex: LanguageModel.currentLanguageIndex

                    ButtonGroup {
                        id: buttonGroup
                    }

                    delegate: Item {
                        implicitWidth: root.width
                        implicitHeight: delegateContent.implicitHeight

                        ColumnLayout {
                            id: delegateContent

                            anchors.fill: parent

                            RadioButton {
                                id: radioButton

                                implicitWidth: parent.width
                                implicitHeight: radioButtonContent.implicitHeight

                                hoverEnabled: true

                                indicator: Rectangle {
                                    anchors.fill: parent
                                    color: radioButton.hovered ? "#2C2D30" : "#1C1D21"

                                    Behavior on color {
                                        PropertyAnimation { duration: 200 }
                                    }
                                }

                                RowLayout {
                                    id: radioButtonContent
                                    anchors.fill: parent

                                    anchors.rightMargin: 16
                                    anchors.leftMargin: 16

                                    spacing: 0

                                    z: 1

                                    ParagraphTextType {
                                        Layout.fillWidth: true
                                        Layout.topMargin: 20
                                        Layout.bottomMargin: 20

                                        text: languageName
                                    }

                                    Image {
                                        source: "qrc:/images/controls/check.svg"
                                        visible: radioButton.checked

                                        width: 24
                                        height: 24

                                        Layout.rightMargin: 8
                                    }
                                }

                                ButtonGroup.group: buttonGroup
                                checked: listView.currentIndex === index

                                onClicked: {
                                    listView.currentIndex = index
                                    LanguageModel.changeLanguage(languageIndex)
                                    root.close()
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

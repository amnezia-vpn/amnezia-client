import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"

DrawerType2 {
    id: root

    expandedStateContent: Item {
        id: container

        implicitHeight: root.height * 0.9

        Component.onCompleted: {
            root.expandedHeight = container.implicitHeight
        }

        ColumnLayout {
            id: backButtonLayout

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 16

            BackButtonType {
                id: backButton

                Layout.fillWidth: true

                backButtonImage: "qrc:/images/controls/arrow-left.svg"
                backButtonFunction: function() { root.closeTriggered() }
            }

            Header2Type {
                id: header

                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                headerText: qsTr("Color theme")
            }
        }

        ListViewType {
            id: listView

            anchors.top: backButtonLayout.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom

            property int selectedIndex: AppearanceController.themeMode

            model: ListModel {
                ListElement { name: qsTr("System"); mode: 0 }
                ListElement { name: qsTr("Light"); mode: 1 }
                ListElement { name: qsTr("Dark"); mode: 2 }
            }

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

                        property bool isFocusable: true

                        Keys.onTabPressed: {
                            FocusController.nextKeyTabItem()
                        }

                        Keys.onBacktabPressed: {
                            FocusController.previousKeyTabItem()
                        }

                        Keys.onUpPressed: {
                            FocusController.nextKeyUpItem()
                        }

                        Keys.onDownPressed: {
                            FocusController.nextKeyDownItem()
                        }

                        Keys.onLeftPressed: {
                            FocusController.nextKeyLeftItem()
                        }

                        Keys.onRightPressed: {
                            FocusController.nextKeyRightItem()
                        }

                        indicator: Rectangle {
                            width: parent.width - 1
                            height: parent.height
                            color: radioButton.hovered ? AmneziaStyle.color.slateGray : AmneziaStyle.color.onyxBlack
                            border.color: radioButton.focus ? AmneziaStyle.color.paleGray : AmneziaStyle.color.transparent
                            border.width: radioButton.focus ? 1 : 0

                            Behavior on color {
                                PropertyAnimation { duration: 200 }
                            }
                            Behavior on border.color {
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

                                text: name
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
                        checked: listView.selectedIndex === index

                        onClicked: {
                            AppearanceController.themeMode = mode
                            root.closeTriggered()
                        }
                    }
                }

                Keys.onEnterPressed: radioButton.clicked()
                Keys.onReturnPressed: radioButton.clicked()
            }
        }
    }
}

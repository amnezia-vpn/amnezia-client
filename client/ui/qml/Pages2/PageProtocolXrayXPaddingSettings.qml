import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    // Temporary local state
    property string xPaddingBytes: "0—0"
    property bool   xPaddingObfsMode: true
    property string xPaddingKey: "www.googletagmanager.com"
    property string xPaddingHeader: ""
    property string xPaddingPlacement: "Cookie"
    property string xPaddingMethod: "Repeat-x"

    BackButtonType {
        id: backButton
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20 + PageController.safeAreaTopMargin
    }

    FlickableType {
        id: flickable
        anchors.top: backButton.bottom
        anchors.bottom: saveButton.top
        anchors.left: parent.left
        anchors.right: parent.right
        contentHeight: mainColumn.implicitHeight

        ColumnLayout {
            id: mainColumn
            width: flickable.width
            spacing: 0

            // ── Header ────────────────────────────────────────────────
            Header2TextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 0
                Layout.bottomMargin: 24
                text: qsTr("xPadding")
            }

            // ── xPaddingBytes nav row ─────────────────────────────────
            LabelWithButtonType {
                Layout.fillWidth: true
                text: qsTr("xPaddingBytes")
                descriptionText: root.xPaddingBytes
                rightImageSource: "qrc:/images/controls/chevron-right.svg"
                clickedFunction: function () {
                    PageController.goToPage(PageEnum.PageProtocolXrayXPaddingBytesSettings)
                }
            }

            DividerType {
            }

            // ── xPaddingObfsMode switcher ─────────────────────────────
            SwitcherType {
                Layout.fillWidth: true
                Layout.margins: 16
                text: qsTr("xPaddingObfsMode")
                checked: root.xPaddingObfsMode
                onToggled: root.xPaddingObfsMode = checked
            }

            DividerType {
            }

            // ── xPaddingKey ───────────────────────────────────────────
            TextFieldWithHeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 16
                headerText: qsTr("xPaddingKey")
                textField.text: root.xPaddingKey
                textField.onEditingFinished: root.xPaddingKey = textField.text
            }

            // ── xPaddingHeader ────────────────────────────────────────
            TextFieldWithHeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 8
                headerText: qsTr("xPaddingHeader")
                textField.text: root.xPaddingHeader
                textField.onEditingFinished: root.xPaddingHeader = textField.text
            }

            // ── xPaddingPlacement dropdown ────────────────────────────
            DropDownType {
                id: placementDropDown
                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: root.xPaddingPlacement
                descriptionText: qsTr("xPaddingPlacement")
                headerText: qsTr("xPaddingPlacement")
                drawerParent: root
                listView: ListViewWithRadioButtonType {
                    rootWidth: root.width
                    model: ListModel {
                        ListElement {
                            name: "Cookie"
                        }
                        ListElement {
                            name: "Header"
                        }
                        ListElement {
                            name: "Query"
                        }
                        ListElement {
                            name: "Body"
                        }
                    }
                    clickedFunction: function () {
                        root.xPaddingPlacement = selectedText
                        placementDropDown.text = selectedText
                        placementDropDown.closeTriggered()
                    }
                    Component.onCompleted: {
                        for (var i = 0; i < model.count; i++) {
                            if (model.get(i).name === root.xPaddingPlacement) {
                                selectedIndex = i;
                                break
                            }
                        }
                    }
                }
            }

            // ── xPaddingMethod dropdown ───────────────────────────────
            DropDownType {
                id: methodDropDown
                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: root.xPaddingMethod
                descriptionText: qsTr("xPaddingMethod")
                headerText: qsTr("xPaddingMethod")
                drawerParent: root
                listView: ListViewWithRadioButtonType {
                    rootWidth: root.width
                    model: ListModel {
                        ListElement {
                            name: "Repeat-x"
                        }
                        ListElement {
                            name: "Random"
                        }
                        ListElement {
                            name: "Zero"
                        }
                    }
                    clickedFunction: function () {
                        root.xPaddingMethod = selectedText
                        methodDropDown.text = selectedText
                        methodDropDown.closeTriggered()
                    }
                    Component.onCompleted: {
                        for (var i = 0; i < model.count; i++) {
                            if (model.get(i).name === root.xPaddingMethod) {
                                selectedIndex = i;
                                break
                            }
                        }
                    }
                }
            }

            Item {
                Layout.preferredHeight: 16
            }
        }
    }

    // ── Save button ───────────────────────────────────────────────────
    BasicButtonType {
        id: saveButton
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottomMargin: 16 + PageController.safeAreaBottomMargin
        anchors.leftMargin: 16
        anchors.rightMargin: 16

        text: qsTr("Save")
        onClicked: {
            forceActiveFocus()
            // XrayConfigModel.setXPadding(...)
        }
        Keys.onEnterPressed: clicked()
        Keys.onReturnPressed: clicked()
    }
}

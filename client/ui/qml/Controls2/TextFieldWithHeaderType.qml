import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "TextTypes"

Item {
    id: root

    property string headerText
    property string headerTextDisabledColor: "#494B50"
    property string headerTextColor: "#878b91"

    property alias errorText: errorField.text
    property bool checkEmptyText: false

    property string buttonText
    property string buttonImageSource
    property var clickedFunc

    property alias textField: textField
    property alias textFieldText: textField.text
    property string textFieldTextColor: "#d7d8db"
    property string textFieldTextDisabledColor: "#878B91"

    property string textFieldPlaceholderText
    property bool textFieldEditable: true

    property string borderColor: "#2C2D30"
    property string borderFocusedColor: "#d7d8db"

    property string backgroundColor: "#1c1d21"
    property string backgroundDisabledColor: "transparent"
    property string bgBorderHoveredColor: "#494B50"

    implicitWidth: content.implicitWidth
    implicitHeight: content.implicitHeight

    ColumnLayout {
        id: content
        anchors.fill: parent

        Rectangle {
            id: backgroud
            Layout.fillWidth: true
            Layout.preferredHeight: input.implicitHeight
            color: root.enabled ? root.backgroundColor : root.backgroundDisabledColor
            radius: 16
            border.color: getBackgroundBorderColor(root.borderColor)
            border.width: 1

            Behavior on border.color {
                PropertyAnimation { duration: 200 }
            }

            RowLayout {
                id: input
                anchors.fill: backgroud
                ColumnLayout {
                    Layout.margins: 16
                    LabelTextType {
                        text: root.headerText
                        color: root.enabled ? root.headerTextColor : root.headerTextDisabledColor

                        visible: text !== ""

                        Layout.fillWidth: true
                    }

                    TextField {
                        id: textField
                        activeFocusOnTab: false

                        enabled: root.textFieldEditable
                        color: root.enabled ? root.textFieldTextColor : root.textFieldTextDisabledColor

                        inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhSensitiveData | Qt.ImhNoPredictiveText

                        placeholderText: root.textFieldPlaceholderText
                        placeholderTextColor: "#494B50"

                        selectionColor:  "#633303"
                        selectedTextColor: "#D7D8DB"

                        font.pixelSize: 16
                        font.weight: 400
                        font.family: "PT Root UI VF"

                        height: 24
                        Layout.fillWidth: true

                        topPadding: 0
                        rightPadding: 0
                        leftPadding: 0
                        bottomPadding: 0

                        background: Rectangle {
                            anchors.fill: parent
                            color: root.enabled ? root.backgroundColor : root.backgroundDisabledColor
                        }

                        onTextChanged: {
                            root.errorText = ""
                        }

                        onActiveFocusChanged: {
                            if (checkEmptyText && textFieldText === "") {
                                errorText = qsTr("The field can't be empty")
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            acceptedButtons: Qt.RightButton
                            onClicked: contextMenu.open()
                            enabled: true
                        }

                        ContextMenuType {
                            id: contextMenu
                            textObj: textField
                        }

                        onFocusChanged: {
                            backgroud.border.color = getBackgroundBorderColor(root.borderColor)
                        }
                    }
                }

                BasicButtonType {
                    visible: (root.buttonText !== "") || (root.buttonImageSource !== "")

//                    defaultColor: "transparent"
//                    hoveredColor: Qt.rgba(1, 1, 1, 0.08)
//                    pressedColor: Qt.rgba(1, 1, 1, 0.12)
//                    disabledColor: "#878B91"
//                    textColor: "#D7D8DB"
//                    borderWidth: 0

                    text: root.buttonText
                    imageSource: root.buttonImageSource

//                        Layout.rightMargin: 24
                    Layout.preferredHeight: content.implicitHeight
                    Layout.preferredWidth: content.implicitHeight
                    squareLeftSide: true

                    clickedFunc: function() {
                        if (root.clickedFunc && typeof root.clickedFunc === "function") {
                            root.clickedFunc()
                        }
                    }
                }
            }
        }

        SmallTextType {
            id: errorField

            text: root.errorText
            visible: root.errorText !== ""
            color: "#EB5757"
        }
    }

    MouseArea {
        anchors.fill: root
        cursorShape: Qt.IBeamCursor

        hoverEnabled: true

        onPressed: function(mouse) {
            textField.forceActiveFocus()
            mouse.accepted = false

            backgroud.border.color = getBackgroundBorderColor(root.borderColor)
        }

        onEntered: {
            backgroud.border.color = getBackgroundBorderColor(bgBorderHoveredColor)
        }


        onExited: {
            backgroud.border.color = getBackgroundBorderColor(root.borderColor)
        }
    }

    function getBackgroundBorderColor(noneFocusedColor) {
        return textField.focus ? root.borderFocusedColor : noneFocusedColor
    }

    Keys.onEnterPressed: {
         KeyNavigation.tab.forceActiveFocus();
    }

    Keys.onReturnPressed: {
         KeyNavigation.tab.forceActiveFocus();
    }
}

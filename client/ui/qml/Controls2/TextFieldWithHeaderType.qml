import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "TextTypes"

Item {
    id: root

    property string headerText
    property string headerTextDisabledColor: AmneziaStyle.color.charcoalGray
    property string headerTextColor: AmneziaStyle.color.mutedGray

    property alias errorText: errorField.text
    property bool checkEmptyText: false
    property bool rightButtonClickedOnEnter: false

    property string buttonText
    property string buttonImageSource
    property var clickedFunc

    property alias textField: textField
    property alias textFieldText: textField.text
    property string textFieldTextColor: AmneziaStyle.color.paleGray
    property string textFieldTextDisabledColor: AmneziaStyle.color.mutedGray

    property string textFieldPlaceholderText
    property bool textFieldEditable: true

    property string borderColor: AmneziaStyle.color.slateGray
    property string borderFocusedColor: AmneziaStyle.color.paleGray

    property string backgroundColor: AmneziaStyle.color.onyxBlack
    property string backgroundDisabledColor: AmneziaStyle.color.transparent
    property string bgBorderHoveredColor: AmneziaStyle.color.charcoalGray

    implicitWidth: content.implicitWidth
    implicitHeight: content.implicitHeight

    property FlickableType parentFlickable

    Connections {
        target: textField
        function onFocusChanged() {
            if (textField.activeFocus) {
                if (root.parentFlickable) {
                    root.parentFlickable.ensureVisible(root)
                }
            }
        }
    }

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

                        property bool isFocusable: true

                        Keys.onTabPressed: {
                            FocusController.nextKeyTabItem()
                        }

                        Keys.onBacktabPressed: {
                            FocusController.previousKeyTabItem()
                        }

                        enabled: root.textFieldEditable
                        color: root.enabled ? root.textFieldTextColor : root.textFieldTextDisabledColor

                        inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhSensitiveData | Qt.ImhNoPredictiveText

                        placeholderText: root.textFieldPlaceholderText
                        placeholderTextColor: AmneziaStyle.color.charcoalGray

                        selectionColor:  AmneziaStyle.color.richBrown
                        selectedTextColor: AmneziaStyle.color.paleGray

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
            }
        }

        SmallTextType {
            id: errorField

            text: root.errorText
            visible: root.errorText !== ""
            color: AmneziaStyle.color.vibrantRed

            Layout.fillWidth: true
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

    BasicButtonType {
        visible: (root.buttonText !== "") || (root.buttonImageSource !== "")

        focusPolicy: Qt.NoFocus
        text: root.buttonText
        imageSource: root.buttonImageSource

        anchors.top: content.top
        anchors.bottom: content.bottom
        anchors.right: content.right

        height: content.implicitHeight
        width: content.implicitHeight
        squareLeftSide: true

        clickedFunc: function() {
            if (root.clickedFunc && typeof root.clickedFunc === "function") {
                root.clickedFunc()
            }
        }
    }

    function getBackgroundBorderColor(noneFocusedColor) {
        return textField.focus ? root.borderFocusedColor : noneFocusedColor
    }

    Keys.onEnterPressed: {
        if (root.rightButtonClickedOnEnter && root.clickedFunc && typeof root.clickedFunc === "function") {
            clickedFunc()
        }

        // if (KeyNavigation.tab) {
        //     KeyNavigation.tab.forceActiveFocus();
        // }
    }

    Keys.onReturnPressed: {
        if (root.rightButtonClickedOnEnter &&root.clickedFunc && typeof root.clickedFunc === "function") {
            clickedFunc()
        }

        // if (KeyNavigation.tab) {
        //     KeyNavigation.tab.forceActiveFocus();
        // }
    }
}

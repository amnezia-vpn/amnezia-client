pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import "../Controls2"

TextFieldType {
    id: root

    property var scroller: null
    property bool rangeValidation: false

    signal edited(string text)

    Layout.fillWidth: true
    Layout.leftMargin: 16
    Layout.rightMargin: 16

    checkEmptyText: false

    property RegularExpressionValidator rangeValidator: RegularExpressionValidator {
        regularExpression: /^\d*(-\d*)?$/
    }

    textField.validator: rangeValidation ? rangeValidator : null

    textField.onTextEdited: root.edited(root.textField.text)

    textField.onEditingFinished: {
        const value = root.rangeValidation ? root.textField.text.replace(/^-+|-+$/g, "") : root.textField.text
        root.edited(value)
        if (root.textField.text !== value) {
            root.textField.text = value
        }
    }

    textField.onActiveFocusChanged: {
        if (root.textField.activeFocus && root.scroller) {
            root.scroller.scrollToItem(root)
        }
    }
}

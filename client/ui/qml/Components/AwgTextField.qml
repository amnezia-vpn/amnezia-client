pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import "../Controls2"

TextFieldWithHeaderType {
    id: root

    property var scroller: null
    property bool rangeValidation: false

    signal edited(string text)

    Layout.fillWidth: true
    Layout.topMargin: 16
    Layout.leftMargin: 16
    Layout.rightMargin: 16

    checkEmptyText: false

    property RegularExpressionValidator rangeValidator: RegularExpressionValidator {
        regularExpression: /^\d*(-\d*)?$/
    }

    textField.validator: rangeValidation ? rangeValidator : null

    textField.onEditingFinished: {
        if (root.rangeValidation) {
            root.textField.text = root.textField.text.replace(/^-+|-+$/g, "")
        }
        root.edited(root.textField.text)
    }

    textField.onActiveFocusChanged: {
        if (root.textField.activeFocus && root.scroller) {
            root.scroller.scrollToItem(root)
        }
    }
}

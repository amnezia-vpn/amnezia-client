pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import "../Controls2"

TextFieldWithHeaderType {
    Layout.fillWidth: true
    Layout.topMargin: 16

    textField.validator: IntValidator { bottom: 0 }

    checkEmptyText: true
}

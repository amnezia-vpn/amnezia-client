import QtQuick
import QtTest 1.2

import "../../ui/qml/Controls2"

Item {
    id: root
    width: 360
    height: 120
    property int clickCount: 0

    TextFieldWithHeaderType {
        id: field
        anchors.fill: parent
        rightButtonClickedOnEnter: true
        clickedFunc: function() {
            root.clickCount += 1
        }
    }

    TestCase {
        name: "TextFieldWithHeaderType"
        when: windowShown

        function focusTextField() {
            field.textField.forceActiveFocus()
            verify(field.textField.activeFocus)
        }

        function test_return_without_visible_button_does_not_trigger() {
            root.clickCount = 0
            field.buttonText = ""
            field.textField.text = "secret"
            focusTextField()

            keyClick(Qt.Key_Return)

            compare(root.clickCount, 0)
        }

        function test_enter_with_visible_button_triggers() {
            root.clickCount = 0
            field.buttonText = "Toggle"
            field.textField.text = "secret"
            focusTextField()

            keyClick(Qt.Key_Enter)

            compare(root.clickCount, 1)
        }
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../Controls2"
import "../Controls2/TextTypes"

Item {
    id: root

    property string minValue: "0"
    property string maxValue: "0"

    property int minLimit: 0
    property int maxLimit: 2147483647

    property string minPlaceholder: ""
    property string maxPlaceholder: ""

    property string helperText: root.minLimit > 0
                              ? (root.minLimit + "–" + root.maxLimit)
                              : ("≤ " + root.maxLimit)

    signal minChanged(string val)
    signal maxChanged(string val)

    implicitHeight: col.implicitHeight
    implicitWidth: col.implicitWidth

    function clampValue(text) {
        if (text === "")
            return ""
        var n = parseInt(text, 10)
        if (isNaN(n))
            return ""
        if (n < root.minLimit)
            n = root.minLimit
        if (n > root.maxLimit)
            n = root.maxLimit
        return String(n)
    }

    function effectiveOther(value, placeholder) {
        if (value !== "")
            return parseInt(value, 10)
        if (placeholder !== "")
            return parseInt(placeholder, 10)
        return NaN
    }

    function capEdit(tf, holder) {
        if (tf.text !== "" && parseInt(tf.text, 10) > root.maxLimit) {
            tf.text = holder.lastValid
            tf.cursorPosition = tf.text.length
        } else {
            holder.lastValid = tf.text
        }
    }

    ColumnLayout {
        id: col
        anchors.fill: parent
        spacing: 12

        RowLayout {
            id: row
            Layout.fillWidth: true
            spacing: 8

            TextFieldType {
                id: minField
                property string lastValid: ""
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                headerText: qsTr("Min")
                placeholderText: root.minPlaceholder
                textField.maximumLength: 10
                textField.validator: RegularExpressionValidator { regularExpression: /^\d*$/ }
                textField.onActiveFocusChanged: {
                    if (minField.textField.activeFocus)
                        minField.lastValid = minField.textField.text
                }
                textField.onTextEdited: { root.capEdit(minField.textField, minField); root.minChanged(minField.textField.text) }
                textField.onEditingFinished: {
                    var v = root.clampValue(minField.textField.text)
                    if (v !== "") {
                        var mx = root.effectiveOther(root.maxValue, root.maxPlaceholder)
                        if (!isNaN(mx) && parseInt(v, 10) > mx)
                            root.maxChanged(v)
                    }
                    if (minField.textField.text !== v)
                        minField.textField.text = v
                    root.minChanged(v)
                }

                Binding {
                    target: minField.textField
                    property: "text"
                    value: root.minValue
                    when: !minField.textField.activeFocus
                    restoreMode: Binding.RestoreNone
                }
            }

            TextFieldType {
                id: maxField
                property string lastValid: ""
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                headerText: qsTr("Max")
                placeholderText: root.maxPlaceholder
                textField.maximumLength: 10
                textField.validator: RegularExpressionValidator { regularExpression: /^\d*$/ }
                textField.onActiveFocusChanged: {
                    if (maxField.textField.activeFocus)
                        maxField.lastValid = maxField.textField.text
                }
                textField.onTextEdited: { root.capEdit(maxField.textField, maxField); root.maxChanged(maxField.textField.text) }
                textField.onEditingFinished: {
                    var v = root.clampValue(maxField.textField.text)
                    if (v !== "") {
                        var mn = root.effectiveOther(root.minValue, root.minPlaceholder)
                        if (!isNaN(mn) && parseInt(v, 10) < mn)
                            v = String(mn)
                    }
                    if (maxField.textField.text !== v)
                        maxField.textField.text = v
                    root.maxChanged(v)
                }

                Binding {
                    target: maxField.textField
                    property: "text"
                    value: root.maxValue
                    when: !maxField.textField.activeFocus
                    restoreMode: Binding.RestoreNone
                }
            }
        }

        SmallTextType {
            visible: root.helperText !== ""
            text: root.helperText
            Layout.fillWidth: true
        }
    }
}

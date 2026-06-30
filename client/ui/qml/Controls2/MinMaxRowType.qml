import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"

// MinMaxRowType — two side-by-side labeled text fields: Min / Max
// Usage:
//   MinMaxRowType {
//       minValue: "0"
//       maxValue: "0"
//       onMinChanged: function(val) { someProperty = val }
//       onMaxChanged: function(val) { someProperty = val }
//   }
Item {
    id: root

    property string minValue: "0"
    property string maxValue: "0"

    property int minLimit: 0
    property int maxLimit: 2147483647

    property string hintText: root.minLimit > 0
                              ? (root.minLimit + "–" + root.maxLimit)
                              : ("≤ " + root.maxLimit)

    signal minChanged(string val)
    signal maxChanged(string val)
    signal edited()

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
        spacing: 4

        RowLayout {
            id: row
            Layout.fillWidth: true
            spacing: 10

            // Min field
            TextFieldWithHeaderType {
                id: minField
                property string lastValid: ""
                Layout.fillWidth: true
                headerText: qsTr("Min")
                textField.maximumLength: 10
                textField.validator: RegularExpressionValidator { regularExpression: /^\d*$/ }
                textField.onActiveFocusChanged: {
                    if (minField.textField.activeFocus)
                        minField.lastValid = minField.textField.text
                }
                textField.onTextEdited: { root.capEdit(minField.textField, minField); root.edited() }
                textField.onEditingFinished: {
                    var v = root.clampValue(minField.textField.text)
                    if (v !== "" && root.maxValue !== "") {
                        var mx = parseInt(root.maxValue, 10)
                        if (!isNaN(mx) && parseInt(v, 10) > mx)
                            root.maxChanged(v)
                    }
                    if (v !== root.minValue)
                        root.minChanged(v)
                    else if (minField.textField.text !== v)
                        minField.textField.text = v
                }

                Binding {
                    target: minField.textField
                    property: "text"
                    value: root.minValue
                    when: !minField.textField.activeFocus
                    restoreMode: Binding.RestoreNone
                }
            }

            // Max field
            TextFieldWithHeaderType {
                id: maxField
                property string lastValid: ""
                Layout.fillWidth: true
                headerText: qsTr("Max")
                textField.maximumLength: 10
                textField.validator: RegularExpressionValidator { regularExpression: /^\d*$/ }
                textField.onActiveFocusChanged: {
                    if (maxField.textField.activeFocus)
                        maxField.lastValid = maxField.textField.text
                }
                textField.onTextEdited: { root.capEdit(maxField.textField, maxField); root.edited() }
                textField.onEditingFinished: {
                    var v = root.clampValue(maxField.textField.text)
                    if (v !== "" && root.minValue !== "") {
                        var mn = parseInt(root.minValue, 10)
                        if (!isNaN(mn) && parseInt(v, 10) < mn)
                            v = String(mn)
                    }
                    if (v !== root.maxValue)
                        root.maxChanged(v)
                    else if (maxField.textField.text !== v)
                        maxField.textField.text = v
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
            visible: root.hintText !== ""
            text: root.hintText
            color: AmneziaStyle.color.mutedGray
            Layout.fillWidth: true
        }
    }
}

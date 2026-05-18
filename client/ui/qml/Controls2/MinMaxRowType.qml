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
//       onMinChanged: someProperty = val
//       onMaxChanged: someProperty = val
//   }
Item {
    id: root

    property string minValue: "0"
    property string maxValue: "0"

    signal minChanged(string val)
    signal maxChanged(string val)

    implicitHeight: row.implicitHeight
    implicitWidth: row.implicitWidth

    RowLayout {
        id: row
        anchors.fill: parent
        spacing: 10

        // Min field
        TextFieldWithHeaderType {
            Layout.fillWidth: true
            headerText: qsTr("Min")
            textField.text: root.minValue
            textField.validator: IntValidator { bottom: 0 }
            textField.onEditingFinished: {
                if (textField.text !== root.minValue) {
                    root.minChanged(textField.text)
                }
            }
        }

        // Max field
        TextFieldWithHeaderType {
            Layout.fillWidth: true
            headerText: qsTr("Max")
            textField.text: root.maxValue
            textField.validator: IntValidator { bottom: 0 }
            textField.onEditingFinished: {
                if (textField.text !== root.maxValue) {
                    root.maxChanged(textField.text)
                }
            }
        }
    }
}

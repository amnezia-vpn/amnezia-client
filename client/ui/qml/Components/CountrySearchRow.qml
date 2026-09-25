import QtQuick
import QtQuick.Layouts

import Style 1.0

import "../Controls2"

RowLayout {
    id: root

    property var listModel: null

    property alias searchField: searchField

    signal sortRequested

    Layout.fillWidth: true
    Layout.leftMargin: 16
    Layout.rightMargin: 16
    Layout.topMargin: 12

    spacing: 8

    CountrySearchField {
        id: searchField

        Layout.fillWidth: true

        onTextChanged: {
            const current = searchField.text
            const clean = root.listModel.sanitizeSearchFieldText(current)
            if (clean !== current) {
                const cursor = searchField.textField.cursorPosition - (current.length - clean.length)
                searchField.text = clean
                searchField.textField.cursorPosition = Math.max(0, Math.min(cursor, clean.length))
            }
        }

        onDisplayTextChanged: root.listModel.searchText = root.listModel.sanitizeSearchFieldText(searchField.displayText)
    }

    ImageButtonType {
        objectName: "sortButton"

        implicitWidth: 64
        implicitHeight: 64

        hoverEnabled: true
        image: "qrc:/images/controls/sort-desc.svg"
        imageColor: AmneziaStyle.color.paleGray

        onClicked: root.sortRequested()
    }
}

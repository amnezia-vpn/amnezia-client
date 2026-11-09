import QtQuick
import QtQuick.Layouts

import "../Controls2"

RowLayout {
    id: root

    property var listModel: null

    property alias searchField: searchField

    signal sortRequested

    Layout.fillWidth: true
    Layout.leftMargin: 16
    Layout.rightMargin: 16
    Layout.topMargin: 16

    spacing: 8

    SearchFieldType {
        id: searchField

        Layout.fillWidth: true

        placeholderText: qsTr("Country or region")
        maximumLength: 64

        onTextChanged: {
            const current = searchField.text
            const clean = root.listModel.sanitizeSearchFieldText(current)
            if (clean !== current) {
                const cursor = searchField.cursorPosition - (current.length - clean.length)
                searchField.text = clean
                searchField.cursorPosition = Math.max(0, Math.min(cursor, clean.length))
            }
        }

        onDisplayTextChanged: root.listModel.searchText = root.listModel.sanitizeSearchFieldText(searchField.displayText)
    }

    IconButtonType {
        implicitWidth: 44
        implicitHeight: 44

        icon.source: "qrc:/images/controls/sort-desc.svg"
        icon.width: 20
        icon.height: 20
        Accessible.name: qsTr("Sort")

        onClicked: root.sortRequested()
    }
}

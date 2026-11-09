import QtQuick

import Style 1.0

import "../Controls2"

EmptyStateType {
    id: root

    property bool isSearchResult: true
    property string categoryName: ""

    signal showAllRequested()

    iconColor: AmneziaStyle.color.accentLight

    title: {
        if (root.categoryName !== "") {
            return qsTr("Nothing found in \"%1\"").arg(root.categoryName)
        }
        return root.isSearchResult ? qsTr("Nothing found. Check the spelling or change your query")
                                   : qsTr("No locations available yet")
    }

    ButtonType {
        visible: root.isSearchResult || root.categoryName !== ""
        size: ButtonType.Medium
        variant: ButtonType.Text
        text: qsTr("Show all")

        onClicked: root.showAllRequested()
    }
}

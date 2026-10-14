import QtQuick

import Style 1.0

import "../Controls2"

ListItemType {
    id: root

    property bool favorite: false
    property string countryFlagCode

    signal favoriteToggled()

    leftPadding: 16
    leadingSlotWidth: 24
    icon.source: root.countryFlagCode !== "" ? "qrc:/countriesFlags/images/flagKit/" + root.countryFlagCode + ".svg" : ""
    icon.width: 21
    icon.height: 15
    icon.color: AmneziaStyle.color.transparent

    FavoriteButton {
        favorite: root.favorite

        onClicked: root.favoriteToggled()
    }
}

import QtQuick

import "../Controls2"

IconButtonType {
    id: root

    property bool favorite: false

    icon.source: root.favorite ? "qrc:/images/controls/star-filled.svg" : "qrc:/images/controls/star.svg"

    Accessible.name: root.favorite ? qsTr("Remove from favorites") : qsTr("Add to favorites")
}

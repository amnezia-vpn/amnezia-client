pragma Singleton

import QtQuick

QtObject {
    readonly property var regionNames: ({
        "europe": qsTr("Europe"),
        "middle-east-caucasus": qsTr("Middle East & Caucasus"),
        "asia": qsTr("Asia"),
        "north-america": qsTr("North America"),
        "latin-america": qsTr("Latin America"),
        "oceania-africa": qsTr("Oceania & Africa"),
        "other": qsTr("Other")
    })

    readonly property var subregionNames: ({
        "western-europe": qsTr("Western Europe"),
        "northern-europe": qsTr("Northern Europe"),
        "central-eastern-europe": qsTr("Central & Eastern Europe"),
        "southern-europe": qsTr("Southern Europe")
    })
}

pragma Singleton

import QtQuick

QtObject {
    readonly property var regionNames: ({
        "europe": qsTr("Europe"),
        "asia": qsTr("Asia"),
        "americas": qsTr("Americas"),
        "africa": qsTr("Africa"),
        "oceania": qsTr("Oceania"),
        "special": qsTr("Special"),
        "other": qsTr("Other")
    })

    readonly property var subregionNames: ({
        "northern-europe": qsTr("Northern Europe"),
        "western-europe": qsTr("Western Europe"),
        "eastern-europe": qsTr("Eastern Europe"),
        "southern-europe": qsTr("Southern Europe"),

        "western-asia": qsTr("Western Asia"),
        "central-asia": qsTr("Central Asia"),
        "southern-asia": qsTr("Southern Asia"),
        "eastern-asia": qsTr("Eastern Asia"),
        "south-eastern-asia": qsTr("South-eastern Asia"),

        "northern-america": qsTr("Northern America"),
        "central-america": qsTr("Central America"),
        "caribbean": qsTr("Caribbean"),
        "south-america": qsTr("South America"),

        "northern-africa": qsTr("Northern Africa"),
        "western-africa": qsTr("Western Africa"),
        "middle-africa": qsTr("Middle Africa"),
        "eastern-africa": qsTr("Eastern Africa"),
        "southern-africa": qsTr("Southern Africa"),

        "australia-nz": qsTr("Australia and New Zealand"),
        "melanesia": qsTr("Melanesia"),
        "micronesia": qsTr("Micronesia"),
        "polynesia": qsTr("Polynesia")
    })

    readonly property var useCaseNames: ({
        "favorites": qsTr("Favorites"),
        "all": qsTr("All"),
        "allowlist": qsTr("Allowlists"),
        "created": qsTr("Created")
    })

    readonly property var useCaseBanners: ({
        "allowlist": qsTr("Connecting through these countries lets you bypass allowlist restrictions")
    })
}

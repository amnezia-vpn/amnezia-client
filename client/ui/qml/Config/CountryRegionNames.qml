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

    readonly property var subsubregionNames: ({
        "nordic-countries": qsTr("Nordic countries"),
        "baltic-states": qsTr("Baltic states"),
        "uk-and-ireland": qsTr("United Kingdom and Ireland"),
        "south-western-europe": qsTr("Southwestern Europe"),
        "south-eastern-europe": qsTr("Southeastern Europe"),

        "caucasus-and-turkey": qsTr("Caucasus and Turkey"),
        "arabian-peninsula": qsTr("Arabian Peninsula"),
        "levant-and-mesopotamia": qsTr("Levant and Mesopotamia"),

        "greater-antilles": qsTr("Greater Antilles"),
        "lesser-antilles": qsTr("Lesser Antilles"),
        "bahamas-and-southern-caribbean": qsTr("Bahamas and Southern Caribbean"),

        "sahel": qsTr("Sahel"),
        "gulf-of-guinea": qsTr("Gulf of Guinea"),
        "atlantic-west-africa": qsTr("Atlantic West Africa"),
        "east-africa-mainland": qsTr("East Africa (mainland)"),
        "southeast-africa": qsTr("Southeast Africa"),
        "indian-ocean-islands": qsTr("Indian Ocean islands")
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

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.settings 1.1

import SortFilterProxyModel 0.2

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    property var processedServer
    property var groupedRegions: []
    property string searchText: ""
    property var regionsExpanded: ({})

    readonly property var regionDefinitions: [
        {
            "regionName": "Europe",
            "countries": [
                { "code": "BE", "name": "Belgium", "ruName": "Бельгия" },
                { "code": "EE", "name": "Estonia", "ruName": "Эстония" },
                { "code": "FI", "name": "Finland", "ruName": "Финляндия" },
                { "code": "FR", "name": "France", "ruName": "Франция" },
                { "code": "GE", "name": "Georgia", "ruName": "Грузия" },
                { "code": "DE", "name": "Germany", "ruName": "Германия" },
                { "code": "NL", "name": "Netherlands", "ruName": "Нидерланды" },
                { "code": "PL", "name": "Poland", "ruName": "Польша" },
                { "code": "RU", "name": "Russia", "ruName": "Россия" },
                { "code": "ES", "name": "Spain", "ruName": "Испания" },
                { "code": "SE", "name": "Sweden", "ruName": "Швеция" },
                { "code": "CH", "name": "Switzerland", "ruName": "Швейцария" },
                { "code": "TR", "name": "Turkey", "ruName": "Турция" }
            ]
        },
        {
            "regionName": "America",
            "countries": [
                { "code": "BR", "name": "Brazil", "ruName": "Бразилия" },
                { "code": "CA", "name": "Canada East", "ruName": "Канада" },
                { "code": "US", "name": "USA East", "ruName": "США" },
                { "code": "US", "name": "USA West", "ruName": "США" }
            ]
        },
        {
            "regionName": "Asia",
            "countries": [
                { "code": "AE", "name": "UAE", "ruName": "ОАЭ" },
                { "code": "JP", "name": "Japan", "ruName": "Япония" },
                { "code": "KZ", "name": "Kazakhstan", "ruName": "Казахстан" },
                { "code": "KR", "name": "South Korea", "ruName": "Южная Корея" },
                { "code": "SG", "name": "Singapore", "ruName": "Сингапур" }
            ]
        },
        {
            "regionName": "Oceania and Africa",
            "countries": [
                { "code": "AU", "name": "Australia", "ruName": "Австралия" },
                { "code": "NZ", "name": "New Zealand", "ruName": "Новая Зеландия" },
                { "code": "ZA", "name": "South Africa", "ruName": "Южная Африка" }
            ]
        }
    ]

    function normalizeCountryCode(countryCode) {
        if (!countryCode) {
            return "";
        }
        return countryCode.toString().trim().toUpperCase();
    }

    function extractCountryIsoCode(countryCode) {
        const normalizedCode = normalizeCountryCode(countryCode);
        const match = normalizedCode.match(/[A-Z]{2}/);
        return match ? match[0] : normalizedCode;
    }

    function normalizeCountryName(countryName) {
        if (!countryName) {
            return "";
        }
        return countryName.toString().trim().toLowerCase();
    }

    function loadRegionExpansionState() {
        try {
            const parsed = JSON.parse(regionExpansionSettings.regionsExpandedJson);
            regionsExpanded = parsed && typeof parsed === "object" ? parsed : ({});
        } catch (error) {
            regionsExpanded = ({});
        }
    }

    function saveRegionExpansionState() {
        regionExpansionSettings.regionsExpandedJson = JSON.stringify(regionsExpanded);
    }

    function isRegionExpanded(regionName) {
        if (regionsExpanded[regionName] === undefined) {
            return true;
        }
        return regionsExpanded[regionName];
    }

    function setRegionExpanded(regionName, isExpanded) {
        let updated = Object.assign({}, regionsExpanded);
        updated[regionName] = isExpanded;
        regionsExpanded = updated;
        saveRegionExpansionState();
    }

    function normalizeSearchComparableText(textValue) {
        const normalizedText = normalizeCountryName(textValue)
            .replace(/[ё]/g, "е")
            .replace(/[й]/g, "и");
        let result = "";

        for (let i = 0; i < normalizedText.length; ++i) {
            const currentChar = normalizedText[i];
            const isSeparator = currentChar === "." || currentChar === "-";

            if (!isSeparator) {
                result += currentChar;
                continue;
            }

            const prevChar = i > 0 ? normalizedText[i - 1] : "";
            const nextChar = i + 1 < normalizedText.length ? normalizedText[i + 1] : "";
            const hasSeparatorNeighbor = prevChar === "." || prevChar === "-" || nextChar === "." || nextChar === "-";

            if (hasSeparatorNeighbor) {
                result += currentChar;
            }
        }

        return result;
    }

    function isCountryMatchingSearch(countryName, regionCountryCode, sourceCountryCode, ruCountryName) {
        const normalizedSearchText = normalizeSearchComparableText(searchText);
        if (normalizedSearchText === "") {
            return true;
        }

        const normalizedCountryName = normalizeSearchComparableText(countryName);
        const normalizedRuCountryName = normalizeSearchComparableText(ruCountryName);
        const normalizedRegionCountryCode = normalizeCountryCode(regionCountryCode).toLowerCase();
        const normalizedSourceCountryCode = normalizeCountryCode(sourceCountryCode).toLowerCase();

        const nameMatch = normalizedCountryName.startsWith(normalizedSearchText);
        const ruNameMatch = normalizedRuCountryName.startsWith(normalizedSearchText);
        const regionCodeMatch = normalizedRegionCountryCode.startsWith(normalizedSearchText);
        const sourceCodeMatch = normalizedSourceCountryCode.startsWith(normalizedSearchText);

        return nameMatch || ruNameMatch || regionCodeMatch || sourceCodeMatch;
    }

    function getDisplayCountryName(countryName) {
        const p2pPostfix = "[P2P] ";
        if (countryName && countryName.indexOf(p2pPostfix) === 0) {
            return countryName.slice(p2pPostfix.length) + " " + p2pPostfix;
        }
        return countryName;
    }

    function findCountryIndexByRef(countryRef, usedIndices) {
        const expectedCode = normalizeCountryCode(countryRef.code);
        const expectedName = normalizeCountryName(countryRef.name);
        const countriesCount = proxyCountriesModel.count !== undefined ? proxyCountriesModel.count : 0;

        for (let i = 0; i < countriesCount; ++i) {
            if (usedIndices[i]) {
                continue;
            }

            const country = proxyCountriesModel.get(i);
            if (!country || country.countryCode === undefined || country.countryName === undefined) {
                continue;
            }

            const modelCode = normalizeCountryCode(country.countryCode);
            const modelIsoCode = extractCountryIsoCode(country.countryCode);
            const modelName = normalizeCountryName(country.countryName);

            if (expectedName !== "" && modelName === expectedName) {
                return i;
            }
            if (expectedCode !== "" && (modelCode === expectedCode || modelIsoCode === expectedCode)) {
                return i;
            }
        }

        return -1;
    }

    function rebuildRegionModel() {
        let regions = [];

        for (let regionNameIndex = 0; regionNameIndex < regionDefinitions.length; ++regionNameIndex) {
            regions.push({
                "regionName": regionDefinitions[regionNameIndex].regionName,
                "countries": []
            });
        }

        let usedIndices = {};
        for (let regionDefIndex = 0; regionDefIndex < regionDefinitions.length; ++regionDefIndex) {
            const regionDefinition = regionDefinitions[regionDefIndex];
            for (let countryIndex = 0; countryIndex < regionDefinition.countries.length; ++countryIndex) {
                const countryRef = regionDefinition.countries[countryIndex];
                const sourceIndex = findCountryIndexByRef(countryRef, usedIndices);

                if (sourceIndex < 0) {
                    if (isCountryMatchingSearch(countryRef.name, countryRef.code, countryRef.code, countryRef.ruName)) {
                        regions[regionDefIndex].countries.push({
                            "sourceIndex": -1,
                            "countryName": getDisplayCountryName(countryRef.name),
                            "sourceCountryName": countryRef.name,
                            "countryCode": countryRef.code,
                            "countryImageCode": extractCountryIsoCode(countryRef.code),
                            "isAvailable": false
                        });
                    }
                    continue;
                }

                const sourceCountry = proxyCountriesModel.get(sourceIndex);
                if (!sourceCountry || sourceCountry.countryCode === undefined || sourceCountry.countryName === undefined) {
                    continue;
                }

                const displayCountryName = getDisplayCountryName(sourceCountry.countryName);
                if (!isCountryMatchingSearch(displayCountryName, countryRef.code, sourceCountry.countryCode, countryRef.ruName)) {
                    continue;
                }

                regions[regionDefIndex].countries.push({
                    "sourceIndex": sourceIndex,
                    "countryName": displayCountryName,
                    "sourceCountryName": sourceCountry.countryName,
                    "countryCode": sourceCountry.countryCode,
                    "countryImageCode": extractCountryIsoCode(sourceCountry.countryImageCode),
                    "isAvailable": true
                });
                usedIndices[sourceIndex] = true;
            }
        }

        let visibleRegions = [];
        for (let regionIndex = 0; regionIndex < regions.length; ++regionIndex) {
            if (regions[regionIndex].countries.length > 0) {
                visibleRegions.push(regions[regionIndex]);
            }
        }

        groupedRegions = visibleRegions;
    }

    Connections {
        target: ServersModel

        function onProcessedServerChanged() {
            root.processedServer = proxyServersModel.get(0)
        }
    }

    Connections {
        target: ApiCountryModel

        function onModelReset() {
            root.rebuildRegionModel()
        }

        function onRowsInserted() {
            root.rebuildRegionModel()
        }

        function onRowsRemoved() {
            root.rebuildRegionModel()
        }

        function onDataChanged() {
            root.rebuildRegionModel()
        }
    }

    SortFilterProxyModel {
        id: proxyServersModel
        objectName: "proxyServersModel"

        sourceModel: ServersModel
        filters: [
            ValueFilter {
                roleName: "isCurrentlyProcessed"
                value: true
            }
        ]

        Component.onCompleted: {
            root.processedServer = proxyServersModel.get(0)
            root.rebuildRegionModel()
        }
    }

    SortFilterProxyModel {
        id: proxyCountriesModel
        objectName: "proxyCountriesModel"

        sourceModel: ApiCountryModel
    }

    Settings {
        id: regionExpansionSettings
        category: "PageSettingsApiAvailableCountries"
        property string regionsExpandedJson: "{}"
    }

    ListViewType {
        id: menuContent

        anchors.fill: parent

        model: root.groupedRegions

        currentIndex: 0

        ButtonGroup {
            id: containersRadioButtonGroup
        }

        header: ColumnLayout {
            width: menuContent.width

            spacing: 4

            BackButtonType {
                id: backButton
                objectName: "backButton"

                Layout.topMargin: 20 + SettingsController.safeAreaTopMargin
            }

            HeaderTypeWithButton {
                id: headerContent
                objectName: "headerContent"

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 10

                actionButtonImage: "qrc:/images/controls/settings.svg"

                headerText: root.processedServer.name
                descriptionText: qsTr("Location for connection")

                actionButtonFunction: function() {
                    PageController.showBusyIndicator(true)
                    let result = ApiSettingsController.getAccountInfo(false)
                    PageController.showBusyIndicator(false)
                    if (!result) {
                        return
                    }

                    PageController.goToPage(PageEnum.PageSettingsApiServerInfo)
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 10

                implicitHeight: 56
                radius: 16

                color: AmneziaStyle.color.onyxBlack
                border.color: searchField.activeFocus ? AmneziaStyle.color.paleGray : AmneziaStyle.color.slateGray
                border.width: 1

                Behavior on border.color {
                    PropertyAnimation { duration: 200 }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 8
                    spacing: 8

                    Image {
                        source: "qrc:/images/controls/search.svg"
                    }

                    TextField {
                        id: searchField

                        Layout.fillWidth: true

                        color: AmneziaStyle.color.paleGray
                        placeholderText: "country or country code"
                        placeholderTextColor: AmneziaStyle.color.charcoalGray

                        selectionColor: AmneziaStyle.color.richBrown
                        selectedTextColor: AmneziaStyle.color.paleGray

                        font.pixelSize: 16
                        font.weight: 400
                        font.family: "PT Root UI VF"

                        inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText

                        topPadding: 0
                        rightPadding: 0
                        leftPadding: 0
                        bottomPadding: 0

                        background: Rectangle {
                            color: AmneziaStyle.color.transparent
                        }

                        onTextChanged: {
                            const shouldRestoreFocus = activeFocus
                            const previousCursorPosition = cursorPosition

                            root.searchText = text
                            root.rebuildRegionModel()

                            if (shouldRestoreFocus) {
                                Qt.callLater(function() {
                                    searchField.forceActiveFocus()
                                    searchField.cursorPosition = Math.min(previousCursorPosition, searchField.text.length)
                                })
                            }
                        }

                        Keys.onEscapePressed: {
                            searchField.text = ""
                        }

                        ContextMenu.menu: ContextMenuType {
                            textObj: searchField
                        }
                    }

                    ImageButtonType {
                        visible: searchField.text !== ""

                        implicitWidth: 40
                        implicitHeight: 40

                        hoverEnabled: true
                        image: "qrc:/images/controls/close.svg"
                        imageColor: AmneziaStyle.color.paleGray

                        onClicked: {
                            searchField.text = ""
                        }
                        Keys.onEnterPressed: {
                            searchField.text = ""
                        }
                        Keys.onReturnPressed: {
                            searchField.text = ""
                        }
                    }
                }
            }
        }

        footer: Item {
            width: menuContent.width
            height: groupedRegions.length === 0 ? emptyStateText.implicitHeight + 32 : 0

            CaptionTextType {
                id: emptyStateText

                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                anchors.top: parent.top
                anchors.topMargin: 16

                visible: groupedRegions.length === 0
                color: AmneziaStyle.color.mutedGray

                font.pixelSize: 15
                horizontalAlignment: Text.AlignLeft
                wrapMode: Text.WordWrap
                text: "Nothing found. Try a different spelling or switch keyboard layout."
            }
        }

        delegate: ColumnLayout {
            id: regionContent

            property var regionData: modelData

            width: menuContent.width
            height: regionContent.implicitHeight
            spacing: 0

            Item {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 12
                Layout.bottomMargin: 8

                implicitHeight: 24

                RowLayout {
                    anchors.fill: parent
                    spacing: 8

                    CaptionTextType {
                        Layout.fillWidth: true
                        color: AmneziaStyle.color.mutedGray

                        text: regionData.regionName
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                    }

                    Image {
                        source: root.isRegionExpanded(regionData.regionName)
                                ? "qrc:/images/controls/chevron-up.svg"
                                : "qrc:/images/controls/chevron-down.svg"
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        root.setRegionExpanded(regionData.regionName, !root.isRegionExpanded(regionData.regionName))
                    }
                }
            }

            Repeater {
                model: root.isRegionExpanded(regionData.regionName) ? regionData.countries : []

                delegate: ColumnLayout {
                    property var countryData: modelData

                    width: menuContent.width
                    spacing: 0

                    RowLayout {
                        VerticalRadioButton {
                            id: containerRadioButton

                            Layout.fillWidth: true
                            Layout.leftMargin: 16

                            text: countryData.countryName

                            ButtonGroup.group: containersRadioButtonGroup

                            imageSource: "qrc:/images/controls/download.svg"

                            checked: countryData.sourceIndex >= 0 && countryData.sourceIndex === ApiCountryModel.currentIndex
                            checkable: countryData.isAvailable && !ConnectionController.isConnected

                            onClicked: {
                                if (!countryData.isAvailable) {
                                    return
                                }
                                if (ConnectionController.isConnectionInProgress) {
                                    PageController.showNotificationMessage(qsTr("Unable change server location while trying to make an active connection"))
                                    return
                                }
                                if (ConnectionController.isConnected) {
                                    PageController.showNotificationMessage(qsTr("Unable change server location while there is an active connection"))
                                    return
                                }

                                if (countryData.sourceIndex !== ApiCountryModel.currentIndex) {
                                    PageController.showBusyIndicator(true)
                                    var prevIndex = ApiCountryModel.currentIndex
                                    ApiCountryModel.currentIndex = countryData.sourceIndex
                                    if (!ApiConfigsController.updateServiceFromGateway(ServersModel.defaultIndex, countryData.countryCode, countryData.sourceCountryName)) {
                                        ApiCountryModel.currentIndex = prevIndex
                                    }
                                    PageController.showBusyIndicator(false)
                                }
                            }

                            Keys.onEnterPressed: {
                                if (checkable) {
                                    checked = true
                                }
                                containerRadioButton.clicked()
                            }
                            Keys.onReturnPressed: {
                                if (checkable) {
                                    checked = true
                                }
                                containerRadioButton.clicked()
                            }
                        }

                        Image {
                            Layout.rightMargin: 32
                            Layout.alignment: Qt.AlignRight

                            source: "qrc:/countriesFlags/images/flagKit/" + countryData.countryImageCode + ".svg"
                        }
                    }

                    DividerType {
                        Layout.fillWidth: true
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        loadRegionExpansionState()
    }
}

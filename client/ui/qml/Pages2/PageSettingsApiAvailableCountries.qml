import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

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
    property var regionsExpanded: ({})

    readonly property var regionDefinitions: [
        {
            "regionName": "Northern Europe",
            "countries": [
                { "code": "SE", "name": "Sweden" },
                { "code": "FI", "name": "Finland" },
                { "code": "EE", "name": "Estonia" },
                { "code": "IE", "name": "Ireland" },
                { "code": "NO", "name": "Norway" },
                { "code": "DK", "name": "Denmark" },
                { "code": "IS", "name": "Iceland" }
            ]
        },
        {
            "regionName": "Western Europe",
            "countries": [
                { "code": "DE", "name": "Germany" },
                { "code": "NL", "name": "Netherlands" },
                { "code": "CH", "name": "Switzerland" },
                { "code": "FR", "name": "France" },
                { "code": "BE", "name": "Belgium" },
                { "code": "PT", "name": "Portugal" },
                { "code": "IM", "name": "Isle of Man" }
            ]
        },
        {
            "regionName": "Central Europe",
            "countries": [
                { "code": "PL", "name": "Poland" },
                { "code": "AT", "name": "Austria" },
                { "code": "LU", "name": "Luxembourg" },
                { "code": "SK", "name": "Slovakia" }
            ]
        },
        {
            "regionName": "Southern Europe & Mediterranean",
            "countries": [
                { "code": "ES", "name": "Spain" },
                { "code": "IT", "name": "Italy" },
                { "code": "CY", "name": "Cyprus" },
                { "code": "MT", "name": "Malta" }
            ]
        },
        {
            "regionName": "Eastern Europe & Baltics",
            "countries": [
                { "code": "RU", "name": "Russia" },
                { "code": "HU", "name": "Hungary" },
                { "code": "RO", "name": "Romania" },
                { "code": "LV", "name": "Latvia" },
                { "code": "LT", "name": "Lithuania" }
            ]
        },
        {
            "regionName": "Balkans",
            "countries": [
                { "code": "BG", "name": "Bulgaria" },
                { "code": "HR", "name": "Croatia" },
                { "code": "BA", "name": "Bosnia and Herzegovina" },
                { "code": "RS", "name": "Serbia" }
            ]
        },
        {
            "regionName": "Caucasus Region",
            "countries": [
                { "code": "GE", "name": "Georgia" }
            ]
        },
        {
            "regionName": "Eurasian Region",
            "countries": [
                { "code": "KZ", "name": "Kazakhstan" },
                { "code": "TR", "name": "Turkey" }
            ]
        },
        {
            "regionName": "East Asia",
            "countries": [
                { "code": "JP", "name": "Japan" },
                { "code": "KR", "name": "South Korea" },
                { "code": "TW", "name": "Taiwan" },
                { "code": "HK", "name": "Hong Kong" }
            ]
        },
        {
            "regionName": "Southeast Asia",
            "countries": [
                { "code": "SG", "name": "Singapore" },
                { "code": "PH", "name": "Philippines" },
                { "code": "VN", "name": "Vietnam" },
                { "code": "ID", "name": "Indonesia" }
            ]
        },
        {
            "regionName": "Central Asia",
            "countries": [
            ]
        },
        {
            "regionName": "Middle East",
            "countries": [
                { "code": "AE", "name": "UAE" }
            ]
        },
        {
            "regionName": "Oceania",
            "countries": [
                { "code": "AU", "name": "Australia" },
                { "code": "NZ", "name": "New Zealand" }
            ]
        },
        {
            "regionName": "North America",
            "countries": [
                { "code": "US", "name": "USA East" },
                { "code": "US", "name": "USA West" },
                { "code": "CA", "name": "Canada East" },
                { "code": "CA", "name": "Canada West" },
                { "code": "MX", "name": "Mexico" }
            ]
        },
        {
            "regionName": "Central America",
            "countries": [
                { "code": "CR", "name": "Costa Rica" },
                { "code": "PA", "name": "Panama" },
                { "code": "GT", "name": "Guatemala" }
            ]
        },
        {
            "regionName": "South America",
            "countries": [
                { "code": "BR", "name": "Brazil" },
                { "code": "AR", "name": "Argentina" },
                { "code": "CL", "name": "Chile" },
                { "code": "CO", "name": "Colombia" },
                { "code": "BO", "name": "Bolivia" },
                { "code": "PE", "name": "Peru" }
            ]
        },
        {
            "regionName": "Africa",
            "countries": [
                { "code": "ZA", "name": "South Africa" },
                { "code": "KE", "name": "Kenya" }
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
            const modelName = normalizeCountryName(country.countryName);

            if (expectedName !== "" && modelName === expectedName) {
                return i;
            }
            if (expectedCode !== "" && modelCode === expectedCode) {
                return i;
            }
        }

        return -1;
    }

    function rebuildRegionModel() {
        let regions = [];

        for (let regionIndex = 0; regionIndex < regionDefinitions.length; ++regionIndex) {
            regions.push({
                "regionName": regionDefinitions[regionIndex].regionName,
                "countries": []
            });
        }

        let usedIndices = {};
        for (let regionIndex = 0; regionIndex < regionDefinitions.length; ++regionIndex) {
            const regionDefinition = regionDefinitions[regionIndex];
            for (let countryIndex = 0; countryIndex < regionDefinition.countries.length; ++countryIndex) {
                const countryRef = regionDefinition.countries[countryIndex];
                const sourceIndex = findCountryIndexByRef(countryRef, usedIndices);

                if (sourceIndex < 0) {
                    continue;
                }

                const sourceCountry = proxyCountriesModel.get(sourceIndex);
                if (!sourceCountry || sourceCountry.countryCode === undefined || sourceCountry.countryName === undefined) {
                    continue;
                }

                regions[regionIndex].countries.push({
                    "sourceIndex": sourceIndex,
                    "countryName": sourceCountry.countryName,
                    "countryCode": sourceCountry.countryCode,
                    "countryImageCode": extractCountryIsoCode(sourceCountry.countryImageCode)
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
        }

        delegate: ColumnLayout {
            id: regionContent

            property var regionData: modelData

            width: menuContent.width
            height: regionContent.implicitHeight
            spacing: 0

            LabelWithButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 4
                Layout.bottomMargin: 4

                textColor: AmneziaStyle.color.paleGray
                borderFocusedColor: AmneziaStyle.color.paleGray

                text: regionData.regionName
                rightImageSource: root.isRegionExpanded(regionData.regionName) ? "qrc:/images/controls/chevron-up.svg" : "qrc:/images/controls/chevron-down.svg"
                textMaximumLineCount: 1
                textElide: Qt.ElideRight

                clickedFunction: function() {
                    root.setRegionExpanded(regionData.regionName, !root.isRegionExpanded(regionData.regionName))
                }
            }

            DividerType {
                Layout.fillWidth: true
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

                            text: countryData.countryName + " (" + root.normalizeCountryCode(countryData.countryCode) + ")"

                            ButtonGroup.group: containersRadioButtonGroup

                            imageSource: "qrc:/images/controls/download.svg"

                            checked: countryData.sourceIndex === ApiCountryModel.currentIndex
                            checkable: !ConnectionController.isConnected

                            onClicked: {
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
                                    if (!ApiConfigsController.updateServiceFromGateway(ServersModel.defaultIndex, countryData.countryCode, countryData.countryName)) {
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
}

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QtCore

import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    isTabBarHidden: true

    property string configExtension: ".conf"
    property string configCaption: qsTr("Save AmneziaVPN config")

    Component.onCompleted: {
        ApiConfigsCountryListModel.applyDefaultState(false)
    }

    property int stickyRevision: 0
    property real savedScroll: 0

    Connections {
        target: ApiConfigsCountryListModel

        function onLayoutRebuilt() {
            Qt.callLater(function() {
                menuContent.forceLayout()
                root.stickyRevision += 1
            })
        }

        function onSourceAboutToRefresh() {
            root.savedScroll = menuContent.contentY - menuContent.originY
        }

        function onSourceRefreshed() {
            Qt.callLater(function() {
                menuContent.forceLayout()
                const maxScroll = Math.max(0, menuContent.contentHeight - menuContent.height)
                menuContent.contentY = menuContent.originY + Math.min(root.savedScroll, maxScroll)
            })
        }

        function onFavoritesLimitExceeded() {
            favoritesLimitToast.show(qsTr("You already have %1 locations in favorites. Remove one")
                                     .arg(ApiConfigsCountryListModel.favoritesLimit),
                                     qsTr("Show"))
        }
    }

    function openConfigOptions(countryCode, countryName) {
        moreOptionsDrawer.countryName = countryName
        moreOptionsDrawer.countryCode = countryCode
        moreOptionsDrawer.open()
    }

    function downloadConfig(countryCode, countryName, isIssued) {
        if (isIssued) {
            root.showQuestion(true, countryCode, countryName)
        } else {
            root.issueConfig(countryCode)
        }
    }

    ListViewType {
        id: menuContent

        anchors.top: parent.top
        anchors.topMargin: collapsingHeader.topBarHeight
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.bottomMargin: searchRow.searchField.activeFocus ? 0 : PageController.imeHeight

        model: ApiConfigsCountryListModel

        interactive: menuContent.contentHeight > menuContent.height

        ScrollBar.vertical: collapsingHeader.scrollBar

        header: collapsingHeader.listHeader

        footer: Item {
            width: menuContent.width
            height: 16 + PageController.safeAreaBottomMargin
        }

        delegate: Item {
            id: rowItem

            required property int index
            required property string rowType
            required property string sectionKey
            required property string countryName
            required property string countryCode
            required property string countryImageCode
            required property bool isIssued
            required property bool isWorkerExpired
            required property bool isFavorite

            width: menuContent.width

            implicitHeight: rowItem.rowType === "section" ? sectionHeader.implicitHeight : 72
            height: implicitHeight

            CountrySectionHeader {
                id: sectionHeader

                listModel: ApiConfigsCountryListModel
                width: rowItem.width
                visible: rowItem.rowType === "section"

                sectionKey: rowItem.rowType === "section" ? rowItem.sectionKey : ""
                row: rowItem.rowType === "section" ? rowItem.index : -1
            }

            CountryListItem {
                x: 16
                width: rowItem.width - 32
                height: rowItem.height

                visible: rowItem.rowType === "country"

                text: rowItem.countryName
                subtitle: rowItem.isWorkerExpired ? qsTr("Update available") : ""
                maximumLineCount: 1
                countryFlagCode: rowItem.countryImageCode
                favorite: rowItem.isFavorite

                onClicked: {
                    if (rowItem.isIssued) {
                        root.openConfigOptions(rowItem.countryCode, rowItem.countryName)
                    } else {
                        root.issueConfig(rowItem.countryCode)
                    }
                }
                onFavoriteToggled: ApiConfigsCountryListModel.toggleFavorite(rowItem.countryCode)

                IconButtonType {
                    icon.source: "qrc:/images/controls/download.svg"
                    Accessible.name: qsTr("Download configuration file")

                    onClicked: root.downloadConfig(rowItem.countryCode, rowItem.countryName, rowItem.isIssued)
                }

                IconButtonType {
                    visible: rowItem.isIssued
                    icon.source: "qrc:/images/controls/more-vertical.svg"
                    Accessible.name: qsTr("Configuration file options")

                    onClicked: root.openConfigOptions(rowItem.countryCode, rowItem.countryName)
                }
            }
        }
    }

    CollapsingHeaderType {
        id: collapsingHeader

        anchors.fill: parent

        listView: menuContent
        title: qsTr("Configuration files")

        collapsibleContent: ColumnLayout {
            spacing: 0

            BodyTextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 8
                Layout.bottomMargin: 16

                text: qsTr("For router setup or the AmneziaWG app")
                color: AmneziaStyle.color.textSecondary
            }
        }

        pinnedContent: [
            CountrySearchRow {
                id: searchRow

                listModel: ApiConfigsCountryListModel

                onSortRequested: sortDrawer.open()
            },

            CountryUseCaseChips {
                id: useCaseChips

                listModel: ApiConfigsCountryListModel

                Layout.fillWidth: true
                Layout.topMargin: 12
                Layout.preferredHeight: useCaseChips.implicitHeight
            },

            NotificationType {
                id: warning

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 24

                text: qsTr("New configurations are available for some locations. We recommend updating the files")

                visible: ApiCountryModel.hasExpiredWorkerConfigs
            },

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: ApiConfigsCountryListModel.isGrouped ? 0 : 12
            }
        ]

        CountryListStickyHeader {
            listModel: ApiConfigsCountryListModel
            listView: menuContent
            header: collapsingHeader
            revision: root.stickyRevision
        }

        CountriesEmptyState {
            anchors.top: parent.top
            anchors.topMargin: collapsingHeader.pinnedBottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.bottomMargin: searchRow.searchField.activeFocus ? 0 : PageController.imeHeight

            visible: !ApiConfigsCountryListModel.hasResults

            isSearchResult: ApiConfigsCountryListModel.isSearchActive
            categoryName: ApiConfigsCountryListModel.activeUseCaseId !== "all"
                          ? CountryRegionNames.useCaseName(ApiConfigsCountryListModel, ApiConfigsCountryListModel.activeUseCaseId)
                          : ""

            onShowAllRequested: {
                searchRow.searchField.clear()
                ApiConfigsCountryListModel.activeUseCaseId = "all"
            }
        }
    }

    Connections {
        target: collapsingHeader.backButton

        function onActiveFocusChanged() {
            if (collapsingHeader.backButton.enabled && collapsingHeader.backButton.activeFocus) {
                menuContent.positionViewAtBeginning()
            }
        }
    }

    SortCountriesDrawer {
        id: sortDrawer

        listModel: ApiConfigsCountryListModel
    }

    SheetType {
        id: moreOptionsDrawer

        property string countryName
        property string countryCode

        expandedHeight: moreOptionsContent.implicitHeight + 16 + PageController.safeAreaBottomMargin

        ColumnLayout {
            id: moreOptionsContent

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 16

            spacing: 0

            SectionHeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 32

                headerText: moreOptionsDrawer.countryName + qsTr(" configuration file")
            }

            ListItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Generate a new configuration file")
                subtitle: qsTr("The previously created one will stop working")

                onClicked: {
                    root.showQuestion(true, moreOptionsDrawer.countryCode, moreOptionsDrawer.countryName)
                }
            }

            ListItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Revoke the current configuration file")

                onClicked: {
                    root.showQuestion(false, moreOptionsDrawer.countryCode, moreOptionsDrawer.countryName)
                }
            }
        }
    }

    function issueConfig(countryCode) {
        var fileName = ""
        if (GC.isMobile()) {
            fileName = countryCode + configExtension
        } else {
            fileName = SystemController.getFileName(configCaption,
                                                    qsTr("Config files (*" + configExtension + ")"),
                                                    StandardPaths.standardLocations(StandardPaths.DocumentsLocation) + "/" + countryCode,
                                                    true,
                                                    configExtension)
        }
        if (fileName !== "") {
            PageController.showBusyIndicator(true)
            let result = SubscriptionUiController.exportNativeConfig(ServersUiController.processedServerId, countryCode, fileName)

            PageController.showBusyIndicator(false)
            if (result) {
                PageController.showNotificationMessage(qsTr("Config file saved"))
            }
        }
    }

    function revokeConfig(countryCode) {
        PageController.showBusyIndicator(true)
        let result = SubscriptionUiController.revokeNativeConfig(ServersUiController.processedServerId, countryCode)
        if (result) {
            SubscriptionUiController.getAccountInfo(ServersUiController.processedServerId, true)
        }
        PageController.showBusyIndicator(false)

        if (result) {
            PageController.showNotificationMessage(qsTr("The config has been revoked"))
        }
    }

    function showQuestion(isConfigIssue, countryCode, countryName) {
        var headerText
        if (isConfigIssue) {
            headerText = qsTr("Generate a new %1 configuration file?").arg(countryName)
        } else {
            headerText = qsTr("Revoke the current %1 configuration file?").arg(countryName)
        }

        var descriptionText = qsTr("Your previous configuration file will no longer work, and it will not be possible to connect using it")
        var yesButtonText = isConfigIssue ? qsTr("Download") : qsTr("Continue")
        var noButtonText = qsTr("Cancel")

        var yesButtonFunction = function() {
            if (isConfigIssue) {
                root.issueConfig(countryCode)
            } else {
                root.revokeConfig(countryCode)
            }
            moreOptionsDrawer.close()
        }
        var noButtonFunction = function() {}

        showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
    }

    SnackbarType {
        id: favoritesLimitToast

        onActionTriggered: ApiConfigsCountryListModel.activeUseCaseId = "favorites"
    }
}

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
            PageController.showNotificationMessage(qsTr("You can add up to %1 locations to favorites")
                                                   .arg(ApiConfigsCountryListModel.favoritesLimit))
        }
    }

    function openConfigOptions(countryCode, countryName) {
        moreOptionsDrawer.countryName = countryName
        moreOptionsDrawer.countryCode = countryCode
        moreOptionsDrawer.openTriggered()
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

        anchors.top: topBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.bottomMargin: searchField.textField.activeFocus ? 0 : PageController.imeHeight

        model: ApiConfigsCountryListModel

        visible: ApiConfigsCountryListModel.hasResults

        interactive: menuContent.contentHeight > menuContent.height

        footer: Item {
            width: menuContent.width
            height: 16
        }

        delegate: Item {
            id: rowItem

            required property string rowType
            required property string sectionKey
            required property string countryName
            required property string countryCode
            required property string countryImageCode
            required property bool isIssued
            required property bool isWorkerExpired
            required property bool isFavorite

            width: menuContent.width

            implicitHeight: rowItem.rowType === "section" ? 60 : 72
            height: implicitHeight

            CountrySectionHeader {
                listModel: ApiConfigsCountryListModel
                width: rowItem.width
                visible: rowItem.rowType === "section"

                sectionKey: rowItem.rowType === "section" ? rowItem.sectionKey : ""
            }

            Item {
                id: countryRow

                width: rowItem.width
                height: rowItem.height
                visible: rowItem.rowType === "country"

                Item {
                    id: rowBody

                    property bool isFocusable: countryRow.visible

                    anchors.fill: parent

                    function activate() {
                        if (rowItem.isIssued) {
                            root.openConfigOptions(rowItem.countryCode, rowItem.countryName)
                        } else {
                            root.issueConfig(rowItem.countryCode)
                        }
                    }

                    HoverHandler {
                        id: rowHover
                        cursorShape: Qt.PointingHandCursor
                    }

                    TapHandler {
                        gesturePolicy: TapHandler.ReleaseWithinBounds

                        onTapped: function(eventPoint) {
                            if (buttons.contains(buttons.mapFromItem(rowBody, eventPoint.position))) {
                                return
                            }
                            rowBody.activate()
                        }
                    }

                    Keys.onEnterPressed: rowBody.activate()
                    Keys.onReturnPressed: rowBody.activate()
                    Keys.onSpacePressed: rowBody.activate()
                    Keys.onTabPressed: FocusController.nextKeyTabItem()
                    Keys.onBacktabPressed: FocusController.previousKeyTabItem()
                    Keys.onUpPressed: FocusController.nextKeyUpItem()
                    Keys.onDownPressed: FocusController.nextKeyDownItem()
                    Keys.onLeftPressed: FocusController.nextKeyLeftItem()
                    Keys.onRightPressed: FocusController.nextKeyRightItem()

                    Rectangle {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16
                        anchors.topMargin: 4
                        anchors.bottomMargin: 4
                        radius: 16

                        color: ((rowHover.hovered && !buttonsHover.hovered) || rowBody.activeFocus)
                               ? AmneziaStyle.color.surfaceHovered
                               : AmneziaStyle.color.transparent
                    }

                    Image {
                        id: flag

                        x: 28
                        anchors.verticalCenter: parent.verticalCenter
                        width: 24
                        height: 16

                        source: rowItem.countryImageCode !== ""
                                ? "qrc:/countriesFlags/images/flagKit/" + rowItem.countryImageCode + ".svg"
                                : ""
                    }

                    ColumnLayout {
                        anchors.left: flag.right
                        anchors.leftMargin: 16
                        anchors.right: parent.right
                        anchors.rightMargin: 28 + buttons.width + 8
                        anchors.verticalCenter: parent.verticalCenter

                        spacing: 0

                        ListItemTitleType {
                            Layout.fillWidth: true

                            text: rowItem.countryName
                            color: AmneziaStyle.color.textPrimary
                            maximumLineCount: 1
                            elide: Text.ElideRight
                        }

                        CaptionTextType {
                            Layout.fillWidth: true

                            visible: rowItem.isWorkerExpired
                            text: qsTr("Download update")
                            color: AmneziaStyle.color.textTertiary
                        }
                    }
                }

                Row {
                    id: buttons

                    anchors.right: parent.right
                    anchors.rightMargin: 28
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 12

                    HoverHandler {
                        id: buttonsHover
                    }

                    Item {
                        id: star

                        property bool isFocusable: countryRow.visible

                        width: 40
                        height: 40

                        function toggle() {
                            ApiConfigsCountryListModel.toggleFavorite(rowItem.countryCode)
                        }

                        Accessible.name: rowItem.isFavorite ? qsTr("Remove from favorites")
                                                            : qsTr("Add to favorites")

                        HoverHandler {
                            id: starHover
                            cursorShape: Qt.PointingHandCursor
                        }

                        TapHandler {
                            gesturePolicy: TapHandler.ReleaseWithinBounds
                            onTapped: star.toggle()
                        }

                        Keys.onEnterPressed: star.toggle()
                        Keys.onReturnPressed: star.toggle()
                        Keys.onSpacePressed: star.toggle()
                        Keys.onTabPressed: FocusController.nextKeyTabItem()
                        Keys.onBacktabPressed: FocusController.previousKeyTabItem()
                        Keys.onUpPressed: FocusController.nextKeyUpItem()
                        Keys.onDownPressed: FocusController.nextKeyDownItem()
                        Keys.onLeftPressed: FocusController.nextKeyLeftItem()
                        Keys.onRightPressed: FocusController.nextKeyRightItem()

                        Rectangle {
                            anchors.fill: parent
                            radius: 12

                            color: (starHover.hovered || star.activeFocus) ? AmneziaStyle.color.surfaceHovered
                                                                           : AmneziaStyle.color.transparent
                            border.width: star.activeFocus ? 1 : 0
                            border.color: AmneziaStyle.color.borderSoft
                        }

                        Image {
                            anchors.centerIn: parent
                            width: 24
                            height: 24

                            source: rowItem.isFavorite ? "qrc:/images/controls/star-filled.svg"
                                                       : "qrc:/images/controls/star.svg"
                        }
                    }

                    ImageButtonType {
                        implicitWidth: 40
                        implicitHeight: 40

                        visible: countryRow.visible
                        hoverEnabled: true
                        image: "qrc:/images/controls/download.svg"
                        imageColor: AmneziaStyle.color.paleGray

                        onClicked: root.downloadConfig(rowItem.countryCode, rowItem.countryName, rowItem.isIssued)
                    }

                    ImageButtonType {
                        implicitWidth: 40
                        implicitHeight: 40

                        visible: countryRow.visible && rowItem.isIssued
                        hoverEnabled: true
                        image: "qrc:/images/controls/more-vertical.svg"
                        imageColor: AmneziaStyle.color.paleGray

                        onClicked: root.openConfigOptions(rowItem.countryCode, rowItem.countryName)
                    }
                }

                DividerType {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    anchors.bottom: parent.bottom
                }
            }
        }
    }

    CountrySectionHeader {
        id: stickyHeader

        listModel: ApiConfigsCountryListModel

        anchors.top: topBar.bottom
        anchors.topMargin: stickyHeader.pushOffset
        anchors.left: parent.left
        anchors.right: parent.right

        readonly property int topRow: {
            root.stickyRevision
            return menuContent.indexAt(menuContent.width / 2, menuContent.contentY + 1)
        }

        readonly property real pushOffset: {
            root.stickyRevision
            const probe = menuContent.indexAt(menuContent.width / 2, menuContent.contentY + 60)
            if (probe < 0 || probe === stickyHeader.topRow || !ApiConfigsCountryListModel.isSectionHeaderRow(probe)) {
                return 0
            }
            const next = menuContent.itemAtIndex(probe)
            if (!next) {
                return 0
            }
            return Math.min(0, next.y - menuContent.contentY - 60)
        }

        onToggled: function(sectionKey) {
            Qt.callLater(function() {
                const headerRow = ApiConfigsCountryListModel.rowForSectionHeader(sectionKey)
                if (headerRow >= 0) {
                    menuContent.positionViewAtIndex(headerRow, ListView.Beginning)
                }
            })
        }

        isPinnedOverlay: true
        sectionKey: stickyHeader.topRow >= 0 ? ApiConfigsCountryListModel.sectionKeyAtRow(stickyHeader.topRow) : ""

        visible: ApiConfigsCountryListModel.isGrouped
                 && menuContent.visible
                 && menuContent.contentHeight > menuContent.height
                 && menuContent.contentY > menuContent.originY
                 && stickyHeader.sectionKey !== ""
    }

    CountriesEmptyState {
        anchors.top: topBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.bottomMargin: searchField.textField.activeFocus ? 0 : PageController.imeHeight

        visible: !ApiConfigsCountryListModel.hasResults

        isSearchResult: ApiConfigsCountryListModel.isSearchActive
        categoryName: ApiConfigsCountryListModel.activeUseCaseId !== "all"
                      ? (CountryRegionNames.useCaseNames[ApiConfigsCountryListModel.activeUseCaseId]
                         || ApiConfigsCountryListModel.activeUseCaseId)
                      : ""

        onShowAllRequested: {
            searchField.clear()
            ApiConfigsCountryListModel.activeUseCaseId = "all"
        }
    }

    Rectangle {
        id: topBar

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        color: AmneziaStyle.color.midnightBlack

        implicitHeight: topBarContent.implicitHeight

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
        }

        ColumnLayout {
            id: topBarContent

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top

            spacing: 4

            BackButtonType {
                id: backButton
                objectName: "backButton"

                Layout.fillWidth: true
                Layout.topMargin: 20 + PageController.safeAreaTopMargin

                onActiveFocusChanged: {
                    if (backButton.enabled && backButton.activeFocus) {
                        menuContent.positionViewAtBeginning()
                    }
                }
            }

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                headerText: qsTr("Configuration files")
                descriptionText: qsTr("For router setup or the AmneziaWG app")
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 12

                spacing: 8

                CountrySearchField {
                    id: searchField

                    Layout.fillWidth: true

                    onTextChanged: ApiConfigsCountryListModel.searchText = searchField.text
                }

                ImageButtonType {
                    objectName: "sortButton"

                    implicitWidth: 64
                    implicitHeight: 64

                    hoverEnabled: true
                    image: "qrc:/images/controls/sort-desc.svg"
                    imageColor: AmneziaStyle.color.paleGray

                    onClicked: sortDrawer.openTriggered()
                }
            }

            WarningType {
                id: warning

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 12

                backGroundColor: AmneziaStyle.color.surfaceBase

                textString: qsTr("Configuration updates are available for some countries. Download and install the updated configuration files")

                iconPath: "qrc:/images/controls/info.svg"

                visible: ApiCountryModel.hasExpiredWorkerConfigs
            }

            CountryUseCaseChips {
                id: useCaseChips

                listModel: ApiConfigsCountryListModel

                Layout.fillWidth: true
                Layout.topMargin: 12
                Layout.preferredHeight: useCaseChips.implicitHeight
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: ApiConfigsCountryListModel.isGrouped ? 0 : 12
            }
        }
    }

    SortCountriesDrawer {
        id: sortDrawer

        listModel: ApiConfigsCountryListModel

        anchors.fill: parent
    }

    DrawerType2 {
        id: moreOptionsDrawer

        property string countryName
        property string countryCode

        anchors.fill: parent
        expandedHeight: parent.height * 0.4375

        expandedStateContent: Item {
            implicitHeight: moreOptionsDrawer.expandedHeight

            BackButtonType {
                id: moreOptionsDrawerBackButton

                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.topMargin: 16

                backButtonFunction: function() {
                    moreOptionsDrawer.closeTriggered()
                }
            }

            ListViewType {
                id: drawerListView

                anchors.top: moreOptionsDrawerBackButton.bottom
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right

                header: ColumnLayout {
                    width: drawerListView.width

                    Header2Type {
                        Layout.fillWidth: true
                        Layout.margins: 16

                        headerText: moreOptionsDrawer.countryName + qsTr(" configuration file")
                    }
                }

                model: 1 // fake model to force the ListView to be created without a model

                delegate: ColumnLayout {
                    width: drawerListView.width

                    LabelWithButtonType {
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16

                        text: qsTr("Generate a new configuration file")
                        descriptionText: qsTr("The previously created one will stop working")

                        clickedFunction: function() {
                            root.showQuestion(true, moreOptionsDrawer.countryCode, moreOptionsDrawer.countryName)
                        }
                    }

                    DividerType {}
                }

                footer: ColumnLayout {
                    width: drawerListView.width

                    LabelWithButtonType {
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16

                        text: qsTr("Revoke the current configuration file")

                        clickedFunction: function() {
                            root.showQuestion(false, moreOptionsDrawer.countryCode, moreOptionsDrawer.countryName)
                        }
                    }

                    DividerType {}
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
            moreOptionsDrawer.closeTriggered()
        }
        var noButtonFunction = function() {}

        showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
    }
}

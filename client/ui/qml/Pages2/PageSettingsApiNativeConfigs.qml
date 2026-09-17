import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QtCore

import Style 1.0

import "./"
import "../Controls2"
import "../Config"
import "../Components"

PageType {
    id: root

    property string configExtension: ".conf"
    property string configCaption: qsTr("Save AmneziaVPN config")

    Component.onCompleted: {
        ApiConfigsCountryListModel.clearSearch()
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

            width: menuContent.width

            implicitHeight: rowItem.rowType === "section"
                            ? 60
                            : (rowItem.isWorkerExpired ? 88 : 72)
            height: implicitHeight

            CountrySectionHeader {
                listModel: ApiConfigsCountryListModel
                width: rowItem.width
                visible: rowItem.rowType === "section"

                sectionKey: rowItem.rowType === "section" ? rowItem.sectionKey : ""
            }

            ColumnLayout {
                width: rowItem.width
                height: rowItem.height
                visible: rowItem.rowType === "country"
                spacing: 0

                LabelWithButtonType {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    text: rowItem.countryName

                    descriptionText: rowItem.isWorkerExpired ? qsTr("Download update") : ""
                    hideDescription: !rowItem.isWorkerExpired
                    descriptionColor: AmneziaStyle.color.vibrantRed

                    leftImageSource: rowItem.countryImageCode !== ""
                                     ? "qrc:/countriesFlags/images/flagKit/" + rowItem.countryImageCode + ".svg"
                                     : ""

                    secondaryImageSource: rowItem.isIssued ? "qrc:/images/controls/download.svg" : ""
                    rightImageSource: rowItem.isIssued ? "qrc:/images/controls/more-vertical.svg"
                                                       : "qrc:/images/controls/download.svg"

                    secondaryClickedFunction: function() {
                        root.showQuestion(true, rowItem.countryCode, rowItem.countryName)
                    }

                    clickedFunction: function() {
                        if (rowItem.isIssued) {
                            moreOptionsDrawer.countryName = rowItem.countryName
                            moreOptionsDrawer.countryCode = rowItem.countryCode
                            moreOptionsDrawer.openTriggered()
                        } else {
                            root.issueConfig(rowItem.countryCode)
                        }
                    }
                }

                DividerType {
                    Layout.fillWidth: true
                }
            }
        }
    }

    CountrySectionHeader {
        id: stickyHeader

        listModel: ApiConfigsCountryListModel

        anchors.top: topBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        readonly property int topRow: menuContent.indexAt(menuContent.width / 2, menuContent.contentY + 1)

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

        onShowAllRequested: searchField.clear()
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

                Layout.bottomMargin: warning.visible ? 0 : (ApiConfigsCountryListModel.isGrouped ? 0 : 12)

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
                    Keys.onEnterPressed: sortDrawer.openTriggered()
                    Keys.onReturnPressed: sortDrawer.openTriggered()
                }
            }

            WarningType {
                id: warning

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 12
                Layout.bottomMargin: ApiConfigsCountryListModel.isGrouped ? 0 : 12

                backGroundColor: AmneziaStyle.color.translucentRichBrown

                textString: qsTr("Configuration updates are available for some countries. Download and install the updated configuration files")

                iconPath: "qrc:/images/controls/alert-circle.svg"

                visible: ApiCountryModel.hasExpiredWorkerConfigs
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

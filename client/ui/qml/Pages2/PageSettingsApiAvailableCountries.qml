import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

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

    Connections {
        target: ServersModel

        function onProcessedServerChanged() {
            root.processedServer = proxyServersModel.get(0)
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
        }
    }

    ListViewType {
        id: menuContent

        anchors.fill: parent

        model: ApiCountriesRegionModel

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

                            ApiCountriesRegionModel.searchText = text

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
            height: ApiCountriesRegionModel.count === 0 ? emptyStateText.implicitHeight + 32 : 0

            CaptionTextType {
                id: emptyStateText

                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                anchors.top: parent.top
                anchors.topMargin: 16

                visible: ApiCountriesRegionModel.count === 0
                color: AmneziaStyle.color.mutedGray

                font.pixelSize: 15
                horizontalAlignment: Text.AlignLeft
                wrapMode: Text.WordWrap
                text: "Nothing found. Try a different spelling or switch keyboard layout."
            }
        }

        delegate: ColumnLayout {
            id: regionContent

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

                        text: regionName
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                    }

                    Image {
                        source: ApiCountriesRegionModel.isRegionExpanded(regionName)
                                ? "qrc:/images/controls/chevron-up.svg"
                                : "qrc:/images/controls/chevron-down.svg"
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        ApiCountriesRegionModel.toggleRegionExpanded(regionName)
                    }
                }
            }

            Repeater {
                model: ApiCountriesRegionModel.isRegionExpanded(regionName) ? countries : []

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

}

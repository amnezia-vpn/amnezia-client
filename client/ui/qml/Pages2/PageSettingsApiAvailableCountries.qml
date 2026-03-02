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
    readonly property int topBarHeight: 20 + SettingsController.safeAreaTopMargin + backButton.implicitHeight + 12

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

    Rectangle {
        id: topBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: root.topBarHeight
        color: "black"
        z: 10

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
        }

        BackButtonType {
            id: backButton
            objectName: "backButton"

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.topMargin: 20 + SettingsController.safeAreaTopMargin
            anchors.leftMargin: 16
            z: 1
        }
    }

    ListViewType {
        id: menuContent

        anchors.fill: parent

        model: ApiCountryModel.regionRowsModel

        currentIndex: 0

        ButtonGroup {
            id: containersRadioButtonGroup
        }

        header: ColumnLayout {
            width: menuContent.width

            spacing: 4

            HeaderTypeWithButton {
                id: headerContent
                objectName: "headerContent"

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: topBar.height + 4
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

                            ApiCountryModel.searchText = text

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
            height: ApiCountryModel.hasVisibleRegions ? 0 : emptyStateText.implicitHeight + 32

            CaptionTextType {
                id: emptyStateText

                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                anchors.top: parent.top
                anchors.topMargin: 16

                visible: !ApiCountryModel.hasVisibleRegions
                color: AmneziaStyle.color.mutedGray

                font.pixelSize: 15
                horizontalAlignment: Text.AlignLeft
                wrapMode: Text.WordWrap
                text: "Nothing found. Try a different spelling or switch keyboard layout."
            }
        }

        delegate: Item {
            width: menuContent.width
            implicitHeight: rowType === "region" ? 44 : 88

            Item {
                anchors.fill: parent
                visible: rowType === "region"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    anchors.topMargin: 12
                    anchors.bottomMargin: 8
                    spacing: 8

                    CaptionTextType {
                        Layout.fillWidth: true
                        color: AmneziaStyle.color.mutedGray
                        text: regionName
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                    }

                    Image {
                        source: isExpanded ? "qrc:/images/controls/chevron-up.svg"
                                           : "qrc:/images/controls/chevron-down.svg"
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        ApiCountryModel.toggleRegionExpanded(regionName)
                    }
                }
            }

            ColumnLayout {
                anchors.fill: parent
                visible: rowType === "country"
                spacing: 0

                RowLayout {
                    Layout.fillWidth: true

                    VerticalRadioButton {
                        id: containerRadioButton
                        Layout.fillWidth: true
                        Layout.leftMargin: 16

                        text: countryName
                        ButtonGroup.group: containersRadioButtonGroup
                        imageSource: "qrc:/images/controls/download.svg"

                        checked: sourceIndex >= 0 && sourceIndex === ApiCountryModel.currentIndex
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

                            if (sourceIndex !== ApiCountryModel.currentIndex) {
                                PageController.showBusyIndicator(true)
                                var prevIndex = ApiCountryModel.currentIndex
                                ApiCountryModel.currentIndex = sourceIndex
                                if (!ApiConfigsController.updateServiceFromGateway(ServersModel.defaultIndex, countryCode, sourceCountryName)) {
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
                        source: "qrc:/countriesFlags/images/flagKit/" + countryImageCode + ".svg"
                    }
                }

                DividerType {
                    Layout.fillWidth: true
                }
            }
        }
    }

}

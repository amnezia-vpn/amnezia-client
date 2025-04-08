import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Config 1.0

import "../Components"
import "../Controls"
import "../Controls/TextTypes"

Page {
    id: root

    ColumnLayout {
        anchors.fill: parent

        RowLayout {
            Layout.leftMargin: 8
            Layout.rightMargin: 8
            Layout.topMargin: 8

            WhiteButtonNoBorder {
                id: backButton
                imageSource: "qrc:/images/controls/arrow-left.svg"
                
                onClicked: PageController.closePage()
            }
        }

        Header1TextType {
            Layout.topMargin: 8
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.fillWidth: true

            text: qsTr("Amnezia Premium servers")

            horizontalAlignment: Qt.AlignLeft
            verticalAlignment: Qt.AlignVCenter
        }

        ButtonGroup {
            id: countriesRadioButtonGroup
        }

        ListView {
            id: countriesListView

            Layout.topMargin: 16
            Layout.fillHeight: true
            Layout.fillWidth: true

            model: ApiCountryModel
            currentIndex: ApiCountryModel.currentIndex

            ScrollBar.vertical: ScrollBar {}

            delegate: Item {
                id: menuContentDelegate
                required property string countryName
                required property string countryCode
                required property string countryImageCode
                required property int index

                implicitWidth: countriesListView.width
                implicitHeight: countryItem.implicitHeight

                RadioButton {
                    id: countryItem

                    anchors.fill: parent
                    anchors.rightMargin: 16
                    anchors.leftMargin: 16

                    ButtonGroup.group: countriesRadioButtonGroup

                    checked: index === countriesListView.currentIndex

                    indicator: Item { }

                    contentItem: Item {
                        id: contentContainer

                        anchors.left: parent.left
                        anchors.right: parent.right

                        implicitHeight: content.implicitHeight

                        Rectangle {
                            anchors.fill: parent
                            radius: 8
                            color: countryItem.checked ? Style.color.gray1 : Style.color.transparent
                        }

                        RowLayout {
                            id: content
                            anchors.fill: parent

                            Header3TextType {
                                Layout.fillWidth: true
                                Layout.leftMargin: 8
                                Layout.topMargin: 19
                                Layout.bottomMargin: 19

                                text: countryName

                                color: countryItem.hovered ? Style.color.gray9 : Style.color.black
                            }

                            Image {
                                Layout.rightMargin: 8
                                width: 32
                                height: 24
                                source: "qrc:/countriesFlags/images/flagKit/" + countryImageCode + ".svg"
                            }
                        }
                    }

                    onClicked: function() {
                        if (ConnectionController.isConnected) {
                            PageController.showNotificationMessage(qsTr("Unable change server location while there is an active connection"))
                            return
                        }

                        PageController.showBusyIndicator(true)
                        var prevIndex = ApiCountryModel.currentIndex
                        ApiCountryModel.currentIndex = index
                        if (!ApiConfigsController.updateServiceFromGateway(ServersModel.defaultIndex, countryCode, countryName)) {
                            ApiCountryModel.currentIndex = prevIndex
                        }
                        PageController.showBusyIndicator(false)
                        PageController.closePage()
                    }

                    MouseArea {
                        anchors.fill: countryItem
                        cursorShape: Qt.PointingHandCursor
                        enabled: false
                    }
                }
            }
        }
    }
} 
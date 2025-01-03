import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    ListView {
        id: menuContent

        property bool isFocusable: true

        width: parent.width
        height: parent.height

        clip: true
        interactive: true
        model: ApiCountryModel

        ButtonGroup {
            id: containersRadioButtonGroup
        }

        delegate: ColumnLayout {
            id: content

            width: menuContent.width
            height: content.implicitHeight

            RowLayout {
                VerticalRadioButton {
                    id: containerRadioButton

                    Layout.fillWidth: true
                    Layout.leftMargin: 16

                    text: countryName

                    ButtonGroup.group: containersRadioButtonGroup

                    imageSource: "qrc:/images/controls/download.svg"

                    checked: index === ApiCountryModel.currentIndex
                    checkable: !ConnectionController.isConnected

                    onClicked: {
                        if (ConnectionController.isConnected) {
                            PageController.showNotificationMessage(qsTr("Unable change server location while there is an active connection"))
                            return
                        }

                        if (index !== ApiCountryModel.currentIndex) {
                            PageController.showBusyIndicator(true)
                            var prevIndex = ApiCountryModel.currentIndex
                            ApiCountryModel.currentIndex = index
                            if (!InstallController.updateServiceFromApi(ServersModel.defaultIndex, countryCode, countryName)) {
                                ApiCountryModel.currentIndex = prevIndex
                            }
                        }
                    }

                    MouseArea {
                        anchors.fill: containerRadioButton
                        cursorShape: Qt.PointingHandCursor
                        enabled: false
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

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

        property var selectedText

        width: parent.width
        height: menuContent.contentItem.height

        clip: true
        interactive: false
        model: ApiCountryModel

        ButtonGroup {
            id: containersRadioButtonGroup
        }

        delegate: Item {
            implicitWidth: parent.width
            implicitHeight: content.implicitHeight

            ColumnLayout {
                id: content

                anchors.fill: parent

                RowLayout {
                    VerticalRadioButton {
                        id: containerRadioButton

                        Layout.fillWidth: true
                        Layout.leftMargin: 16

                        text: countryName

                        ButtonGroup.group: containersRadioButtonGroup

                        imageSource: "qrc:/images/controls/download.svg"

                        checked: index === ApiCountryModel.currentIndex

                        onClicked: {
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
}

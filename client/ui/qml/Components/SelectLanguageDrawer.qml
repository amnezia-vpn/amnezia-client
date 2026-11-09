import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

SheetType {
    id: root

    expandedHeight: windowHeight * 0.9

    Item {
        id: container

        anchors.fill: parent

        ColumnLayout {
            id: headerLayout

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0

            Item {
                Layout.fillWidth: true
                Layout.topMargin: 2
                Layout.preferredHeight: 48

                IconButtonType {
                    id: backButton

                    x: 4
                    y: 0
                    icon.source: "qrc:/images/controls/arrow-left.svg"
                    Accessible.name: qsTr("Back")

                    onClicked: root.close()
                }
            }

            SectionHeaderType {
                id: header

                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 32

                headerText: qsTr("Choose language")
            }
        }

        ListViewType {
            id: listView

            anchors.top: headerLayout.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom

            property int selectedIndex: LanguageUiController.currentLanguageIndex

            model: LanguageModel

            footer: Item {
                width: listView.width
                height: 16 + PageController.safeAreaBottomMargin
            }

            delegate: Item {
                width: listView.width
                implicitHeight: languageItem.implicitHeight

                RadioItemType {
                    id: languageItem

                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16

                    text: languageName

                    checked: listView.selectedIndex === index

                    onClicked: {
                        listView.selectedIndex = index
                        LanguageUiController.changeLanguage(languageIndex)
                        root.close()
                    }
                }
            }
        }
    }
}

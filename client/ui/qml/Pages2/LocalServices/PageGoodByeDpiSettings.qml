import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../../Controls2"
import "../../Config"
import "../../Controls2/TextTypes"
import "../../Components"

PageType {
    id: root

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20
    }

    FlickableType {
        id: fl
        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        contentHeight: content.height

        ColumnLayout {
            id: content

            property bool isGoodbyeDpiEnabled: LocalServicesController.isGoodbyeDpiEnabled

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 16
            anchors.rightMargin: 16

            spacing: 16

            HeaderType {
                Layout.fillWidth: true

                headerText: qsTr("GoodbyeDPI settings")
                descriptionText: qsTr("Deep Packet Inspection circumvention utility")
            }

            SwitcherType {
                Layout.fillWidth: true

                text: qsTr("Enable GoodbyeDPI")

                checked: LocalServicesController.isGoodbyeDpiEnabled
                onCheckedChanged: {
                    if (checked !== LocalServicesController.isGoodbyeDpiEnabled) {
                        LocalServicesController.toggleGoodbyeDpi(checked)
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 16

                enabled: !content.isGoodbyeDpiEnabled

                ListItemTitleType {
                    Layout.fillWidth: true

                    text: LocalServicesController.getGoodbyeDpiBlackListFile()
                }

                ImageButtonType {
                    image: "qrc:/images/controls/folder-search-2.svg"
                    imageColor: AmneziaStyle.color.paleGray

                    onClicked: function() {
                        var fileName = SystemController.getFileName(qsTr("Open black list file"),
                                                                    qsTr("Text files (*.txt)"))

                        LocalServicesController.setGoodbyeDpiBlackListFile(fileName)
                    }
                }

                ImageButtonType {
                    image: "qrc:/images/controls/trash.svg"
                    imageColor: AmneziaStyle.color.paleGray

                    onClicked: function() {
                        LocalServicesController.resetGoodbyeDpiBlackListFile()
                    }
                }
            }

            DropDownType {
                id: modsetDropDown
                Layout.fillWidth: true

                descriptionText: qsTr("Setup templates")
                headerText: qsTr("Modset")

                drawerParent: root

                enabled: !content.isGoodbyeDpiEnabled

                listView: ListViewWithRadioButtonType {
                    id: modsetListView

                    rootWidth: root.width

                    model: ListModel {
                        ListElement { name : "-p -r -s -f 2 -k 2 -n -e 2" }
                        ListElement { name : "-p -r -s -f 2 -k 2 -n -e 40" }
                        ListElement { name : "-p -r -s -e 40" }
                        ListElement { name : "-p -r -s" }
                        ListElement { name : "-f 2 -e 2 --auto-ttl --reverse-frag --max-payload" }
                        ListElement { name : "-f 2 -e 2 --wrong-seq --reverse-frag --max-payload" }
                        ListElement { name : "-f 2 -e 2 --wrong-chksum --reverse-frag --max-payload" }
                        ListElement { name : "-f 2 -e 2 --wrong-seq --wrong-chksum --reverse-frag --max-payload" }
                        ListElement { name : "-f 2 -e 2 --wrong-seq --wrong-chksum --reverse-frag --max-payload -q" }
                    }

                    clickedFunction: function() {
                        modsetDropDown.text = selectedText
                        LocalServicesController.setGoodbyeDpiModset(currentIndex + 1)
                        modsetDropDown.close()
                    }

                    Component.onCompleted: {
                        modsetListView.currentIndex = (LocalServicesController.getGoodbyeDpiModset() - 1)
                        modsetListView.triggerCurrentItem()
                    }
                }
            }

            BasicButtonType {
                id: detailedInstructionsButton
                implicitHeight: 32

                defaultColor: AmneziaStyle.color.transparent
                hoveredColor: AmneziaStyle.color.translucentWhite
                pressedColor: AmneziaStyle.color.sheerWhite
                disabledColor: AmneziaStyle.color.mutedGray
                textColor: AmneziaStyle.color.goldenApricot

                text: qsTr("Description of options")

                clickedFunc: function() {
                    Qt.openUrlExternally("https://github.com/ValdikSS/GoodbyeDPI?tab=readme-ov-file#how-to-use")
                }
            }
        }
    }

}

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

PageType {
    id: root

    defaultActiveFocusItem: focusItem

    ColumnLayout {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        spacing: 0

        Item {
            id: focusItem
            KeyNavigation.tab: backButton
        }

        BackButtonType {
            id: backButton
            Layout.topMargin: 20
//                KeyNavigation.tab: fileButton.rightButton
        }

        HeaderType {
            Layout.fillWidth: true
            Layout.topMargin: 8
            Layout.rightMargin: 16
            Layout.leftMargin: 16
            Layout.bottomMargin: 16

            headerText: qsTr("VPN by Amnezia")
            descriptionText: qsTr("Choose a VPN service that suits your needs.")
        }
    }

    ListView {
        id: servicesListView
        anchors.top: header.bottom
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.topMargin: 16
        spacing: 0

        currentIndex: 1
        clip: true
        model: ApiServicesModel

        ScrollBar.vertical: ScrollBar {}

        delegate: Item {
            implicitWidth: servicesListView.width
            implicitHeight: delegateContent.implicitHeight

            ColumnLayout {
                id: delegateContent

                anchors.fill: parent

                CardWithIconsType {
                    id: card

                    Layout.fillWidth: true
                    Layout.rightMargin: 16
                    Layout.leftMargin: 16
                    Layout.bottomMargin: 16

                    headerText: name
                    bodyText: cardDescription
                    footerText: price

                    rightImageSource: "qrc:/images/controls/chevron-right.svg"

                    enabled: isServiceAvailable

                    onClicked: {
                        if (isServiceAvailable) {
                            ApiServicesModel.setServiceIndex(index)
                            PageController.goToPage(PageEnum.PageSetupWizardApiServiceInfo)
                        }
                    }
                }
            }
        }
    }
}

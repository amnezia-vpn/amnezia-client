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

    ColumnLayout {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        spacing: 0

        BackButtonType {
            id: backButton
            Layout.topMargin: 20
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

        property bool isFocusable: true
        
        clip: true
        reuseItems: true

        model: ApiServicesModel

        ScrollBar.vertical: ScrollBarType {}

        delegate: Item {
            implicitWidth: servicesListView.width
            implicitHeight: delegateContent.implicitHeight

            enabled: isServiceAvailable

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

                    onClicked: {
                        if (isServiceAvailable) {
                            ApiServicesModel.setServiceIndex(index)
                            PageController.goToPage(PageEnum.PageSetupWizardApiServiceInfo)
                        }
                    }
                    
                    Keys.onEnterPressed: clicked()
                    Keys.onReturnPressed: clicked()
                }
            }
        }
    }
}

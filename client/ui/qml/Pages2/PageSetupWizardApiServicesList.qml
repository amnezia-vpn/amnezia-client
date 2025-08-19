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

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        Layout.topMargin: 20

        onActiveFocusChanged: {
            if(backButton.enabled && backButton.activeFocus) {
                listView.positionViewAtBeginning()
            }
        }
    }

    ListViewType {
        id: listView

        anchors.top: backButton.bottom
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.topMargin: 16

        header: ColumnLayout {
            width: listView.width

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.rightMargin: 16
                Layout.leftMargin: 16
                Layout.bottomMargin: 24

                headerText: qsTr("VPN by Amnezia")
                descriptionText: qsTr("Choose a VPN service that suits your needs.")
            }
        }

        spacing: 0

        model: ApiServicesModel

        delegate: ColumnLayout {

            width: listView.width

            enabled: isServiceAvailable

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

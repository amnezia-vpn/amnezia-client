import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import PageEnum 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

PageType {
    id: root

    defaultActiveFocusItem: focusItem

    property var serviceInfo: ApiServicesModel.getSelectedServiceInfo()

    FlickableType {
        id: fl
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        contentHeight: content.height

        ColumnLayout {
            id: content

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
                KeyNavigation.tab: fileButton.rightButton
            }

            HeaderType {
                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                headerText: serviceInfo["name"]
                descriptionText: serviceInfo["description"]
            }

            BasicButtonType {
                id: continueButton

                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                text: qsTr("Connect")

                clickedFunc: function() {
                    PageController.showBusyIndicator(true)
                    if (InstallController.installServiceFromApi()) {
                        PageController.goToStartPage()
                        if (stackView.currentItem.objectName === PageController.getPagePath(PageEnum.PageSetupWizardStart)) {
                            PageController.replaceStartPage()
                        }
                    }
                    PageController.showBusyIndicator(false)
                }
            }
        }
    }
}

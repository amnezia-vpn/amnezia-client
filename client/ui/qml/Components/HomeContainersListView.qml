import QtQuick
import QtQuick.Controls

import PageEnum 1.0

import "../Controls2"

ListViewType {
    id: root

    signal activated()

    leftMargin: 16
    rightMargin: 16
    bottomMargin: 16

    delegate: RadioItemType {
        width: root.width - root.leftMargin - root.rightMargin

        text: name
        subtitle: description
        maximumLineCount: 6
        showSeparator: index < root.count - 1

        icon.source: isInstalled ? "" : "qrc:/images/controls/download.svg"

        checked: root.model.mapToSource(index) === ServersUiController.serverDefaultContainer(ServersUiController.defaultServerId)

        onClicked: {
            if (ConnectionController.isConnected && isInstalled) {
                PageController.showNotificationMessage(qsTr("Unable change protocol while there is an active connection"))
                return
            }

            var containerIndex = root.model.mapToSource(index)

            if (!isInstalled) {
                ServersUiController.setProcessedServerId(ServersUiController.defaultServerId)
                ServersUiController.processedContainerIndex = containerIndex
                root.activated()
                PageController.goToPage(PageEnum.PageSetupWizardProtocolSettings)
                return
            }

            root.activated()
            ServersUiController.setDefaultContainer(ServersUiController.defaultServerId, containerIndex)
        }
    }
}

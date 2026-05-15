import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ContainerProps 1.0

import "../Controls2"
import "../Controls2/TextTypes"


ListViewType {
    id: root

    anchors.fill: parent

    delegate: ColumnLayout {
        width: root.width

        LabelWithButtonType {
            Layout.fillWidth: true

            text: name
            descriptionText: description
            rightImageSource: isInstalled ? "qrc:/images/controls/chevron-right.svg" : "qrc:/images/controls/download.svg"

            clickedFunction: function() {
                if (isInstalled) {
                    var containerIndex = root.model.mapToSource(index)
                    ServersUiController.processedContainerIndex = containerIndex

                    if (isVpnContainer) {
                        // var isThirdPartyConfig = root.model.data(index, ContainersModel.IsThirdPartyConfigRole)
                        if (isThirdPartyConfig) {
                            InstallController.updateProtocols(ServersUiController.getServerId(ServersUiController.processedServerIndex), containerIndex)
                            PageController.goToPage(PageEnum.PageProtocolRaw)
                            return
                        }
                    }

                    if (isIpsec) {
                        InstallController.updateProtocols(ServersUiController.getServerId(ServersUiController.processedServerIndex), containerIndex)
                        PageController.goToPage(PageEnum.PageProtocolRaw)
                    } else if (isDns) {
                        PageController.goToPage(PageEnum.PageServiceDnsSettings)
                    } else if (isMtProxy) {
                        MtProxyConfigModel.updateModel(config)
                        PageController.goToPage(PageEnum.PageServiceMtProxySettings)
                    } else {
                        InstallController.updateProtocols(ServersUiController.getServerId(ServersUiController.processedServerIndex), containerIndex)
                        PageController.goToPage(PageEnum.PageSettingsServerProtocol)
                    }

                } else {
                    var containerIndex = root.model.mapToSource(index)
                    ServersUiController.processedContainerIndex = containerIndex
                    PageController.goToPage(PageEnum.PageSetupWizardProtocolSettings)
                }
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                enabled: false
            }
        }

        DividerType {}
    }
}

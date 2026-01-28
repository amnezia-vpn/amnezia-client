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
                    ContainersModel.setProcessedContainerIndex(containerIndex)

                    if (isVpnContainer) {
                        var isThirdPartyConfig = config[ContainerProps.containerTypeToString(containerIndex)]["isThirdPartyConfig"]
                        if (isThirdPartyConfig) {
                            InstallController.updateProtocols(config)
                            PageController.goToPage(PageEnum.PageProtocolRaw)
                            return
                        }
                    }

                    if (isIpsec) {
                        InstallController.updateProtocols(config)
                        PageController.goToPage(PageEnum.PageProtocolRaw)
                    } else if (isDns) {
                        PageController.goToPage(PageEnum.PageServiceDnsSettings)
                    } else {
                        InstallController.updateProtocols(config)
                        PageController.goToPage(PageEnum.PageSettingsServerProtocol)
                    }

                } else {
                    ContainersModel.setProcessedContainerIndex(root.model.mapToSource(index))
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

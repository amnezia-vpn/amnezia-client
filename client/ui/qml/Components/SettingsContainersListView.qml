import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ProtocolEnum 1.0
import ContainerEnum 1.0
import ContainerProps 1.0

import "../Controls2"
import "../Controls2/TextTypes"


ListView {
    id: root

    width: parent.width
    height: root.contentItem.height

    clip: true
    interactive: false

    delegate: Item {
        implicitWidth: root.width
        implicitHeight: delegateContent.implicitHeight

        ColumnLayout {
            id: delegateContent

            anchors.fill: parent

            LabelWithButtonType {
                implicitWidth: parent.width

                text: name
                descriptionText: description
                rightImageSource: isInstalled ? "qrc:/images/controls/chevron-right.svg" : "qrc:/images/controls/download.svg"

                clickedFunction: function() {
                    if (isInstalled) {
                        var containerIndex = root.model.mapToSource(index)
                        ContainersModel.setCurrentlyProcessedContainerIndex(containerIndex)

                        if (serviceType !== ProtocolEnum.Other) {
                            if (config[ContainerProps.containerTypeToString(containerIndex)]["isThirdPartyConfig"]) {
                                ProtocolsModel.updateModel(config)
                                PageController.goToPage(PageEnum.PageProtocolRaw)
                                return
                            }
                        }

                        switch (containerIndex) {
                        case ContainerEnum.OpenVpn: {
                            OpenVpnConfigModel.updateModel(config)
                            PageController.goToPage(PageEnum.PageProtocolOpenVpnSettings)
                            break
                        }
                        case ContainerEnum.Xray: {
                            XrayConfigModel.updateModel(config)
                            PageController.goToPage(PageEnum.PageProtocolXraySettings)
                            break
                        }

                        case ContainerEnum.WireGuard: {
                            WireGuardConfigModel.updateModel(config)
                            PageController.goToPage(PageEnum.PageProtocolWireGuardSettings)
                            break
                        }
                        case ContainerEnum.Awg: {
                            AwgConfigModel.updateModel(config)
                            PageController.goToPage(PageEnum.PageProtocolAwgSettings)
                            break
                        }
                        case ContainerEnum.Ipsec: {
                            ProtocolsModel.updateModel(config)
                            PageController.goToPage(PageEnum.PageProtocolRaw)
                            break
                        }
                        case ContainerEnum.Sftp: {
                            SftpConfigModel.updateModel(config)
                            PageController.goToPage(PageEnum.PageServiceSftpSettings)
                            break
                        }
                        case ContainerEnum.TorWebSite: {
                            PageController.goToPage(PageEnum.PageServiceTorWebsiteSettings)
                            break
                        }
                        case ContainerEnum.Dns: {
                            PageController.goToPage(PageEnum.PageServiceDnsSettings)
                            break
                        }
                        default: { // go to the settings page of the container with multiple protocols
                            ProtocolsModel.updateModel(config)
                            PageController.goToPage(PageEnum.PageSettingsServerProtocol)
                        }
                        }

                    } else {
                        ContainersModel.setCurrentlyProcessedContainerIndex(root.model.mapToSource(index))
                        InstallController.setShouldCreateServer(false)
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
}

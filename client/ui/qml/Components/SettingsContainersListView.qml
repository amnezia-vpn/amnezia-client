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


ListViewType {
    id: root
    
    // Вычисляем высоту содержимого на основе количества элементов
    property int itemHeight: 60 // Примерная высота одного элемента
    implicitHeight: model ? model.count * itemHeight : 0

    delegate: ColumnLayout {
        width: root.width
        
        property bool itemInstalled: model ? (isInstalled || false) : false
        property string itemName: model ? (name || "") : ""
        property string itemDescription: model ? (description || "") : ""

        LabelWithButtonType {
            Layout.fillWidth: true

            text: itemName
            descriptionText: itemDescription
            rightImageSource: itemInstalled ? "qrc:/images/controls/chevron-right.svg" : "qrc:/images/controls/download.svg"

            clickedFunction: function() {
                if (itemInstalled && model) {
                    var containerIndex = root.model.mapToSource(index)
                    ContainersModel.setProcessedContainerIndex(containerIndex)

                    if (serviceType !== ProtocolEnum.Other) {
                        if (config[ContainerProps.containerTypeToString(containerIndex)]["isThirdPartyConfig"]) {
                            ProtocolsModel.updateModel(config)
                            PageController.goToPage(PageEnum.PageProtocolRaw)
                            return
                        }
                    }

                    switch (containerIndex) {
                    case ContainerEnum.Ipsec: {
                        ProtocolsModel.updateModel(config)
                        PageController.goToPage(PageEnum.PageProtocolRaw)
                        break
                    }
                    case ContainerEnum.Dns: {
                        PageController.goToPage(PageEnum.PageServiceDnsSettings)
                        break
                    }
                    case ContainerEnum.Sftp: {
                        PageController.goToPage(PageEnum.PageServiceSftpSettings)
                        break
                    }
                    case ContainerEnum.Socks5Proxy: {
                        PageController.goToPage(PageEnum.PageServiceSocksProxySettings)
                        break
                    }
                    case ContainerEnum.TorWebSite: {
                        PageController.goToPage(PageEnum.PageServiceTorWebsiteSettings)
                        break
                    }
                    case ContainerEnum.CryptPad: {
                        PageController.goToPage(PageEnum.PageCryptPad)
                        break
                    }
                    default: {
                        ProtocolsModel.updateModel(config)
                        PageController.goToPage(PageEnum.PageSettingsServerProtocol)
                    }
                    }

                } else if (model) {
                    ContainersModel.setProcessedContainerIndex(root.model.mapToSource(index))
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

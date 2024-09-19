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

    activeFocusOnTab: true
    // Keys.onTabPressed: {
    //     if (currentIndex < this.count - 1) {
    //         this.incrementCurrentIndex()
    //     } else {
    //         currentIndex = 0
    //         lastItemTabClickedSignal()
    //     }
    // }

    onCurrentIndexChanged: {
        if (visible) {
            if (fl.contentHeight > fl.height) {
                var item = this.currentItem
                if (item.y < fl.height) {
                    fl.contentY = item.y
                } else if (item.y + item.height > fl.contentY + fl.height) {
                    fl.contentY = item.y + item.height - fl.height
                }
            }
        }
    }

    onVisibleChanged: {
        if (visible) {
            this.currentIndex = 0
        }
    }

    delegate: Item {
        implicitWidth: root.width
        implicitHeight: delegateContent.implicitHeight

        onActiveFocusChanged: {
            if (activeFocus) {
                containerRadioButton.rightButton.forceActiveFocus()
            }
        }

        ColumnLayout {
            id: delegateContent

            anchors.fill: parent

            LabelWithButtonType {
                id: containerRadioButton
                implicitWidth: parent.width

                text: name
                descriptionText: description
                rightImageSource: isInstalled ? "qrc:/images/controls/chevron-right.svg" : "qrc:/images/controls/download.svg"

                clickedFunction: function() {
                    if (isInstalled) {
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
                        default: {
                            ProtocolsModel.updateModel(config)
                            PageController.goToPage(PageEnum.PageSettingsServerProtocol)
                        }
                        }

                    } else {
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
}

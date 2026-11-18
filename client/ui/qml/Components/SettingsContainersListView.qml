import QtQuick
import QtQuick.Controls.impl
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "../Controls2"

ListViewType {
    id: root

    anchors.fill: parent

    delegate: Item {
        id: delegateRoot

        readonly property bool isOutdatedAwgContainer: Boolean(isInstalled && ServersUiController.isContainerOutdatedAwg(root.model.mapToSource(index)))

        implicitWidth: root.width
        implicitHeight: containerItem.implicitHeight

        ListItemType {
            id: containerItem

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 16
            anchors.rightMargin: 16

            text: name
            subtitle: description
            maximumLineCount: 4
            trailingIconSource: isInstalled ? "qrc:/images/controls/chevron-right.svg" : "qrc:/images/controls/download.svg"
            trailingContentVisible: delegateRoot.isOutdatedAwgContainer

            Item {
                Layout.preferredWidth: 48
                Layout.preferredHeight: 48

                IconImage {
                    anchors.centerIn: parent
                    width: 24
                    height: 24

                    source: "qrc:/images/controls/alert-circle.svg"
                    sourceSize: Qt.size(24, 24)
                    color: AmneziaStyle.color.statusWarning
                }
            }

            onClicked: {
                if (isInstalled) {
                    var containerIndex = root.model.mapToSource(index)
                    ServersUiController.processedContainerIndex = containerIndex

                    if (isVpnContainer) {
                        if (isThirdPartyConfig) {
                            InstallController.updateProtocols(ServersUiController.processedServerId, containerIndex)
                            PageController.goToPage(PageEnum.PageProtocolRaw)
                            return
                        }
                    }

                    if (isIpsec) {
                        InstallController.updateProtocols(ServersUiController.processedServerId, containerIndex)
                        PageController.goToPage(PageEnum.PageProtocolRaw)
                    } else if (isDns) {
                        PageController.goToPage(PageEnum.PageServiceDnsSettings)
                    } else if (isMtProxy) {
                        MtProxyConfigModel.updateModel(config)
                        PageController.goToPage(PageEnum.PageServiceMtProxySettings, false)
                    } else if (isTelemt) {
                        TelemtConfigModel.updateModel(config)
                        PageController.goToPage(PageEnum.PageServiceTelemtSettings, false)
                    } else if (isTProxy) {
                        TProxyConfigModel.updateModel(config)
                        PageController.goToPage(PageEnum.PageServiceTProxySettings, false)
                    } else {
                        InstallController.updateProtocols(ServersUiController.processedServerId, containerIndex)
                        PageController.goToPage(PageEnum.PageSettingsServerProtocol)
                    }

                } else {
                    var containerIndex = root.model.mapToSource(index)
                    ServersUiController.processedContainerIndex = containerIndex
                    PageController.goToPage(PageEnum.PageSetupWizardProtocolSettings)
                }
            }
        }
    }

    footer: Item {
        width: root.width
        height: 16 + PageController.safeAreaBottomMargin
    }
}

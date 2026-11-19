import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import SortFilterProxyModel 0.2

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

PageType {
    id: root

    ListViewType {
        id: listView

        anchors.fill: parent

        header: ColumnLayout {
            width: listView.width

            z: 2

            spacing: 0

            NavigationBarType {
                Layout.fillWidth: true

                title: qsTr("VPN by Amnezia")
                description: qsTr("Choose a VPN service that suits your needs.")
            }
        }

        spacing: 0

        model: SortFilterProxyModel {
            id: proxyApiServicesModel

            sourceModel: ApiServicesModel
            sorters: RoleSorter {
                roleName: "order"
                sortOrder: Qt.AscendingOrder
            }
        }

        delegate: ColumnLayout {
            property bool hideCard: isPremium && !hasSubscriptionPlans

            width: listView.width
            visible: !hideCard
            height: hideCard ? 0 : implicitHeight

            enabled: isServiceAvailable

            BasicCardType {
                id: card

                Layout.fillWidth: true
                Layout.rightMargin: 16
                Layout.leftMargin: 16
                Layout.bottomMargin: 12

                text: name
                subtitle: cardDescription
                footnote: price

                badgeText: (showRecommended && isServiceAvailable) ? qsTr("Recommended") : ""
                muted: !isServiceAvailable

                onClicked: {
                    if (isServiceAvailable) {
                        ApiServicesModel.setServiceIndex(proxyApiServicesModel.mapToSource(index))
                        if (ApiServicesModel.getSelectedServiceType() === "amnezia-premium") {
                            PageController.goToPage(PageEnum.PageSetupWizardApiPremiumInfo)
                        } else {
                            PageController.goToPage(PageEnum.PageSetupWizardApiFreeInfo)
                        }
                    }
                }
            }
        }

        footer: Item {
            width: listView.width
            height: 4 + PageController.safeAreaBottomMargin
        }
    }
}

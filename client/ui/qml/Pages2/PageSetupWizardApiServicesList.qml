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

    property bool isAmneziaFreeSelected: false

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20 + SettingsController.safeAreaTopMargin

        onActiveFocusChanged: {
            if(backButton.enabled && backButton.activeFocus) {
                listView.positionViewAtBeginning()
            }
        }
    }

    ColumnLayout {
        id: mainLayout

        anchors.top: backButton.bottom
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.topMargin: 16

        spacing: 0

        BaseHeaderType {
            Layout.fillWidth: true
            Layout.rightMargin: 16
            Layout.leftMargin: 16
            Layout.bottomMargin: 24

            headerText: qsTr("VPN by Amnezia")
            descriptionText: qsTr("Choose a VPN service that suits your needs.")
        }

        PremiumBannerType {
            id: premiumBanner

            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.bottomMargin: 16

            visible: root.isAmneziaFreeSelected
            enabled: visible

            onClicked: {
                PageController.goToPage(PageEnum.PageSetupWizardPremiumWebView)
            }

            Keys.onEnterPressed: clicked()
            Keys.onReturnPressed: clicked()
        }

        ListViewType {
            id: listView

            Layout.fillWidth: true
            Layout.fillHeight: true

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
                        var sourceIndex = proxyApiServicesModel.mapToSource(index)
                        
                        // Устанавливаем индекс ПЕРЕД проверкой типа
                        ApiServicesModel.setServiceIndex(sourceIndex)
                        
                        // Проверяем тип через метод
                        var serviceType = ApiServicesModel.getSelectedServiceType()
                        
                        // Также проверяем имя напрямую из делегата
                        var nameLower = name ? name.toString().toLowerCase() : ""
                        var nameHasFree = nameLower.indexOf("free") >= 0
                        
                        // Комбинированная проверка
                        var isAmneziaFree = (serviceType === "amnezia-free") || nameHasFree
                        
                        if (isAmneziaFree) {
                            // Показываем баннер
                            root.isAmneziaFreeSelected = true
                        } else {
                            // Скрываем баннер и переходим на страницу сервиса
                            root.isAmneziaFreeSelected = false
                            PageController.goToPage(PageEnum.PageSetupWizardApiServiceInfo)
                        }
                    }
                }
                
                Keys.onEnterPressed: clicked()
                Keys.onReturnPressed: clicked()
            }
        }
        }
    }
}

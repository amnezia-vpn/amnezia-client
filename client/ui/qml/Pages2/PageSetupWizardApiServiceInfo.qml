import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    defaultActiveFocusItem: focusItem

    property var serviceInfo: ApiServicesModel.getSelectedServiceInfo()

    FlickableType {
        id: fl
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        contentHeight: content.height + continueButton.implicitHeight + continueButton.anchors.bottomMargin

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
//                KeyNavigation.tab: fileButton.rightButton
            }

            HeaderType {
                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                headerText: serviceInfo["name"]
                descriptionText: serviceInfo["description"]
            }

            LabelWithImageType {
                Layout.fillWidth: true
                Layout.margins: 16

                imageSource: "qrc:/images/controls/map-pin.svg"
                leftText: qsTr("For the region")
                rightText: serviceInfo["region"]
            }

            LabelWithImageType {
                Layout.fillWidth: true
                Layout.margins: 16

                imageSource: "qrc:/images/controls/tag.svg"
                leftText: qsTr("Price")
                rightText: serviceInfo["price"]
            }

            LabelWithImageType {
                Layout.fillWidth: true
                Layout.margins: 16

                imageSource: "qrc:/images/controls/history.svg"
                leftText: qsTr("Work period")
                rightText: serviceInfo["timelimit"]
            }

            LabelWithImageType {
                Layout.fillWidth: true
                Layout.margins: 16

                imageSource: "qrc:/images/controls/gauge.svg"
                leftText: qsTr("Speed")
                rightText: serviceInfo["speed"]
            }

            LabelWithImageType {
                Layout.fillWidth: true
                Layout.margins: 16

                imageSource: "qrc:/images/controls/info.svg"
                leftText: qsTr("Features")
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                text: serviceInfo["features"]
            }

            ShowSupportedSitesButton {
                drawerParent: root
                sitesList: serviceInfo["sites_list"]
            }

            Header2TextType {
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.bottomMargin: 8
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                text: qsTr("How to connect")
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.rightMargin: 16
                Layout.leftMargin: 16
                Layout.bottomMargin: 24

                text: serviceInfo["how_to_install"]
            }

            Header2TextType {
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.bottomMargin: 8
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                text: qsTr("How to use")
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.rightMargin: 16
                Layout.leftMargin: 16
                Layout.bottomMargin: 24

                text: serviceInfo["how_to_use"]
            }
        }
    }

    BasicButtonType {
        id: continueButton

        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: parent.bottom

        anchors.rightMargin: 16
        anchors.leftMargin: 16
        anchors.bottomMargin: 48

        text: qsTr("Connect")

        clickedFunc: function() {
            PageController.showBusyIndicator(true)
            InstallController.installServiceFromApi()
            PageController.showBusyIndicator(false)
        }
    }
}

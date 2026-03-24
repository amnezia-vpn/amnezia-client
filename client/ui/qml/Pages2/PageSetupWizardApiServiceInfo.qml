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

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20 + SettingsController.safeAreaTopMargin

        onFocusChanged: {
            if (this.activeFocus) {
                listView.positionViewAtBeginning()
            }
        }
    }

    ListViewType {
        id: listView

        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.left: parent.left

        header: ColumnLayout {
            width: listView.width

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.rightMargin: 16
                Layout.leftMargin: 16
                Layout.bottomMargin: 32

                headerText: ApiServicesModel.getSelectedServiceData("name")
                descriptionText: ApiServicesModel.getSelectedServiceData("serviceDescription")
            }
        }

        model: inputFields
        spacing: 0

        delegate: ColumnLayout {
            width: listView.width

            LabelWithImageType {
                Layout.fillWidth: true
                Layout.margins: 16

                imageSource: imagePath
                leftText: lText
                rightText: rText

                visible: isVisible
            }
        }

        footer: ColumnLayout {
            width: listView.width

            spacing: 0

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                onLinkActivated: function(link) {
                    Qt.openUrlExternally(link)
                }
                textFormat: Text.RichText
                text: {
                    var text = ApiServicesModel.getSelectedServiceData("features")
                    return text.replace("%1", LanguageModel.getCurrentSiteUrl("free")).replace("/free", "") // todo link should come from gateway
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.NoButton
                    cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                }
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: (Qt.platform.os === "ios" || IsMacOsNeBuild) && ApiServicesModel.getSelectedServiceType() === "amnezia-premium"

                horizontalAlignment: Text.AlignHCenter
                textFormat: Text.PlainText
                color: AmneziaStyle.color.mutedGray
                font.pixelSize: 12

                text: qsTr("Charged to your Apple ID at confirmation. Renews automatically unless auto-renew is turned off at least 24 hours before period end. Manage in Apple ID settings.")
            }

            BasicButtonType {
                id: continueButton

                Layout.fillWidth: true
                Layout.topMargin: 32
                Layout.bottomMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: ApiServicesModel.getSelectedServiceType() === "amnezia-premium" ? qsTr("Subscribe Now") : (ApiServicesModel.getSelectedServiceType() === "amnezia-trial" ? qsTr("Try Trial") : qsTr("Connect"))

                clickedFunc: function() {
                    PageController.showBusyIndicator(true)
                    var result = ApiConfigsController.importService()
                    PageController.showBusyIndicator(false)

                    if (!result) {
                        var endpoint = ApiServicesModel.getStoreEndpoint()
                        Qt.openUrlExternally(endpoint)
                        PageController.closePage()
                        PageController.closePage()
                    }
                }
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 32

                visible: (Qt.platform.os === "ios" || IsMacOsNeBuild) && ApiServicesModel.getSelectedServiceType() === "amnezia-premium"

                horizontalAlignment: Text.AlignHCenter
                textFormat: Text.RichText
                color: AmneziaStyle.color.mutedGray
                font.pixelSize: 12

                text: {
                    var termsUrl = "https://www.apple.com/legal/internet-services/itunes/dev/stdeula/"
                    var privacyUrl = LanguageModel.getCurrentSiteUrl("policy")
                    return qsTr("By continuing, you agree to the <a href=\"%1\" style=\"color: #FBB26A;\">Terms of Use</a> and <a href=\"%2\" style=\"color: #FBB26A;\">Privacy Policy</a>").arg(termsUrl).arg(privacyUrl)
                }

                onLinkActivated: function(link) {
                    Qt.openUrlExternally(link)
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.NoButton
                    cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                }
            }
        }
    }

    property list<QtObject> inputFields: [
        region,
        price,
        timeLimit,
        speed,
        features
    ]

    QtObject {
        id: region

        readonly property string imagePath: "qrc:/images/controls/map-pin.svg"
        readonly property string lText: qsTr("For the region")
        readonly property string rText: ApiServicesModel.getSelectedServiceData("region")
        property bool isVisible: true
    }

    QtObject {
        id: price

        readonly property string imagePath: "qrc:/images/controls/tag.svg"
        readonly property string lText: qsTr("Price")
        readonly property string rText: ApiServicesModel.getSelectedServiceData("price")
        property bool isVisible: true
    }

    QtObject {
        id: timeLimit

        readonly property string imagePath: "qrc:/images/controls/history.svg"
        readonly property string lText: qsTr("Work period")
        readonly property string rText: ApiServicesModel.getSelectedServiceData("timeLimit")
        property bool isVisible: rText !== ""
    }

    QtObject {
        id: speed

        readonly property string imagePath: "qrc:/images/controls/gauge.svg"
        readonly property string lText: qsTr("Speed")
        readonly property string rText: ApiServicesModel.getSelectedServiceData("speed")
        property bool isVisible: true
    }

    QtObject {
        id: features

        readonly property string imagePath: "qrc:/images/controls/info.svg"
        readonly property string lText: qsTr("Features")
        readonly property string rText: ""
        property bool isVisible: true
    }
}

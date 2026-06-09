import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0
import ContainerProps 1.0

import "./"
import "../Controls2"
import "../Config"
import "../Controls2/TextTypes"
import "../Components"

PageType {
    id: root

    property int amneziaDnsIndex: ContainerProps.containerFromString("amnezia-dns")
    property int dnsMode: ServersUiController.serverDnsMode(ServersUiController.processedServerId)
    property int selectedIndex: dnsMode
    property bool dnsFieldsVisible: false
    property bool saveButtonVisible: false
    property string primaryDnsText: ServersUiController.getDnsPair(ServersUiController.processedServerId).first
    property string secondaryDnsText: ServersUiController.getDnsPair(ServersUiController.processedServerId).second

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20 + PageController.safeAreaTopMargin

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
            spacing: 16

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("DNS server")
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 16
                
                BasicButtonType {
                    id: privacyPolicyButton

                    Layout.alignment: Qt.AlignHCenter
                    Layout.bottomMargin: 16
                    Layout.topMargin: -15
                    implicitHeight: 25

                    defaultColor: AmneziaStyle.color.transparent
                    hoveredColor: AmneziaStyle.color.translucentWhite
                    pressedColor: AmneziaStyle.color.sheerWhite
                    disabledColor: AmneziaStyle.color.mutedGray
                    textColor: AmneziaStyle.color.goldenApricot

                    text: qsTr("Learn more")

                    clickedFunc: function() {
                        Qt.openUrlExternally(LanguageUiController.getCurrentSiteUrl("dns"))
                    }
                }
            }
        }

        model: dnsList

        ButtonGroup {
            id: dnsChoicesRadioButtonGroup
        }

        delegate: ColumnLayout {
            id: content

            width: listView.width
            height: content.implicitHeight

            RowLayout {
                VerticalRadioButton {
                    id: dnsChoiceRadioButton

                    Layout.fillWidth: true
                    Layout.leftMargin: 16

                    text: title
                    descriptionText: description
                    imageSource: "qrc:/images/controls/download.svg"

                    ButtonGroup.group: dnsChoicesRadioButtonGroup

                    checked: index === selectedIndex
                    checkable: !ConnectionController.isConnected

                    onClicked: {
                        if (ConnectionController.isConnectionInProgress) {
                            PageController.showNotificationMessage(qsTr("Unable change dns settings while trying to make an active connection"))
                            return
                        }
                        if (ConnectionController.isConnected) {
                            PageController.showNotificationMessage(qsTr("Unable change dns settings while there is an active connection"))
                            return
                        }

                        if (index !== selectedIndex) {
                            selectedIndex = index
                            clickedHandler()
                        }
                    }

                    Keys.onEnterPressed: {
                        if (checkable) {
                            checked = true
                        }
                            dnsChoiceRadioButton.clicked()
                        }
                    Keys.onReturnPressed: {
                        if (checkable) {
                            checked = true
                        }
                        dnsChoiceRadioButton.clicked()
                    }
                }

                BasicButtonType {
                    id: removeAmneziaDnsButton

                    visible: ContainersModel.isContainerInstalled(amneziaDnsIndex) && title === "AmneziaDNS"
                    
                    Layout.rightMargin: 32
                    Layout.alignment: Qt.AlignRight

                    defaultColor: AmneziaStyle.color.midnightBlack
                    hoveredColor: AmneziaStyle.color.slateGray
                    pressedColor: AmneziaStyle.color.mutedGray

                    leftImageSource: image
                    leftImageColor: AmneziaStyle.color.paleGray

                    clickedFunc: function() {
                        ContainersModel.setProcessedContainerIndex(amneziaDnsIndex)

                        var headerText = qsTr("Remove %1 from server?").arg(ContainersModel.getProcessedContainerName())
                        var yesButtonText = qsTr("Continue")
                        var noButtonText = qsTr("Cancel")

                        var yesButtonFunction = function() {
                            if (ServersUiController.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected && (root.dnsMode === 1)) {
                                PageController.showNotificationMessage(qsTr("Cannot remove AmneziaDNS from running server"))
                            } else {
                                PageController.goToPage(PageEnum.PageDeinstalling)
                                InstallController.removeContainer(ServersUiController.processedServerId, amneziaDnsIndex)
                            }
                        }
                        var noButtonFunction = function() {}

                        showQuestionDrawer(headerText, "", yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                    }
                }
            }

            DividerType {
            }
        }

        footer: ColumnLayout {
            width: listView.width

            TextFieldWithHeaderType {
                id: primaryDns

                visible: root.dnsFieldsVisible

                Layout.fillWidth: true
                Layout.topMargin: 32
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Primary DNS")

                textField.text: root.primaryDnsText
                textField.validator: RegularExpressionValidator {
                    regularExpression: InstallController.ipAddressRegExp()
                }
            }

            TextFieldWithHeaderType {
                id: secondaryDns

                visible: root.dnsFieldsVisible

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Secondary DNS")

                textField.text: root.secondaryDnsText
                textField.validator: RegularExpressionValidator {
                    regularExpression: InstallController.ipAddressRegExp()
                }
            }

            BasicButtonType {
                id: saveButton

                visible: root.saveButtonVisible || (root.dnsMode === 1 && !ContainersModel.isContainerInstalled(amneziaDnsIndex))

                Layout.fillWidth: true
                Layout.topMargin: root.dnsFieldsVisible ? 8 : 32
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 8

                text: qsTr("Save")

                clickedFunc: function() {
                    if (!ContainersModel.isContainerInstalled(amneziaDnsIndex)) {
                        PageController.goToPage(PageEnum.PageSetupWizardInstalling);
                        InstallController.install(amneziaDnsIndex, InstallController.getPortForInstall(ContainerProps.defaultProtocol(amneziaDnsIndex)),
                                                  InstallController.defaultTransportProto(ContainerProps.defaultProtocol(amneziaDnsIndex)), ServersUiController.processedServerId)
                    }

                    if (primaryDns.textField.text === "") {
                        primaryDns.errorText = qsTr("Primary DNS cannot be empty")
                        return
                    }
                    
                    primaryDns.errorText = ""
                    secondaryDns.errorText = ""
                    
                    root.saveButtonVisible = false
                    root.dnsMode = root.selectedIndex

                    ServersUiController.editServerDns(primaryDns.textField.text, secondaryDns.textField.text, root.dnsMode)
                    PageController.showNotificationMessage(qsTr("Settings saved"))
                }
            }

            CaptionTextType {
                visible: !ContainersModel.isContainerInstalled(amneziaDnsIndex) && root.selectedIndex === 1

                Layout.fillWidth: true

                horizontalAlignment: Text.AlignHCenter

                text: qsTr("The AmneziaDNS service will be installed on the server")
                color: AmneziaStyle.color.mutedGray
            }
        }
    }

    property list<QtObject> dnsList: [
    defaultDns,
    amneziaDns,
    specifyDns,
    ]

    QtObject {
        id: defaultDns
        
        readonly property string title: qsTr("DNS by default")
        readonly property string description: qsTr("1.1.1.1  1.0.0.1")
        readonly property var clickedHandler: function() {
            root.dnsFieldsVisible = false
            root.saveButtonVisible = true

            root.primaryDnsText = "1.1.1.1"
            root.secondaryDnsText = "1.0.0.1"
        }

    }

    QtObject {
        id: amneziaDns
        
        readonly property string title: qsTr("AmneziaDNS")
        readonly property string description: qsTr("AmneziaDNS on your server")
        readonly property string image: qsTr("qrc:/images/controls/trash.svg")
        readonly property var clickedHandler: function() {
            root.dnsFieldsVisible = false
            root.saveButtonVisible = true
        }
    }

    QtObject {
        id: specifyDns
        
        readonly property string title: qsTr("Specify DNS server address")
        readonly property var clickedHandler: function() {
            root.dnsFieldsVisible = true
            root.saveButtonVisible = true
            
            var dnsPair = ServersUiController.getDnsPair(ServersUiController.processedServerId)
            if (!dnsPair) return

            root.primaryDnsText = dnsPair.first || ""
            root.secondaryDnsText = dnsPair.second || ""
        }
    }
}

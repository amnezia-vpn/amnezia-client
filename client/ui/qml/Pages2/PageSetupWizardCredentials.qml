import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Config"
import "../Controls2/TextTypes"

PageType {
    id: root

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20

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
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 16

                headerText: qsTr("Configure your server")
            }
        }

        model: inputFields
        spacing: 16

        delegate: ColumnLayout {
            width: listView.width

            TextFieldWithHeaderType {
                id: delegate

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: title
                textField.echoMode: hideContent ? TextInput.Password : TextInput.Normal
                textField.placeholderText: placeholderContent
                textField.text: textField.text

                rightButtonClickedOnEnter: true

                clickedFunc: function () {
                    clickedHandler()
                }

                textField.onFocusChanged: {
                    textField.text = textField.text.replace(/^\s+|\s+$/g, '')
                }

                textField.onTextChanged: {
                    if (hideContent) {
                        buttonImageSource = textField.text !== "" ? (hideContent ? "qrc:/images/controls/eye.svg" : "qrc:/images/controls/eye-off.svg") : ""
                    }
                }
            }
        }

        footer: ColumnLayout {
            width: listView.width

            BasicButtonType {
                id: continueButton

                Layout.fillWidth: true
                Layout.topMargin: 32
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Continue")

                clickedFunc: function() {
                    if (!root.isCredentialsFilled()) {
                        return
                    }

                    InstallController.setShouldCreateServer(true)
                    var _hostname = listView.itemAtIndex(vars.hostnameIndex).children[0].textField.text
                    var _username = listView.itemAtIndex(vars.usernameIndex).children[0].textField.text
                    var _secretData = listView.itemAtIndex(vars.secretDataIndex).children[0].textField.text

                    InstallController.setProcessedServerCredentials(_hostname, _username, _secretData)

                    PageController.showBusyIndicator(true)
                    var isConnectionOpened = InstallController.checkSshConnection()
                    PageController.showBusyIndicator(false)
                    if (!isConnectionOpened) {
                        return
                    }

                    PageController.goToPage(PageEnum.PageSetupWizardEasy)
                }
            }

            LabelTextType {
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 16

                text: qsTr("All data you enter will remain strictly confidential and will not be shared or disclosed to the Amnezia or any third parties")
            }

            CardWithIconsType {
                id: siteLink

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 16

                headerText: qsTr("How to run your VPN server")
                bodyText: qsTr("Where to get connection data, step-by-step instructions for buying a VPS")

                rightImageSource: "qrc:/images/controls/chevron-right.svg"
                leftImageSource: "qrc:/images/controls/help-circle.svg"

                onClicked: {
                    Qt.openUrlExternally(LanguageModel.getCurrentSiteUrl("starter-guide"))
                }

                Keys.onEnterPressed: this.clicked()
                Keys.onReturnPressed: this.clicked()
            }
        }
    }

    function isCredentialsFilled() {
        var hasEmptyField = false

        var hostnameItem = listView.itemAtIndex(vars.hostnameIndex).children[0]
        if (hostnameItem.textField.text === "") {
            hostnameItem.errorText = qsTr("Ip address cannot be empty")
            hasEmptyField = true
        } else if (!hostnameItem.textField.acceptableInput) {
            hostnameItem.errorText = qsTr("Enter the address in the format 255.255.255.255:88")
        }

        var usernameItem = listView.itemAtIndex(vars.usernameIndex).children[0]
        if (usernameItem.textField.text === "") {
            usernameItem.errorText = qsTr("Login cannot be empty")
            hasEmptyField = true
        }

        var secretDataItem = listView.itemAtIndex(vars.secretDataIndex).children[0]
        if (secretDataItem.textField.text === "") {
            secretDataItem.errorText = qsTr("Password/private key cannot be empty")
            hasEmptyField = true
        }

        return !hasEmptyField
    }

    property list<QtObject> inputFields: [
        hostnameObject,
        usernameObject,
        secretDataObject
    ]

    QtObject {
        id: hostnameObject

        property string title: qsTr("Server IP address [:port]")
        readonly property string placeholderContent: qsTr("255.255.255.255:22")
        property bool hideContent: false
        readonly property var clickedHandler: undefined
    }

    QtObject {
        id: usernameObject

        property string title: qsTr("SSH Username")
        readonly property string placeholderContent: "root"
        property bool hideContent: false
        readonly property var clickedHandler: undefined
    }

    QtObject {
        id: secretDataObject

        property string title: qsTr("Password or SSH private key")
        readonly property string placeholderContent: ""
        property bool hideContent: true
        readonly property var clickedHandler: function() {
            hideContent = !hideContent
        }
    }

    QtObject {
        id: vars

        readonly property int hostnameIndex: 0
        readonly property int usernameIndex: 1
        readonly property int secretDataIndex: 2
    }
}

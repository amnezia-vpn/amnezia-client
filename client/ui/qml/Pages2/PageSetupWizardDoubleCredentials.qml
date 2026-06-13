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

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 16

                headerText: qsTr("Configure Double VPN")
            }
        }

        model: inputFields
        spacing: 16

        delegate: ColumnLayout {
            width: listView.width

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: (index === 4) ? 24 : 0
                
                headerText: isHeader ? title : ""
                visible: isHeader
            }

            TextFieldWithHeaderType {
                id: delegate

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                visible: !isHeader

                headerText: title
                textField.echoMode: hideContent ? TextInput.Password : TextInput.Normal
                textField.placeholderText: placeholderContent
                textField.text: textField.text

                rightButtonClickedOnEnter: true

                clickedFunc: function () {
                    if (clickedHandler) clickedHandler()
                    buttonImageSource = textField.text !== "" ? imageSource : ""
                }

                textField.onFocusChanged: {
                    textField.text = textField.text.replace(/^\s+|\s+$/g, '')
                }

                textField.onTextChanged: {
                    if (headerText === qsTr("Password or SSH private key")) {
                        buttonImageSource = textField.text !== "" ? imageSource : ""
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

                    var _entryIp = listView.itemAtIndex(vars.entryHostnameIndex).children[1].textField.text
                    var _entryUser = listView.itemAtIndex(vars.entryUsernameIndex).children[1].textField.text
                    var _entryPass = listView.itemAtIndex(vars.entrySecretDataIndex).children[1].textField.text

                    var _exitIp = listView.itemAtIndex(vars.exitHostnameIndex).children[1].textField.text
                    var _exitUser = listView.itemAtIndex(vars.exitUsernameIndex).children[1].textField.text
                    var _exitPass = listView.itemAtIndex(vars.exitSecretDataIndex).children[1].textField.text

                    // We set the Entry Node credentials into a special variable in InstallController
                    InstallController.setDoubleVpnEntryCredentials(_entryIp, _entryUser, _entryPass)

                    // The Exit Node becomes the processed server for standard installation
                    InstallController.setProcessedServerCredentials(_exitIp, _exitUser, _exitPass)
                    ServersUiController.setProcessedServerId("")

                    PageController.showBusyIndicator(true)
                    
                    // Verify SSH for Exit Node
                    var isExitOpened = InstallController.checkSshConnection()
                    if (!isExitOpened) {
                        PageController.showBusyIndicator(false)
                        PageController.showErrorMessage(qsTr("Failed to connect to Exit Node"))
                        return
                    }
                    
                    // Verify SSH for Entry Node
                    var isEntryOpened = InstallController.checkDoubleVpnEntryConnection()
                    if (!isEntryOpened) {
                        PageController.showBusyIndicator(false)
                        PageController.showErrorMessage(qsTr("Failed to connect to Entry Node"))
                        return
                    }

                    PageController.showBusyIndicator(false)
                    PageController.goToPage(PageEnum.PageSetupWizardEasy)
                }
            }

            LabelTextType {
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 16

                text: qsTr("Traffic will be routed as: You -> Entry Node -> Exit Node -> Internet")
            }
        }
    }

    function isCredentialsFilled() {
        var hasEmptyField = false

        // A helper function to validate a single field
        function validateField(itemIndex, emptyMsg, formatMsg, isIp) {
            var item = listView.itemAtIndex(itemIndex).children[1]
            if (item.textField.text === "") {
                item.errorText = emptyMsg
                hasEmptyField = true
            } else if (isIp && !item.textField.acceptableInput) {
                item.errorText = formatMsg
                hasEmptyField = true
            } else {
                item.errorText = ""
            }
        }

        validateField(vars.entryHostnameIndex, qsTr("Entry IP cannot be empty"), qsTr("Enter the address in the format 255.255.255.255:88"), true)
        validateField(vars.entryUsernameIndex, qsTr("Entry Login cannot be empty"), "", false)
        validateField(vars.entrySecretDataIndex, qsTr("Entry Password cannot be empty"), "", false)

        validateField(vars.exitHostnameIndex, qsTr("Exit IP cannot be empty"), qsTr("Enter the address in the format 255.255.255.255:88"), true)
        validateField(vars.exitUsernameIndex, qsTr("Exit Login cannot be empty"), "", false)
        validateField(vars.exitSecretDataIndex, qsTr("Exit Password cannot be empty"), "", false)

        return !hasEmptyField
    }

    property list<QtObject> inputFields: [
        entryHeaderObject,
        entryHostnameObject,
        entryUsernameObject,
        entrySecretDataObject,
        exitHeaderObject,
        exitHostnameObject,
        exitUsernameObject,
        exitSecretDataObject
    ]

    QtObject {
        id: entryHeaderObject
        property bool isHeader: true
        property string title: qsTr("Entry Node (First Server)")
        property string placeholderContent: ""
        property bool hideContent: false
    }

    QtObject {
        id: entryHostnameObject
        property bool isHeader: false
        property string title: qsTr("Server IP address [:port]")
        readonly property string placeholderContent: qsTr("255.255.255.255:22")
        property bool hideContent: false
        readonly property var clickedHandler: undefined
    }

    QtObject {
        id: entryUsernameObject
        property bool isHeader: false
        property string title: qsTr("SSH Username")
        readonly property string placeholderContent: "root"
        property bool hideContent: false
        readonly property var clickedHandler: undefined
    }

    QtObject {
        id: entrySecretDataObject
        property bool isHeader: false
        property string title: qsTr("Password or SSH private key")
        readonly property string placeholderContent: ""
        property bool hideContent: true
        property string imageSource: "qrc:/images/controls/eye.svg"
        readonly property var clickedHandler: function() {
            hideContent = !hideContent
            imageSource = hideContent ? "qrc:/images/controls/eye.svg" : "qrc:/images/controls/eye-off.svg"
        }
    }

    QtObject {
        id: exitHeaderObject
        property bool isHeader: true
        property string title: qsTr("Exit Node (Final Server)")
        property string placeholderContent: ""
        property bool hideContent: false
    }

    QtObject {
        id: exitHostnameObject
        property bool isHeader: false
        property string title: qsTr("Server IP address [:port]")
        readonly property string placeholderContent: qsTr("255.255.255.255:22")
        property bool hideContent: false
        readonly property var clickedHandler: undefined
    }

    QtObject {
        id: exitUsernameObject
        property bool isHeader: false
        property string title: qsTr("SSH Username")
        readonly property string placeholderContent: "root"
        property bool hideContent: false
        readonly property var clickedHandler: undefined
    }

    QtObject {
        id: exitSecretDataObject
        property bool isHeader: false
        property string title: qsTr("Password or SSH private key")
        readonly property string placeholderContent: ""
        property bool hideContent: true
        property string imageSource: "qrc:/images/controls/eye.svg"
        readonly property var clickedHandler: function() {
            hideContent = !hideContent
            imageSource = hideContent ? "qrc:/images/controls/eye.svg" : "qrc:/images/controls/eye-off.svg"
        }
    }

    QtObject {
        id: vars

        readonly property int entryHostnameIndex: 1
        readonly property int entryUsernameIndex: 2
        readonly property int entrySecretDataIndex: 3
        
        readonly property int exitHostnameIndex: 5
        readonly property int exitUsernameIndex: 6
        readonly property int exitSecretDataIndex: 7
    }
}

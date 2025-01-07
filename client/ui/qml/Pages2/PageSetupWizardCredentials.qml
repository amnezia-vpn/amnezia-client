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

    ListView {
        id: listView
        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.left: parent.left

        property bool isFocusable: true

        Keys.onTabPressed: {
            FocusController.nextKeyTabItem()
        }

        Keys.onBacktabPressed: {
            FocusController.previousKeyTabItem()
        }

        Keys.onUpPressed: {
            FocusController.nextKeyUpItem()
        }

        Keys.onDownPressed: {
            FocusController.nextKeyDownItem()
        }

        Keys.onLeftPressed: {
            FocusController.nextKeyLeftItem()
        }

        Keys.onRightPressed: {
            FocusController.nextKeyRightItem()
        }

        ScrollBar.vertical: ScrollBarType {}

        header: ColumnLayout {
            width: listView.width

            HeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 16

                headerText: qsTr("Configure your server")
            }
        }

        model: inputFields
        spacing: 16
        clip: true
        reuseItems: true

        delegate: ColumnLayout {
            property alias textField: _textField.textField

            width: listView.width

            TextFieldWithHeaderType {
                id: _textField

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                property bool hidePassword: hideText

                headerText: title
                textField.echoMode: hideText ? TextInput.Password : TextInput.Normal
                buttonImageSource: imageSource
                textFieldPlaceholderText: placeholderText
                textField.text: textFieldText

                rightButtonClickedOnEnter: true

                clickedFunc: function () {
                    clickedHandler()
                }

                textField.onFocusChanged: {
                    var _currentIndex = listView.currentIndex
                    var _currentItem = listView.itemAtIndex(_currentIndex).children[0]
                    listView.model[_currentIndex].textFieldText = _currentItem.textFieldText.replace(/^\s+|\s+$/g, '')
                }

                textField.onTextChanged: {
                    var _currentIndex = listView.currentIndex
                    textFieldText = textField.text

                    if (_currentIndex === vars.secretDataIndex) {
                        buttonImageSource = textFieldText !== "" ? (hideText ? "qrc:/images/controls/eye.svg" : "qrc:/images/controls/eye-off.svg") : ""
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
                    var _hostname = listView.itemAtIndex(vars.hostnameIndex).children[0].textFieldText
                    var _username = listView.itemAtIndex(vars.usernameIndex).children[0].textFieldText
                    var _secretData = listView.itemAtIndex(vars.secretDataIndex).children[0].textFieldText

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
                    Qt.openUrlExternally(LanguageModel.getCurrentSiteUrl() + "/starter-guide")
                }
            }
        }
    }

    function isCredentialsFilled() {
        var hasEmptyField = false

        var _hostname = listView.itemAtIndex(vars.hostnameIndex).children[0]
        if (_hostname.textFieldText === "") {
            _hostname.errorText = qsTr("Ip address cannot be empty")
            hasEmptyField = true
        } else if (!_hostname.textField.acceptableInput) {
            _hostname.errorText = qsTr("Enter the address in the format 255.255.255.255:88")
        }

        var _username = listView.itemAtIndex(vars.usernameIndex).children[0]
        if (_username.textFieldText === "") {
            _username.errorText = qsTr("Login cannot be empty")
            hasEmptyField = true
        }

        var _secretData = listView.itemAtIndex(vars.secretDataIndex).children[0]
        if (_secretData.textFieldText === "") {
            _secretData.errorText = qsTr("Password/private key cannot be empty")
            hasEmptyField = true
        }

        return !hasEmptyField
    }

    property list<QtObject> inputFields: [
        hostname,
        username,
        secretData
    ]

    QtObject {
        id: hostname

        property string title: qsTr("Server IP address [:port]")
        readonly property string placeholderText: qsTr("255.255.255.255:22")
        property string textFieldText: ""
        property bool hideText: false
        property string imageSource: ""
        readonly property var clickedHandler: function() {
            console.debug(">>> Server IP address text field was clicked!!!")
            clicked()
        }
    }

    QtObject {
        id: username

        property string title: qsTr("SSH Username")
        readonly property string placeholderText: "root"
        property string textFieldText: ""
        property bool hideText: false
        property string imageSource: ""
        readonly property var clickedHandler: undefined
    }

    QtObject {
        id: secretData

        property string title: qsTr("Password or SSH private key")
        readonly property string placeholderText: ""
        property string textFieldText: ""
        property bool hideText: true
        property string imageSource: textFieldText !== "" ? (hideText ? "qrc:/images/controls/eye.svg" : "qrc:/images/controls/eye-off.svg") : ""
        readonly property var clickedHandler: function() {
            hideText = !hideText
        }
    }

    QtObject {
        id: vars

        readonly property int hostnameIndex: 0
        readonly property int usernameIndex: 1
        readonly property int secretDataIndex: 2
    }
}

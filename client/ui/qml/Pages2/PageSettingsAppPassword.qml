import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Config"
import "../Controls2/TextTypes"
import "../Components"

PageType {
    id: root

    property bool isChangingPassword: false

    Connections {
        target: SettingsController

        function onChangingPassword() {
            root.isChangingPassword = true
        }
    }

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

        backButtonFunction: function() {
            PageController.closePage()
            if (root.isChangingPassword) {
                root.isChangingPassword = false
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

                headerText: root.isChangingPassword ? qsTr("Change password") : qsTr("Password & Encryption")
                descriptionText: root.isChangingPassword ? qsTr("Existing encrypted files will still require the old password.\nThe new password will be used for new encrypted files.")
                                                         : qsTr("Password protection for backups and configuration files.\nRequired to restore or import encrypted files.")
            }

            BasicButtonType {
                Layout.leftMargin: 8
                Layout.bottomMargin: 16
                implicitHeight: 16

                defaultColor: AmneziaStyle.color.transparent
                hoveredColor: AmneziaStyle.color.translucentWhite
                pressedColor: AmneziaStyle.color.sheerWhite
                disabledColor: AmneziaStyle.color.mutedGray
                textColor: AmneziaStyle.color.goldenApricot

                text: qsTr("Learn more")

                clickedFunc: function() {
                    Qt.openUrlExternally("https://storage.googleapis.com/amnezia/docs?m-path=/documentation/instructions/encryption")
                }
            }

            EncryptionIndicator {
                id: indicator

                visible: !root.isChangingPassword

                textString: qsTr("Password not set. Encryption disabled")
                iconPath: "qrc:/images/controls/lock-unlocked.svg"
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
                    buttonImageSource = textField.text !== "" ? imageSource : ""
                }

                textField.onFocusChanged: {
                    textField.text = textField.text.replace(/^\s+|\s+$/g, '')
                }

                textField.onTextChanged: {
                    buttonImageSource = textField.text !== "" ? imageSource : ""
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
                    if (!root.isPasswordProperlyFilled()) {
                        return
                    }

                    var _password = listView.itemAtIndex(vars.passwordIndex).children[0].textField.text
                    var _hint = listView.itemAtIndex(vars.hintIndex).children[0].textField.text

                    SettingsController.setTempPassword(_password)
                    SettingsController.setTempHint(_hint)

                    PageController.goToPage(PageEnum.PageSettingsAppPasswordConfirm)
                    if (root.isChangingPassword) {
                        SettingsController.changingPassword()
                    }
                }
            }
        }
    }

    function isPasswordProperlyFilled() {
        var tooShort = false

        var secretDataItem = listView.itemAtIndex(vars.passwordIndex).children[0]
        if (secretDataItem.textField.text === "") {
            secretDataItem.errorText = qsTr("Password cannot be empty")
            tooShort = true
        } else if (secretDataItem.textField.text.length < 4) {
            secretDataItem.errorText = qsTr("Password too short")
            tooShort = true
        }

        return !tooShort
    }

    property list<QtObject> inputFields: [
        passwordObject,
        hintObject
    ]

    QtObject {
        id: passwordObject

        property string title: root.isChangingPassword ? qsTr("New password") : qsTr("Set encryption password")
        readonly property string placeholderContent: ""
        property string imageSource: "qrc:/images/controls/eye.svg"
        property bool hideContent: true
        readonly property var clickedHandler: function() {
            hideContent = !hideContent
            imageSource = hideContent ? "qrc:/images/controls/eye.svg" : "qrc:/images/controls/eye-off.svg"
        }
    }

    QtObject {
        id: hintObject

        property string title: root.isChangingPassword ? qsTr("New password hint (optional)") : qsTr("Password hint")
        readonly property string placeholderContent: ""
        property string imageSource: ""
        property bool hideContent: false
        readonly property var clickedHandler: undefined
    }

    QtObject {
        id: vars

        readonly property int passwordIndex: 0
        readonly property int hintIndex: 1
    }
}
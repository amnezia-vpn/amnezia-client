import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root
    property string trialEmailErrorMessage: ""

    Connections {
        target: SubscriptionUiController

        function onTrialEmailError(message) {
            root.trialEmailErrorMessage = message
            emailField.errorText = message
        }
    }

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20 + PageController.safeAreaTopMargin

        onFocusChanged: {
            if (activeFocus) {
                flick.contentY = 0
            }
        }
    }

    FlickableType {
        id: flick

        anchors.top: backButton.bottom
        anchors.bottom: continueButton.top
        anchors.left: parent.left
        anchors.right: parent.right

        contentHeight: scrollColumn.implicitHeight + 24

        ColumnLayout {
            id: scrollColumn

            width: flick.width
            spacing: 0

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 24

                headerText: qsTr("Create an account")
                descriptionText: qsTr("To manage your subscription")
            }

            TextFieldWithHeaderType {
                id: emailField

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 24

                headerText: qsTr("Email")
                textField.placeholderText: qsTr("Email")
                textField.inputMethodHints: Qt.ImhEmailCharactersOnly

                Connections {
                    target: emailField.textField

                    function onTextChanged() {
                        if (root.trialEmailErrorMessage !== "") {
                            root.trialEmailErrorMessage = ""
                            emailField.errorText = ""
                        }
                    }
                }
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 24

                wrapMode: Text.WordWrap
                color: AmneziaStyle.color.mutedGray
                font.pixelSize: 12
                text: qsTr("We will create an account for your trial subscription and send important subscription updates to this email address")
            }
        }
    }

    BasicButtonType {
        id: continueButton

        z: 2
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.bottomMargin: 16 + PageController.safeAreaBottomMargin

        text: qsTr("Continue")

        clickedFunc: function() {
            root.trialEmailErrorMessage = ""
            emailField.errorText = ""

            var raw = emailField.textField.text.trim()
            if (raw.length === 0 || raw.indexOf("@") < 0) {
                PageController.showNotificationMessage(qsTr("Enter a valid email address"))
                return
            }
            PageController.showBusyIndicator(true)
            var ok = SubscriptionUiController.importTrialFromGateway(raw)
            PageController.showBusyIndicator(false)
            if (ok) {
                PageController.closePage()
                PageController.closePage()
            }
        }
    }
}

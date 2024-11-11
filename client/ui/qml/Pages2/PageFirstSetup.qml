import QtQuick
import QtQuick.Controls
import QtQuick.Shapes
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

    ColumnLayout {
        id: content
        anchors.fill: parent

        Image {
            id: image
            source: "qrc:/images/zlovpn.svg"

            // antialiasing
            sourceSize.width: width
            sourceSize.height: height
            smooth: true
            antialiasing: true

            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            Layout.topMargin: 64
            Layout.preferredWidth: 300
            Layout.preferredHeight: 240
        }

        Header2Type {
            headerText: qsTr("Initial setup is running")
            Layout.alignment: Qt.AlignHCenter
        }

        ParagraphTextType {
            Layout.alignment: Qt.AlignHCenter

            text: qsTr("Please wait")
            color: AmneziaStyle.color.mutedGray
        }

        ParagraphTextType {
            id: errorText
            opacity: 0

            Layout.topMargin: 32
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 280
            Layout.maximumWidth: parent.width

            text: "Go to System Settings -> General -> Login Items & Extensions and allow ZloVPN.app to run in the background. After doing so, restart the app."
            wrapMode: Text.WordWrap

            Behavior on opacity {
                PropertyAnimation { duration: 200 }
            }
        }

        Item {
            Layout.preferredHeight: image.height - busyIndicator.height - errorText.height
            Layout.preferredWidth: image.width
        }

        SpinnerType {}
    }

    Connections {
        target: FirstSetupController

        function onFirstSetupFinished() {
            PageController.goToPageHome()
        }

        function onFirstSetupFailed(requiresApproval, message) {
            if (requiresApproval) {
                errorText.opacity = 1
            } else {
                PageController.showErrorMessage(message)
            }
        }
    }

    Component.onCompleted: {
        if (FirstSetupController.firstSetupNeeded()) {
            FirstSetupController.doFirstSetup();
        } else {
            PageController.goToPageHome()
        }
    }
}

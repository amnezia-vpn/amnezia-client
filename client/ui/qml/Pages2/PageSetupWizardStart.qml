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

    defaultActiveFocusItem: focusItem

    ColumnLayout {
        id: content

        anchors.fill: parent
        spacing: 0

        Image {
            id: image
            source: "qrc:/images/zlovpn.svg"

            // antialiasing
            sourceSize.width: width
            sourceSize.height: height
            smooth: true
            antialiasing: true

            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            Layout.topMargin: 32
            Layout.preferredWidth: 300
            Layout.preferredHeight: 240
        }

        Item {
            id: focusItem
            KeyNavigation.tab: loginButton
        }

        ColumnLayout {
            spacing: 4

            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.bottomMargin: 24
            Layout.alignment: Qt.AlignBottom

            BasicButtonType {
                id: loginButton
                Layout.fillWidth: true

                text: qsTr("Login")

                KeyNavigation.tab: registerButton

                onClicked: {
                    PageController.goToPage(PageEnum.PageLogin)
                }
            }

            BasicButtonType {
                id: registerButton
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignBottom

                defaultColor: AmneziaStyle.color.transparent
                hoveredColor: AmneziaStyle.color.translucentWhite
                pressedColor: AmneziaStyle.color.sheerWhite
                disabledColor: AmneziaStyle.color.mutedGray
                textColor: AmneziaStyle.color.mutedGray
                leftImageColor: AmneziaStyle.color.transparent

                text: qsTr("Register")

                KeyNavigation.tab: loginButton

                onClicked: {
                    PageController.goToPage(PageEnum.PageRegister)
                }
            }
        }
    }
}

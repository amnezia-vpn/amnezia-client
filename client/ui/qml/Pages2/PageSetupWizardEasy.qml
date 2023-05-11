import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0

import "./"
import "../Pages"
import "../Controls2"
import "../Config"

PageBase {
    id: root
    page: PageEnum.PageSetupWizardEasy

    FlickableType {
        id: fl
        anchors.top: root.top
        anchors.bottom: root.bottom
        contentHeight: content.height

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.rightMargin: 16
            anchors.leftMargin: 16

            spacing: 16

            HeaderType {
                Layout.fillWidth: true
                Layout.topMargin: 20

                backButtonImage: "qrc:/images/controls/arrow-left.svg"

                headerText: "Какой уровень контроля интернета в вашем регионе?"
            }

            CardType {
                Layout.fillWidth: true

                headerText: "Высокий"
                bodyText: "Многие иностранные сайты и VPN-провайдеры заблокированы"
            }

            CardType {
                Layout.fillWidth: true

                checked: true

                headerText: "Средний"
                bodyText: "Некоторые иностранные сайты заблокированы, но VPN-провайдеры не блокируются"
            }

            CardType {
                Layout.fillWidth: true

                headerText: "Низкий"
                bodyText: "Хочу просто повысить уровень приватности"
            }

            BasicButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.bottomMargin: 32

                text: qsTr("Продолжить")
            }
        }
    }
}

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Page {
    id: cryptpadPage
    title: qsTr("CryptPad")

    ColumnLayout {
        anchors.fill: parent
        spacing: 24
        anchors.margins: 24

        Label {
            text: qsTr("CryptPad — защищённый онлайн-редактор для совместной работы над документами, таблицами и презентациями. Все данные шифруются на стороне клиента, что обеспечивает максимальную приватность.")
            wrapMode: Text.WordWrap
            font.pixelSize: 18
            Layout.fillWidth: true
        }

        Label {
            text: qsTr("\nКак пользоваться CryptPad:")
            font.bold: true
            font.pixelSize: 16
            Layout.fillWidth: true
        }

        Label {
            text: qsTr("1. Установите и запустите контейнер CryptPad на сервере через панель управления Amnezia.\n2. После запуска контейнера откройте CryptPad в браузере по адресу, который будет выведен в настройках сервиса.\n3. Создайте или откройте документы, делитесь ссылками для совместной работы.\n\nВнимание: CryptPad работает как отдельный сервис и не связан с VPN-трафиком. Для доступа к нему используйте браузер и адрес, указанный в настройках сервера.")
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Button {
            text: qsTr("Открыть CryptPad в браузере")
            onClicked: Qt.openUrlExternally("http://<ваш-сервер>:3000") // TODO: заменить на реальный адрес сервиса
            Layout.topMargin: 16
            Layout.alignment: Qt.AlignLeft
        }
    }
} 
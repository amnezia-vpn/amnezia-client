import QtQuick 2.15
import QtQuick.Controls 2.15
import QtWebEngine 1.7

Page {
    id: cryptpadPage
    title: qsTr("CryptPad")

    WebEngineView {
        anchors.fill: parent
        url: "https://your-cryptpad-server.example.com" // TODO: заменить на реальный URL или сделать настраиваемым
    }
} 
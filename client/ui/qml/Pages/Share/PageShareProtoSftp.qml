import QtQuick 2.12
import QtQuick.Controls 2.12
import ProtocolEnum 1.0
import "../"
import "../../Controls"
import "../../Config"

PageShareProtocolBase {
    id: root
    protocol: ProtocolEnum.Sftp

    BackButton {
        id: back
    }

    Caption {
        id: caption
        text: qsTr("Share SFTF settings")
    }

 }

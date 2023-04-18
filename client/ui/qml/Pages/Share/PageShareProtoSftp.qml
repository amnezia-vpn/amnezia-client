import QtQuick
import QtQuick.Controls
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
        text: qsTr("Share SFTP settings")
    }

 }

import QtQuick
import QtQuick.Controls
import ProtocolEnum 1.0
import "../"
import "../../Controls"
import "../../Config"

PageShareProtocolBase {
    id: root
    protocol: ProtocolEnum.TorWebSite

    BackButton {
        id: back
    }

    Caption {
        id: caption
        text: qsTr("Share Tor Web site")
    }
}

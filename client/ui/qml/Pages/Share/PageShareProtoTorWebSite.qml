import QtQuick 2.12
import QtQuick.Controls 2.12
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
        text: qsTr("Share TOR Web site")
    }
}
